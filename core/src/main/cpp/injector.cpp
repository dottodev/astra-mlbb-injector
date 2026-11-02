#include "injector.h"
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace astra {

// Minimal JSON parser for flat "key": "0x.." | number objects
struct MiniJson {
    const char* raw;
    int rawLen;
    int findKey(const char* key) const {
        const char* p = raw;
        int len = rawLen;
        while (len > 0) {
            if (*p == '"') {
                const char* q = p + 1;
                int k = 0;
                while (k < len - 1 && q[k] && q[k] != '"') k++;
                if (k == (int)strlen(key) && strncmp(q, key, k) == 0) {
                    return (int)(p - raw + 1 + k + 1); // after key"
                }
                p += 2 + k; len -= 2 + k;
            } else { p++; len--; }
        }
        return -1;
    }
    int getStr(const char* key, char* out, int outSize) const {
        int p = findKey(key);
        if (p < 0 || p >= rawLen) return 0;
        while (p < rawLen && (raw[p] == ' ' || raw[p] == '\t' || raw[p] == '\n' || raw[p] == ':')) p++;
        if (p >= rawLen) return 0;
        if (raw[p] == '"') {
            p++; int q = p;
            while (q < rawLen && raw[q] != '"' && q - p < outSize - 1) q++;
            int n = q - p; memcpy(out, raw + p, n); out[n] = 0; return n;
        }
        int q = p;
        while (q < rawLen && raw[q] != ',' && raw[q] != '}' && raw[q] != '\n' && q - p < outSize - 1) q++;
        int n = q - p; memcpy(out, raw + p, n); out[n] = 0; return n;
    }
    bool has(const char* key) const { return findKey(key) >= 0; }
};

static bool parseHex(const char* s, uintptr_t& out) {
    while (*s == ' ' || *s == '\t') s++;
    out = (uintptr_t)strtoull(s, nullptr, 0);
    return true;
}

// ------------------------- process discovery --------------------------------

static bool findPackagePidFile(const char* packageName, char* outPid, int outSize) {
    DIR* d = opendir("/proc");
    if (!d) return false;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr) {
        if (e->d_name[0] < '0' || e->d_name[0] > '9') continue;
        char path[64];
        snprintf(path, sizeof(path), "/proc/%s/cmdline", e->d_name);
        FILE* f = fopen(path, "r");
        if (!f) continue;
        char cmd[256] = {0};
        size_t n = fread(cmd, 1, sizeof(cmd) - 1, f);
        fclose(f);
        if (n && strcmp(cmd, packageName) == 0) {
            int l = strlen(e->d_name);
            if (l < outSize) { memcpy(outPid, e->d_name, l + 1); }
            closedir(d);
            return true;
        }
    }
    closedir(d);
    return false;
}

pid_t findProcess(const char* packageName) {
    char pid[16];
    if (!findPackagePidFile(packageName, pid, 16)) return -1;
    return (pid_t)atoi(pid);
}

pid_t findProcessByLibName(const char* libName) {
    DIR* d = opendir("/proc");
    if (!d) return -1;
    struct dirent* e;
    pid_t found = -1;
    while ((e = readdir(d)) != nullptr) {
        if (e->d_name[0] < '0' || e->d_name[0] > '9') continue;
        char maps[64];
        snprintf(maps, sizeof(maps), "/proc/%s/maps", e->d_name);
        FILE* f = fopen(maps, "r");
        if (!f) continue;
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, libName)) {
                if (found == -1) found = (pid_t)atoi(e->d_name);
            }
        }
        fclose(f);
        if (found != -1) break;
    }
    closedir(d);
    return found;
}

// ------------------------- attach -------------------------------------------------

static bool g_attached = false;
static pid_t g_pid = -1;

bool attach(pid_t pid) {
    if (g_attached && g_pid == pid) return true;
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == 0) {
        waitpid(pid, nullptr, 0);
        g_attached = true;
        g_pid = pid;
        return true;
    }
    char mem[32];
    snprintf(mem, sizeof(mem), "/proc/%d/mem", pid);
    if (access(mem, F_OK) == 0) {
        g_pid = pid;
        g_attached = true;
        return true;
    }
    return false;
}

bool detach(pid_t pid) {
    if (!g_attached) return true;
    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
    g_attached = false;
    g_pid = -1;
    return true;
}

bool isAttached(pid_t pid) {
    return g_attached && g_pid == pid;
}

// ------------------------- memory I/O --------------------------------------------

