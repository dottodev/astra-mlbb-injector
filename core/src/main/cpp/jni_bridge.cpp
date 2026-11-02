#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "injector.h"
#include "features.h"
#include "ipc.h"
#include "anticheat.h"

#define LOG_TAG "astra_jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace astra {
static OffsetTable g_offsets;
static RingBuffer g_ring;
static int g_pid = -1;
static uintptr_t g_base = 0;
static volatile int g_running = 0;
static pthread_t g_loop;
static char g_gameVersion[64] = {0};

extern "C" JNIEXPORT void JNICALL
Java_com_astra_injector_NativeBridge_init(JNIEnv* env, jclass) {
    LOGI("core init");
    g_ring.open("/data/data/com.astra.injector/files/ring");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_astra_injector_NativeBridge_start(JNIEnv* env, jclass,
    jstring pkg, jstring version) {
    if (g_running) return JNI_TRUE;
    const char* p = env->GetStringUTFChars(pkg, nullptr);
    const char* v = env->GetStringUTFChars(version, nullptr);
    strncpy(g_gameVersion, v, 63);
    env->ReleaseStringUTFChars(pkg, p);
    env->ReleaseStringUTFChars(version, v);

    if (!loadOffsets(offsetsPath(), &g_offsets) || !versionMatches(g_offsets, g_gameVersion)) {
        LOGI("offsets missing or version mismatch");
        return JNI_FALSE;
    }

    g_pid = findProcess(p);
    if (g_pid <= 0) { LOGI("game process not found"); return JNI_FALSE; }

    applyAntiDetection();
    if (!attach(g_pid)) { LOGI("attach failed"); return JNI_FALSE; }

    bool ok = false;
    g_base = moduleBase(g_pid, "libunity.so", &ok);
    if (!ok) g_base = moduleBase(g_pid, "libil2cpp.so", &ok);
    if (!ok) { LOGI("module not found"); return JNI_FALSE; }

    g_running = 1;
    pthread_create(&g_loop, nullptr, [](void* arg) -> void* {
        AimbotConfig cfg{30.f, 0.15f, 0};
        while (g_running) {
            updateESP(g_pid, g_base, &g_offsets, g_ring);
            updateAimbot(g_pid, g_base, &g_offsets, cfg);
            usleep(16000);
        }
        return nullptr;
    }, nullptr);
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_astra_injector_NativeBridge_stop(JNIEnv*, jclass) {
    g_running = 0;
    if (g_loop) pthread_join(g_loop, nullptr);
    if (g_pid > 0) detach(g_pid);
    g_ring.close();
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_astra_injector_NativeBridge_getFrame(JNIEnv* env, jclass) {
    SharedFrame f;
    if (!g_ring.latest(f)) return nullptr;
    jclass eCls = env->FindClass("com/astra/injector/Entity");
    jmethodID ctor = env->GetMethodID(eCls, "<init>", "(FFFFII)V");
    jobjectArray arr = env->NewObjectArray(f.count, eCls, nullptr);
    for (int i = 0; i < f.count; ++i) {
        Entity& e = f.entities[i];
        jobject obj = env->NewObject(eCls, ctor, e.screenX, e.screenY, e.health, e.team, e.heroId, e.valid);
        env->SetObjectArrayElement(arr, i, obj);
    }
    return arr;
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_astra_injector_NativeBridge_getLobby(JNIEnv* env, jclass) {
    LobbyState s = getLobbyState();
    jclass cls = env->FindClass("com/astra/injector/LobbyEntry");
    jmethodID ctor = env->GetMethodID(cls, "<init>", "(IILjava/lang/String;)V");
    jobjectArray arr = env->NewObjectArray(s.count, cls, nullptr);
    for (int i = 0; i < s.count; ++i) {
        LobbyEntry& e = s.entries[i];
        jstring name = env->NewStringUTF(e.name);
        jobject obj = env->NewObject(cls, ctor, e.heroId, e.winRate, name);
        env->SetObjectArrayElement(arr, i, obj);
    }
    return arr;
}
} // namespace astra