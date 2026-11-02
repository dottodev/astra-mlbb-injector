#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace astra {

// ---- process discovery / attach -------------------------------------------
pid_t findProcess(const char* packageName);
pid_t findProcessByLibName(const char* libName);
bool attach(pid_t pid);
bool detach(pid_t pid);
bool isAttached(pid_t pid);

// ---- memory I/O ------------------------------------------------------------
bool readMem(pid_t pid, uintptr_t addr, void* out, size_t len);
bool writeMem(pid_t pid, uintptr_t addr, const void* in, size_t len);
template <typename T> T readValue(pid_t pid, uintptr_t addr, bool* ok) {
    T v{}; bool r = readMem(pid, addr, &v, sizeof(T)); if (ok) *ok = r; return v;
}
template <typename T> bool writeValue(pid_t pid, uintptr_t addr, T val) {
    return writeMem(pid, addr, &val, sizeof(T));
}

// ---- module base -----------------------------------------------------------
uintptr_t moduleBase(pid_t pid, const char* libName, bool* ok);

// ---- pattern scan ----------------------------------------------------------
uintptr_t patternScan(pid_t pid, uintptr_t start, uintptr_t end, const char* pattern);

// ---- offset table ----------------------------------------------------------
struct OffsetTable {
    char version[64];
    uintptr_t entityList;
    uintptr_t entityCount;
    uintptr_t entityStride;
    uintptr_t posX;
    uintptr_t posY;
    uintptr_t posZ;
    uintptr_t team;
    uintptr_t health;
    uintptr_t heroId;
    uintptr_t camera;
    uintptr_t aimVector;
    uintptr_t localPlayer;
    uintptr_t lobbyHandler;
    bool valid() const;
};
bool loadOffsets(const char* jsonPath, OffsetTable* out);
const char* offsetsPath();
bool versionMatches(const OffsetTable& t, const char* gameVersion);

} // namespace astra