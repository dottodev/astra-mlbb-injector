# Astra - MLBB Injector

![Android](https://img.shields.io/badge/Platform-Android-green?logo=android&logoColor=white)
![Language](https://img.shields.io/badge/Language-Java%20%7C%20C%2B%2B17-blue?logo=java&logoColor=white)
![NDK](https://img.shields.io/badge/NDK-25.1.8937393-orange?logo=androidstudio&logoColor=white)
![Build](https://img.shields.io/badge/Build-Gradle%208.12.1-brightgreen?logo=gradle&logoColor=white)
![MinSDK](https://img.shields.io/badge/Min%20SDK-26%20(Android%208.0)-lightgrey)
![ABI](https://img.shields.io/badge/ABI-arm64--v8a-red)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## Introduction

**Astra** is an Android overlay injector built for **Mobile Legends: Bang Bang (MLBB)**. It attaches to the game process and provides a floating overlay with ESP, aimbot, projectile prediction, and lobby intelligence -- all running on top of the game without modifying the game's APK.

The project is built with a **Java shell** for the Android UI and a **C++ native core** (libcore.so) via JNI for high-performance memory access and injection. It supports both **rooted** and **rootless** (virtual-space container) execution modes.

---

## How It Was Built

### Architecture Overview

```
+---------------------------------------------------+
|                  Android App (Java)                |
|  +-----------+  +---------------+  +-------------+ |
|  | MainActivity|  | OverlayService |  | OverlayView | |
|  | (UI)      |  | (Foreground)   |  | (Canvas)    | |
|  +-----+-----+  +-------+-------+  +------+-----+ |
|        |              |               |            |
|  +-----+--------------+---------------+-----+     |
|  |            NativeBridge (JNI)             |     |
|  +--------------------+--------------------+     |
+-----------------------+---------------------------+
                        | JNI
+-----------------------+---------------------------+
|              libcore.so (C++17)                    |
|  +--------+ +--------+ +------+ +----------+      |
|  | Injector| | Memory | | IPC  | | Features |      |
|  |        | |        | |      | |          |      |
|  | attach | | readMem| |RingBuf| | ESP      |      |
|  | inject | | writeM | |      | | Aimbot   |      |
|  | ptrace | | offsets| |      | | Predict  |      |
|  | dlopen | | pattern| |      | | Lobby    |      |
|  +--------+ +--------+ +------+ +----------+      |
|  +-------------+                                  |
|  | Anti-Detect |                                  |
|  | thread spoof|                                  |
|  | delayed attach|                                |
|  +-------------+                                  |
+---------------------------------------------------+
```

### Step-by-Step Build Process

1. **Project Scaffolding** -- Created a Gradle project with two modules: `:app` (Android app) and `:core` (native library). Configured NDK with arm64-v8a ABI and min SDK 26.

2. **Environment Detection** -- Java `Environment` class probes for root (`su`) and detects virtual-space containers (VMOS, Parallel Space, VirtualApp) by checking known paths.

3. **Native Injector Core (C++)** -- Implemented process discovery via `/proc`, ptrace attach, and `/proc/<pid>/mem` read/write primitives. Added module base resolution by parsing `/proc/<pid>/maps`.

4. **Memory & Offset System** -- Built a runtime JSON offset loader with version gating, plus a pattern scanner for signature-based offset discovery across game updates.

5. **Shared-Memory IPC** -- Created a mmap-backed ring buffer with seqlock for publishing ESP entity data from the native core to the Java overlay renderer.

6. **Feature Modules**
   - **ESP** -- Reads entity list from game memory, extracts positions/health/team/hero, publishes via ring buffer
   - **Aimbot** -- FOV-cone target selection, smoothing, aim-vector write with sanity checks
   - **Prediction** -- Iterative projectile lead calculation based on target velocity and projectile speed
   - **Lobby Hook** -- Handler validation for in-process protobuf interception

7. **Floating Overlay** -- Foreground service with `WindowManager` floating view, canvas-based ESP rendering, lobby info panel, and touch-toggle controls.

8. **Anti-Detection** -- Thread-name spoofing, delayed attach (2.5s after lobby), randomized build signatures, and `/proc` scanning restricted to injector process only.

9. **In-Process Injection** -- arm64 ptrace + dlopen shellcode injection for lobby hooks inside the game process.

### Build Toolchain
- **Gradle 8.12.1** with AGP 8.3.2
- **Android NDK 25.1.8937393** (clang++, c++_shared STL)
- **CMake 3.22.1** with Ninja build system
- **Java 17** for the app shell
- **C++17** for the native core

---

## Project Structure

```
astra/
  app/
    src/main/
      java/com/astra/injector/
        MainActivity.java          # Permission & service controls
        OverlayService.java        # Foreground service + overlay
        OverlayView.java           # Canvas ESP + lobby renderer
        NativeBridge.java          # JNI bridge
        Entity.java               # ESP entity data
        LobbyEntry.java           # Lobby info data
        AstraApp.java             # Application class
        env/
          Environment.java      # Root/container detection
      res/
      assets/offsets/default.json   # Version-gated offset table
    build.gradle.kts
  core/
    src/main/cpp/
      CMakeLists.txt
      core.cpp                      # JNI init
      injector.cpp                  # Process, attach, memory I/O, offsets
      features.cpp                  # ESP, aimbot, prediction, lobby
      ipc.cpp                       # Ring buffer
      inject.cpp                    # In-process injection
      anticheat.cpp                 # Anti-detection
      jni_bridge.cpp                # JNI exports
      include/
        injector.h
        features.h
        ipc.h
        anticheat.h
    build.gradle.kts
  build.gradle.kts
  settings.gradle.kts
  .gitignore
```

---

## How the App Works

1. **Launch** - `MainActivity` requests overlay permission and starts `OverlayService`
2. **Environment Check** - Detects root or virtual-space container; refuses if neither
3. **Native Init** - Loads `libcore.so`, opens shared-memory ring buffer
4. **Offset Load** - Reads `offsets.json` from assets, validates game version
5. **Process Attach** - Finds MLBB process, applies anti-detection delay, attaches via ptrace
6. **Module Resolution** - Locates `libunity.so` / `libil2cpp.so` base address
7. **Feature Loop** (~60fps) - ESP reads entities -> publishes to ring buffer -> overlay renders boxes; aimbot selects target -> writes aim vector
8. **Overlay Rendering** - Floating canvas draws ESP boxes, health bars, and lobby info panel
9. **Cleanup** - On destroy, stops native loop, detaches from game, closes ring buffer

---

## Building from Source

```bash
# Clone
git clone https://github.com/dottodev/astra-mlbb-injector.git
cd astra-mlbb-injector

# Build debug APK
./gradlew assembleDebug

# Output: app/build/outputs/apk/debug/app-debug.apk
```

**Requirements:**
- Android Studio with NDK 25.1.x
- JDK 17
- Android SDK (platform 34, build-tools)

---

## Notes

- Offsets in `assets/offsets/default.json` are **placeholders** -- real offsets must be obtained via reverse engineering for the target MLBB version
- Lobby info requires in-process injection (payload `.so`) which needs to be built separately
- Rootless mode requires injector and game to run in the same virtual-space container
- Anti-detection is **middle-level** -- avoids obvious signatures but is not invisible to advanced anti-cheat

---

## License

MIT License - free to use, modify, and distribute. See LICENSE file.