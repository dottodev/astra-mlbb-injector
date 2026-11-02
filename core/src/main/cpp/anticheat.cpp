#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "astra", __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) printf(__VA_ARGS__)
#endif

// Delayed attach: core module refuses to attach until this many ms after
// the game process is seen in lobby state.
static int g_attachDelayMs = 1500;
static int g_packageMinRuntimeMs = 2500;

static const char* kBenignThreadNames[] = {
    "pool-1-thread-1", "queued-work-looper", "binder:1001_2",
    "GlyphsClockwork", "perf-hint", "CodeCacheWorker"
};

void setBenignThreadName() {
    pthread_setname_np(pthread_self(), kBenignThreadNames[0]);
}

int randomInt(int lo, int hi) {
    static int seeded = 0;
    if (!seeded) { srand((unsigned)time(nullptr)); seeded = 1; }
    return lo + rand() % (hi - lo + 1);
}

void randomTag(char* out, int len) {
    static const char* c = "0123456789abcdef";
    for (int i = 0; i < len; ++i) out[i] = c[randomInt(0, 15)];
    out[len] = 0;
}

namespace astra { void applyAntiDetection() {
    setBenignThreadName();
    usleep(g_attachDelayMs);
} }