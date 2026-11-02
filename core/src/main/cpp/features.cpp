#include "features.h"
#include "injector.h"
#include "ipc.h"
#include <math.h>
#include <string.h>
#include <pthread.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "astra", __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) printf(__VA_ARGS__)
#endif

namespace astra {

// ------------------------- world -> screen -------------------------------------
struct Mat4 { float m[16]; };

static void multiply(const Mat4& a, const Mat4& b, Mat4& out) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += a.m[r*4+k]*b.m[k*4+c];
            out.m[r*4+c] = s;
        }
}

static bool worldToScreen(const Mat4& viewProj, const Vec3& world, float& sx, float& sy) {
    float clip[4] = {
        viewProj.m[0]*world.x + viewProj.m[4]*world.y + viewProj.m[ 8]*world.z + viewProj.m[12],
        viewProj.m[1]*world.x + viewProj.m[5]*world.y + viewProj.m[ 9]*world.z + viewProj.m[13],
        viewProj.m[2]*world.x + viewProj.m[6]*world.y + viewProj.m[10]*world.z + viewProj.m[14],
        viewProj.m[3]*world.x + viewProj.m[7]*world.y + viewProj.m[11]*world.z + viewProj.m[15],
    };
    if (clip[3] < 0.1f) return false;
    float ndcX = clip[0]/clip[3];
    float ndcY = clip[1]/clip[3];
    sx = (ndcX + 1.f) * 0.5f;
    sy = (1.f - ndcY) * 0.5f;
    return true;
}

static inline float minf(float a, float b) { return a < b ? a : b; }
static inline float maxf(float a, float b) { return a > b ? a : b; }
static inline int minu(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }

// ------------------------- ESP ---------------------------------------------------
void updateESP(int pid, uintptr_t moduleBase, const void* offsetsRaw, RingBuffer& ring) {
    const OffsetTable* t = reinterpret_cast<const OffsetTable*>(offsetsRaw);
    if (!t->valid()) return;
    SharedFrame frame; frame.count = 0; frame.deltaTime = 0;

    bool ok = false;
    uint8_t count = readValue<uint8_t>(pid, moduleBase + t->entityCount, &ok);
    if (!ok || count == 0 || count > ENTITY_CAP) count = 8;
    count = (uint8_t)minu(count, ENTITY_CAP);

    uintptr_t listBase = moduleBase + t->entityList;
    uintptr_t list = readValue<uintptr_t>(pid, listBase, &ok);
    if (!ok || list < 0x10000) list = listBase;

    Mat4 vp{};
    ok = readMem(pid, moduleBase + t->camera, vp.m, sizeof(vp.m));

    int localTeam = -1;
    if (t->localPlayer) {
        uintptr_t lp = readValue<uintptr_t>(pid, moduleBase + t->localPlayer, &ok);
        if (ok && lp > 0x10000) localTeam = readValue<int32_t>(pid, lp + t->team, &ok);
    }

    for (uint8_t i = 0; i < count && frame.count < ENTITY_CAP; ++i) {
        uintptr_t ent = ok ? readValue<uintptr_t>(pid, list + i * 8, &ok) : 0;
        if (!ok || ent < 0x10000) continue;
        Vec3 pos{};
        ok = readMem(pid, ent + t->posX, &pos.x, 4)
          && readMem(pid, ent + t->posY, &pos.y, 4)
          && readMem(pid, ent + t->posZ, &pos.z, 4);
        if (!ok) continue;
        int team = readValue<int32_t>(pid, ent + t->team, &ok);
        float hp = readValue<float>(pid, ent + t->health, &ok);
        int hero = readValue<int32_t>(pid, ent + t->heroId, &ok);
        float sx, sy;
        Entity& e = frame.entities[frame.count];
        (void)worldToScreen(vp, pos, sx, sy);
        e.x = pos.x; e.y = pos.y; e.z = pos.z; e.screenX = sx; e.screenY = sy;
        e.health = hp; e.team = team; e.heroId = hero; e.valid = 1;
        (void)localTeam;
        ++frame.count;
    }
    frame.seq++;
    ring.publish(frame);
}