bool readMem(pid_t pid, uintptr_t addr, void* out, size_t len) {
    if (pid == getpid()) { memcpy(out, (void*)addr, len); return true; }
    char path[32];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDWR);
    if (fd < 0) return false;
    bool ok = pread64(fd, out, len, (off64_t)addr) == (ssize_t)len;
    close(fd);
    return ok;
}

bool writeMem(pid_t pid, uintptr_t addr, const void* in, size_t len) {
    if (pid == getpid()) { memcpy((void*)addr, in, len); return true; }
    char path[32];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDWR);
    if (fd < 0) return false;
    bool ok = pwrite64(fd, in, len, (off64_t)addr) == (ssize_t)len;
    close(fd);
    return ok;
}




// ------------------------- module base ------------------------------------------

uintptr_t moduleBase(pid_t pid, const char* libName, bool* ok) {
    char maps[64];
    snprintf(maps, sizeof(maps), "/proc/%d/maps", pid);
    FILE* f = fopen(maps, "r");
    if (!f) { if (ok) *ok = false; return 0; }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, libName) && !strstr(line, "---p")) {
            uintptr_t start = strtoull(line, nullptr, 16);
            fclose(f);
            if (ok) *ok = true;
            return start;
        }
    }
    fclose(f);
    if (ok) *ok = false;
    return 0;
}

// ------------------------- pattern scan ------------------------------------------

static bool parseSig(const char* pattern, uint8_t* bytes, bool* mask, int& outLen) {
    int n = 0;
    while (*pattern && n < 256) {
        while (*pattern == ' ' || *pattern == '\t') pattern++;
        if (!*pattern) break;
        if (pattern[0] == '?' && (pattern[1] == '?' || pattern[1] == ' ' || pattern[1] == 0)) {
            bytes[n] = 0; mask[n] = false; n++; pattern += 2;
        } else {
            bytes[n] = (uint8_t)strtol(pattern, nullptr, 16); mask[n] = true; n++;
            while (*pattern && *pattern != ' ' && *pattern != '\t') pattern++;
        }
    }
    outLen = n;
    return n > 0;
}

uintptr_t patternScan(pid_t pid, uintptr_t start, uintptr_t end, const char* pattern) {
    uint8_t sig[256]; bool mask[256]; int sigLen = 0;
    if (!parseSig(pattern, sig, mask, sigLen)) return 0;
    const size_t chunk = 4096;
    uint8_t buf[4096 + 256];
    for (uintptr_t base = start; base < end; base += chunk) {
        size_t need = (chunk + sigLen < end - base) ? chunk + sigLen : end - base;
        if (!readMem(pid, base, buf, need)) continue;
        for (size_t i = 0; i + sigLen <= need; ++i) {
            bool match = true;
            for (int j = 0; j < sigLen; ++j) {
                if (mask[j] && buf[i + j] != sig[j]) { match = false; break; }
            }
            if (match) return base + i;
        }
    }
    return 0;
}

// ------------------------- offset table ------------------------------------------

static bool parseField(const MiniJson& j, const char* key, uintptr_t& out) {
    char buf[128];
    if (!j.has(key) || j.getStr(key, buf, 128) <= 0) return false;
    parseHex(buf, out);
    return true;
}

bool loadOffsets(const char* jsonPath, OffsetTable* out) {
    FILE* f = fopen(jsonPath, "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 8192) { fclose(f); return false; }
    char* raw = (char*)malloc(sz + 1);
    fread(raw, 1, sz, f); raw[sz] = 0; fclose(f);
    MiniJson j{ raw, (int)sz };
    j.getStr("version", out->version, 64);
    parseField(j, "entityList", out->entityList);
    parseField(j, "entityCount", out->entityCount);
    parseField(j, "entityStride", out->entityStride);
    parseField(j, "posX", out->posX);
    parseField(j, "posY", out->posY);
    parseField(j, "posZ", out->posZ);
    parseField(j, "team", out->team);
    parseField(j, "health", out->health);
    parseField(j, "heroId", out->heroId);
    parseField(j, "camera", out->camera);
    parseField(j, "aimVector", out->aimVector);
    parseField(j, "localPlayer", out->localPlayer);
    parseField(j, "lobbyHandler", out->lobbyHandler);
    free(raw);
    return true;
}

bool OffsetTable::valid() const {
    return entityList && entityCount && entityStride && posX && posY && posZ;
}

const char* offsetsPath() {
    return "/data/data/com.astra.injector/files/offsets.json";
}

bool versionMatches(const OffsetTable& t, const char* gameVersion) {
    return t.version[0] == 0 || strcmp(t.version, gameVersion) == 0;
}

} // namespace astra