#include "injector.h"
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <elf.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "astra", __VA_ARGS__)
#else
#include <stdio.h>
#define LOGI(...) printf(__VA_ARGS__)
#endif

using namespace astra;

uintptr_t resolveRemoteSymbol(int pid, uintptr_t base, const char* name);

#if defined(__aarch64__)

struct Injector {
    pid_t pid;
    struct user_pt_regs regs, saved;

    bool getRegs(struct user_pt_regs* r) {
        struct iovec io = { r, sizeof(*r) };
        return ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &io) == 0;
    }
    bool setRegs(struct user_pt_regs* r) {
        struct iovec io = { r, sizeof(*r) };
        return ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &io) == 0;
    }
    bool remoteWrite(uintptr_t addr, const void* buf, size_t len) {
        const size_t word = sizeof(uintptr_t);
        const char* p = (const char*)buf;
        size_t done = 0;
        while (done < len) {
            uintptr_t target = (addr + done) & ~(word - 1);
            size_t off = (addr + done) - target;
            size_t chunk = word - off;
            if (chunk > len - done) chunk = len - done;
            uint64_t val = 0;
            if (!readMem(pid, target, &val, word)) return false;
            memcpy((char*)&val + off, p + done, chunk);
            if (ptrace(PTRACE_POKEDATA, pid, (void*)target, (void*)val) < 0) return false;
            done += chunk;
        }
        return true;
    }
};

static const uint32_t dlopenStub[] = {
    0x58000040, // ldr x0, #8          (path ptr)
    0xD2800041, // movz x1, #2         (RTLD_NOW)
    0x58000052, // ldr x2, #8          (dlopen fn)
    0xD63F0040, // blr x2
    0xD65F03C0, // ret
};

#else
struct Injector { };
#endif

bool injectPayload(int pid, const char* soPath, size_t pathLen) {
#if defined(__aarch64__)
    Injector inj{pid};
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) return false;
    waitpid(pid, nullptr, 0);
    if (!inj.getRegs(&inj.regs)) { ptrace(PTRACE_DETACH, pid, nullptr, nullptr); return false; }
    inj.saved = inj.regs;

    bool ok = false;
    uintptr_t dlopenBase = 0;
    {
        char maps[64]; snprintf(maps, sizeof(maps), "/proc/%d/maps", pid);
        FILE* f = fopen(maps, "r");
        char line[512];
        while (f && fgets(line, sizeof(line), f)) {
            if (strstr(line, "libdl.so")) {
                dlopenBase = strtoull(line, nullptr, 16);
                break;
            }
        }
        if (f) fclose(f);
    }
    if (!dlopenBase) { ptrace(PTRACE_DETACH, pid, nullptr, nullptr); return false; }

    uintptr_t dlopenAddr = resolveRemoteSymbol(pid, dlopenBase, "dlopen");
    if (!dlopenAddr) { ptrace(PTRACE_DETACH, pid, nullptr, nullptr); return false; }

    uintptr_t scratch = inj.regs.sp - 0x400;
    char scratchBuf[0x400] = {0};
    if (!inj.remoteWrite(scratch, scratchBuf, sizeof(scratchBuf))) { /* best-effort */ }

    if (!inj.remoteWrite(scratch, soPath, pathLen)) { ptrace(PTRACE_DETACH, pid, nullptr, nullptr); return false; }

    const size_t codeLen = sizeof(dlopenStub);
    if (!inj.remoteWrite(scratch + 0x100, dlopenStub, codeLen)) { ptrace(PTRACE_DETACH, pid, nullptr, nullptr); return false; }
    uintptr_t litPath = scratch + 0x100 + 0x20;
    uintptr_t litFn = scratch + 0x100 + 0x28;
    inj.remoteWrite(litPath, &scratch, sizeof(uintptr_t));
    inj.remoteWrite(litFn, &dlopenAddr, sizeof(uintptr_t));

    inj.regs.pc = scratch + 0x100;
    inj.setRegs(&inj.regs);

    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitpid(pid, nullptr, 0);
    inj.setRegs(&inj.saved);
    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
    return true;
#else
    (void)pid; (void)soPath; (void)pathLen;
    return false;
#endif
}

// Walk a remote module's .dynsym/.dynstr to resolve `name`.
uintptr_t resolveRemoteSymbol(int pid, uintptr_t base, const char* name) {
    auto rd = [&](uintptr_t a, void* b, size_t n) { return readMem(pid, a, b, n); };
    Elf64_Ehdr eh{};
    if (!rd(base, &eh, sizeof(eh))) return 0;
    if (memcmp(eh.e_ident, "\177ELF", 4) != 0) return 0;

    Elf64_Phdr ph{};
    Elf64_Dyn dyn{};
    uintptr_t symtab = 0, strtab = 0, syment = 0, strsz = 0;
    for (uint16_t i = 0; i < eh.e_phnum; ++i) {
        if (!rd(base + eh.e_phoff + i * sizeof(ph), &ph, sizeof(ph))) break;
        if (ph.p_type != PT_DYNAMIC) continue;
        for (uintptr_t off = 0; ; off += sizeof(dyn)) {
            if (!rd(ph.p_vaddr + base + off, &dyn, sizeof(dyn))) break;
            if (dyn.d_tag == DT_NULL) break;
            if (dyn.d_tag == DT_SYMTAB) symtab = dyn.d_un.d_ptr;
            if (dyn.d_tag == DT_STRTAB) strtab = dyn.d_un.d_ptr;
            if (dyn.d_tag == DT_SYMENT) syment = dyn.d_un.d_val;
            if (dyn.d_tag == DT_STRSZ) strsz = dyn.d_un.d_val;
        }
        break;
    }
    if (!symtab || !strtab || !syment || !strsz) return 0;

    char* strtabBuf = (char*)malloc(strsz);
    if (!strtabBuf) return 0;
    if (!rd(strtab, strtabBuf, strsz)) { free(strtabBuf); return 0; }

    for (uintptr_t off = 0; ; off += syment) {
        Elf64_Sym sym{};
        if (!rd(symtab + off, &sym, sizeof(sym))) break;
        if (sym.st_name == 0) continue;
        if (sym.st_name >= strsz) break;
        const char* s = strtabBuf + sym.st_name;
        if (strcmp(s, name) == 0) { free(strtabBuf); return sym.st_value; }
    }
    free(strtabBuf);
    return 0;
}