// ------------------------- aimbot + prediction ----------------------------------
Vec3 predictLead(const Vec3& targetPos, const Vec3& targetVel, float projectileSpeed) {
    Vec3 out = targetPos;
    if (projectileSpeed <= 0.01f) return out;
    float dist = sqrtf(targetPos.x*targetPos.x + targetPos.y*targetPos.y + targetPos.z*targetPos.z);
    float time = dist / projectileSpeed;
    for (int i = 0; i < 3; ++i) {
        float px = targetPos.x + targetVel.x * time;
        float py = targetPos.y + targetVel.y * time;
        float pz = targetPos.z + targetVel.z * time;
        time = sqrtf(px*px + py*py + pz*pz) / projectileSpeed;
    }
    out.x = targetPos.x + targetVel.x * time;
    out.y = targetPos.y + targetVel.y * time;
    out.z = targetPos.z + targetVel.z * time;
    return out;
}

static float g_lastYaw = 0, g_lastPitch = 0;

void updateAimbot(int pid, uintptr_t moduleBase, const void* offsetsRaw, const AimbotConfig& cfg) {
    const OffsetTable* t = reinterpret_cast<const OffsetTable*>(offsetsRaw);
    if (!t->valid() || !t->aimVector) return;
    Vec3 aim{};
    if (!readMem(pid, moduleBase + t->aimVector, &aim, sizeof(aim))) return;

    uintptr_t listBase = moduleBase + t->entityList;
    bool ok = false;
    uint8_t count = readValue<uint8_t>(pid, moduleBase + t->entityCount, &ok);
    if (!ok || count == 0 || count > ENTITY_CAP) count = 8;
    count = (uint8_t)minu(count, ENTITY_CAP);
    uintptr_t list = readValue<uintptr_t>(pid, listBase, &ok);
    if (!ok || list < 0x10000) list = listBase;

    float bestScore = -2.f;
    Vec3 bestTarget{};
    for (uint8_t i = 0; i < count; ++i) {
        uintptr_t ent = readValue<uintptr_t>(pid, list + i*8, &ok);
        if (!ok || ent < 0x10000) continue;
        Vec3 pos{};
        if (!readMem(pid, ent + t->posX, &pos.x, 4) ||
            !readMem(pid, ent + t->posY, &pos.y, 4) ||
            !readMem(pid, ent + t->posZ, &pos.z, 4)) continue;
        Vec3 dir{ pos.x - aim.x, pos.y - aim.y, pos.z - aim.z };
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if (len < 1e-3f) continue;
        dir.x/=len; dir.y/=len; dir.z/=len;
        float aimLen = sqrtf(aim.x*aim.x+aim.y*aim.y+aim.z*aim.z)+1e-3f;
        float dot = (dir.x*aim.x+dir.y*aim.y+dir.z*aim.z)/aimLen;
        if (dot > 1.f) dot = 1.f; if (dot < -1.f) dot = -1.f;
        float angle = acosf(dot) * 180.f/M_PI;
        if (angle > cfg.fovDegrees) continue;
        float score = cfg.targetPriority == 0 ? -angle : 1.f/len;
        if (score > bestScore) { bestScore = score; bestTarget = pos; }
    }
    if (bestScore == -2.f) return;

    float yaw = atan2f(bestTarget.y, bestTarget.x);
    float pitch = asinf(bestTarget.z / (sqrtf(bestTarget.x*bestTarget.x + bestTarget.z*bestTarget.z)+1e-3f));
    float s = maxf(0.f, minf(1.f, cfg.smoothing));
    yaw = g_lastYaw * s + yaw * (1.f - s);
    pitch = g_lastPitch * s + pitch * (1.f - s);
    g_lastYaw = yaw; g_lastPitch = pitch;

    Vec3 out{};
    float cp = cosf(pitch);
    out.x = cp * cosf(yaw); out.y = cp * sinf(yaw); out.z = sinf(pitch);
    // sanity: reject NaN / degenerate vectors before writing
    if (out.x == out.x && out.y == out.y && out.z == out.z) // NaN check
        writeMem(pid, moduleBase + t->aimVector, &out, sizeof(out));
}

// ------------------------- lobby hook ------------------------------------------
static LobbyState g_lobby;
static pthread_mutex_t g_lobbyMu = PTHREAD_MUTEX_INITIALIZER;

void installLobbyHook(int pid, uintptr_t moduleBase, const void* offsetsRaw) {
    const OffsetTable* t = reinterpret_cast<const OffsetTable*>(offsetsRaw);
    (void)pid; (void)moduleBase;
    if (!t->lobbyHandler) LOGI("lobby hook: no handler offset; skipping\n");
}

LobbyState getLobbyState() {
    LobbyState copy;
    pthread_mutex_lock(&g_lobbyMu);
    memcpy(&copy, &g_lobby, sizeof(LobbyState));
    pthread_mutex_unlock(&g_lobbyMu);
    return copy;
}

} // namespace astra