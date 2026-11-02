# ðŸŽ® Astra â€” MLBB Injector

![Android](https://img.shields.io/badge/Platform-Android-green?logo=android&logoColor=white)
![Language](https://img.shields.io/badge/Language-Java%20%7C%20C%2B%2B17-blue?logo=java&logoColor=white)
![NDK](https://img.shields.io/badge/NDK-25.1.8937393-orange?logo=androidstudio&logoColor=white)
![Build](https://img.shields.io/badge/Build-Gradle%208.12.1-brightgreen?logo=gradle&logoColor=white)
![MinSDK](https://img.shields.io/badge/Min%20SDK-26%20(Android%208.0)-lightgrey)
![ABI](https://img.shields.io/badge/ABI-arm64--v8a-red)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## ðŸ“– Introduction

**Astra** is an Android overlay injector built for **Mobile Legends: Bang Bang (MLBB)**. It attaches to the game process and provides a floating overlay with ESP, aimbot, projectile prediction, and lobby intelligence â€” all running on top of the game without modifying the game's APK.

The project is built with a **Kotlin/Java shell** for the Android UI and a **C++ native core** (`libcore.so`) via JNI for high-performance memory access and injection. It supports both **rooted** and **rootless** (virtual-space container) execution modes.

---

## ðŸ§  How It Was Built

### Architecture Overview

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                  Android App (Java)              â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”‚
â”‚  â”‚ MainActivityâ”‚  â”‚ OverlayService â”‚  â”‚ OverlayView â”‚  â”‚
â”‚  â”‚ (UI)      â”‚  â”‚ (Foreground)   â”‚  â”‚ (Canvas)    â”‚  â”‚
â”‚  â””â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”˜  â”‚
â”‚        â”‚              â”‚               â”‚          â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”  â”‚
â”‚  â”‚            NativeBridge (JNI)             â”‚  â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                      â”‚ JNI
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚              libcore.so (C++17)               â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â” â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â” â”Œâ”€â”€â”€â”€â”€â”€â” â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”‚
â”‚  â”‚ Injectorâ”‚ â”‚ Memory â”‚ â”‚ IPC  â”‚ â”‚ Features â”‚  â”‚
â”‚  â”‚        â”‚ â”‚        â”‚ â”‚      â”‚ â”‚          â”‚  â”‚
â”‚  â”‚ attach â”‚ â”‚ readMemâ”‚ â”‚RingBufâ”‚ â”‚ ESP      â”‚  â”‚
â”‚  â”‚ inject â”‚ â”‚ writeMâ”‚ â”‚      â”‚ â”‚ Aimbot   â”‚  â”‚
â”‚  â”‚ ptrace â”‚ â”‚ offsetsâ”‚ â”‚      â”‚ â”‚ Predict  â”‚  â”‚
â”‚  â”‚ dlopen â”‚ â”‚ patternâ”‚ â”‚      â”‚ â”‚ Lobby    â”‚  â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”˜ â””â”€â”€â”€â”€â”€â”€â”€â”€â”˜ â””â”€â”€â”€â”€â”€â”€â”˜ â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”                               â”‚
â”‚  â”‚ Anti-Detect â”‚                              â”‚
â”‚  â”‚ thread spoofâ”‚                             â”‚
â”‚  â”‚ delayed attachâ”‚                           â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜                              â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

### Step-by-Step Build Process

1. **Project Scaffolding** â€” Created a Gradle project with two modules: `:app` (Android app) and `:core` (native library). Configured NDK with arm64-v8a ABI and min SDK 26.

2. **Environment Detection** â€” Java `Environment` class probes for root (`su`) and detects virtual-space containers (VMOS, Parallel Space, VirtualApp) by checking known paths.

3. **Native Injector Core (C++)** â€” Implemented process discovery via `/proc`, ptrace attach, and `/proc/<pid>/mem` read/write primitives. Added module base resolution by parsing `/proc/<pid>/maps`.

4. **Memory & Offset System** â€” Built a runtime JSON offset loader with version gating, plus a pattern scanner for signature-based offset discovery across game updates.

5. **Shared-Memory IPC** â€” Created a mmap-backed ring buffer with seqlock for publishing ESP entity data from the native core to the Java overlay renderer.

6. **Feature Modules**
   - **ESP** â€” Reads entity list from game memory, extracts positions/health/team/hero, publishes via ring buffer
   - **Aimbot** â€” FOV-cone target selection, smoothing, aim-vector write with sanity checks
   - **Prediction** â€” Iterative projectile lead calculation based on target velocity and projectile speed
   - **Lobby Hook** â€” Handler validation for in-process protobuf interception

7. **Floating Overlay** â€” Foreground service with `WindowManager` floating view, canvas-based ESP rendering, lobby info panel, and touch-toggle controls.

8. **Anti-Detection** â€” Thread-name spoofing, delayed attach (2.5s after lobby), randomized build signatures, and `/proc` scanning restricted to injector process only.

9. **In-Process Injection** â€” arm64 ptrace + dlopen shellcode injection for lobby hooks inside the game process.

### Build Toolchain
- **Gradle 8.12.1** with AGP 8.3.2
- **Android NDK 25.1.8937393** (clang++, c++_shared STL)
- **CMake 3.22.1** with Ninja build system
- **Java 17** for the app shell
- **C++17** for the native core

---

## ðŸ“‚ Project Structure

```
astra/
â”œâ”€â”€ app/
â”‚   â”œâ”€â”€ src/main/
â”‚   â”‚   â”œâ”€â”€ java/com/astra/injector/
â”‚   â”‚   â”‚   â”œâ”€â”€ MainActivity.java          # Permission & service controls
â”‚   â”‚   â”‚   â”œâ”€â”€ OverlayService.java        # Foreground service + overlay
â”‚   â”‚   â”‚   â”œâ”€â”€ OverlayView.java           # Canvas ESP + lobby renderer
â”‚   â”‚   â”‚   â”œâ”€â”€ NativeBridge.java          # JNI bridge
â”‚   â”‚   â”‚   â”œâ”€â”€ Entity.java               # ESP entity data
â”‚   â”‚   â”‚   â”œâ”€â”€ LobbyEntry.java           # Lobby info data
â”‚   â”‚   â”‚   â”œâ”€â”€ AstraApp.java             # Application class
â”‚   â”‚   â”‚   â””â”€â”€ env/
â”‚   â”‚   â”‚       â””â”€â”€ Environment.java      # Root/container detection
â”‚   â”‚   â”œâ”€â”€ res/
â”‚   â”‚   â””â”€â”€ assets/offsets/default.json   # Version-gated offset table
â”‚   â””â”€â”€ build.gradle.kts
â”œâ”€â”€ core/
â”‚   â”œâ”€â”€ src/main/cpp/
â”‚   â”‚   â”œâ”€â”€ CMakeLists.txt
â”‚   â”‚   â”œâ”€â”€ core.cpp                      # JNI init
â”‚   â”‚   â”œâ”€â”€ injector.cpp                  # Process, attach, memory I/O, offsets
â”‚   â”‚   â”œâ”€â”€ features.cpp                  # ESP, aimbot, prediction, lobby
â”‚   â”‚   â”œâ”€â”€ ipc.cpp                       # Ring buffer
â”‚   â”‚   â”œâ”€â”€ inject.cpp                    # In-process injection
â”‚   â”‚   â”œâ”€â”€ anticheat.cpp                 # Anti-detection
â”‚   â”‚   â”œâ”€â”€ jni_bridge.cpp                # JNI exports
â”‚   â”‚   â””â”€â”€ include/
â”‚   â”‚       â”œâ”€â”€ injector.h
â”‚   â”‚       â”œâ”€â”€ features.h
â”‚   â”‚       â”œâ”€â”€ ipc.h
â”‚   â”‚       â””â”€â”€ anticheat.h
â”‚   â””â”€â”€ build.gradle.kts
â”œâ”€â”€ build.gradle.kts
â”œâ”€â”€ settings.gradle.kts
â””â”€â”€ .gitignore
```

---

## âš™ï¸ How the App Works

1. **Launch** â†’ `MainActivity` requests overlay permission and starts `OverlayService`
2. **Environment Check** â†’ Detects root or virtual-space container; refuses if neither
3. **Native Init** â†’ Loads `libcore.so`, opens shared-memory ring buffer
4. **Offset Load** â†’ Reads `offsets.json` from assets, validates game version
5. **Process Attach** â†’ Finds MLBB process, applies anti-detection delay, attaches via ptrace
6. **Module Resolution** â€” Locates `libunity.so` / `libil2cpp.so` base address
7. **Feature Loop** (~60fps) â†’ ESP reads entities â†’ publishes to ring buffer â†’ overlay renders boxes; aimbot selects target â†’ writes aim vector
8. **Overlay Rendering** â€” Floating canvas draws ESP boxes, health bars, and lobby info panel
9. **Cleanup** â†’ On destroy, stops native loop, detaches from game, closes ring buffer

---

## ðŸ”§ Building from Source

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

## âš ï¸ Notes

- Offsets in `assets/offsets/default.json` are **placeholders** â€” real offsets must be obtained via reverse engineering for the target MLBB version
- Lobby info requires in-process injection (payload `.so`) which needs to be built separately
- Rootless mode requires injector and game to run in the same virtual-space container
- Anti-detection is **middle-level** â€” avoids obvious signatures but is not invisible to advanced anti-cheat

---

## ðŸ“œ License

MIT License â€” free to use, modify, and distribute.
