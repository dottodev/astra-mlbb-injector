#pragma once
#include "ipc.h"

namespace astra {
struct Vec3 { float x, y, z; };

// ESP: read entities, world->screen, publish to ring.
void updateESP(int pid, uintptr_t moduleBase, const void* offsetsRaw, RingBuffer& ring);

// Aimbot configuration.
struct AimbotConfig {
    float fovDegrees;
    float smoothing;     // 0..1
    int   targetPriority;
};
void updateAimbot(int pid, uintptr_t moduleBase, const void* offsetsRaw, const AimbotConfig& cfg);

// Prediction: lead point given target pos/velocity and projectile speed.
Vec3 predictLead(const Vec3& targetPos, const Vec3& targetVel, float projectileSpeed);

// Lobby state.
struct LobbyEntry {
    int heroId;
    int winRate;
    char name[32];
    uint8_t valid;
};
struct LobbyState {
    int count;
    LobbyEntry entries[12];
};
void installLobbyHook(int pid, uintptr_t moduleBase, const void* offsetsRaw);
LobbyState getLobbyState();

// Anti detection.
void applyAntiDetection();
} // namespace astra
