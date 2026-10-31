# MLBB Injector Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an Android APK (arm64-v8a, API 26+) that injects an overlay cheat into MLBB, running rooted or rootless inside a virtual-space container, providing ESP, aimbot, prediction, and lobby info, with middle-level anti-detection.

**Architecture:** Kotlin shell (permissions, root/container detection, foreground service, floating overlay) + `libcore.so` C++ core (injector, memory, features, anti-detection) bridged via JNI. In-process injection for hooks; external memory R/W fallback. Version-gated JSON offset table.

**Tech Stack:** Kotlin, Android NDK (C++17), CMake, JNI, Google Test (host), Gradle.

**Spec:** `docs/superpowers/specs/2026-10-31-mlbb-injector-design.md`

## Global Constraints
- ABI: arm64-v8a only. Min SDK 26.
- Language: Kotlin (shell), C++17 (core).
- No pre-baked game offsets; offsets live in runtime JSON.
- Anti-detection: thread-name spoof, delayed attach, randomized build signatures.
- Rootless requires injector + game in same virtual-space container.

## Review Focus
- Rootless attach silently failing and reporting success → must verify access before claiming ready.
- Stale offset table causing wrong memory reads → version check + graceful disable.
- Overlay drawn outside game coordinate space → world-to-screen must use game camera matrix.
- Aimbot writing invalid aim vectors → bounds/sanity checks before write.
- Lobby hook capturing partial/corrupt protobuf → validate before render.

---

### Task 1: Project scaffolding

**Files:**
- Create: `settings.gradle.kts`, `build.gradle.kts`, `app/build.gradle.kts`, `app/src/main/AndroidManifest.xml`
- Create: `core/CMakeLists.txt`, `core/src/main/cpp/CMakeLists.txt`
- Create: `core/src/main/cpp/include/core.h`
- Test: (none yet; build verification)

**Interfaces:**
- Produces: Gradle module `:app` and `:core` (native lib `libcore.so`), JNI entry `Java_com_astra_injector_NativeBridge_init`.

- [ ] **Step 1: Create Gradle project with NDK module.** Configure `minSdk 26`, `ndk { abiFilters "arm64-v8a" }`, CMake path.
- [ ] **Step 2: Add empty `libcore.so` exporting `init()`.** Verify build: `./gradlew assembleDebug` succeeds.
- [ ] **Step 3: Commit.**

### Task 2: Root / container detection (Kotlin)

**Files:**
- Create: `app/src/main/java/com/astra/injector/env/Environment.kt`
- Test: `app/src/test/java/com/astra/injector/env/EnvironmentTest.kt`

**Interfaces:**
- Produces: `fun detect(): EnvResult` where `EnvResult = { root: Boolean, container: ContainerType?, canAttach: Boolean }`.
- Consumes: (none)

- [ ] **Step 1: Write failing test** asserting `detect()` returns `root=false` on a no-root emulator mock.
- [ ] **Step 2: Run test; expect fail.**
- [ ] **Step 3: Implement detection:** root via `su` probe; container via known env paths (VMOS, Parallel Space, VirtualApp); set `canAttach` accordingly.
- [ ] **Step 4: Run tests; pass.**
- [ ] **Step 5: Commit.**

### Task 3: Injector core (C++)

**Files:**
- Create: `core/src/main/cpp/injector/process.cpp`, `injector/attach.cpp`, `injector/mem.cpp`
- Create: `core/src/main/cpp/include/injector.h`
- Test: `core/src/test/cpp/injector_test.cpp`

**Interfaces:**
- Produces: `bool attach(pid_t pid)`; `bool readMem(pid_t, addr, buf, len)`; `bool writeMem(...)`; `pid_t findProcess(const char* name)`.
- Consumes: (none)

- [ ] **Step 1: Write failing gtest** for `readMem` on a known `/proc/self/mem` region.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** using `ptrace`/`process_vm_readv` (root) with fallback; expose via JNI.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 4: Offset table + pattern scanner (C++)

**Files:**
- Create: `core/src/main/cpp/memory/offsets.cpp`, `memory/pattern.cpp`
- Create: `app/src/main/assets/offsets/default.json`
- Test: `core/src/test/cpp/offsets_test.cpp`

**Interfaces:**
- Produces: `struct OffsetTable { uint64_t entityList, camera, aimVector, ... }`; `bool loadOffsets(path)`; `uintptr_t patternScan(region, pattern)`.
- Consumes: `readMem` from Task 3.

- [ ] **Step 1: Write failing test** for JSON parse + version gate.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** JSON loader (nlohmann/json) + pattern scanner.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 5: Shared-memory IPC ring buffer

**Files:**
- Create: `core/src/main/cpp/ipc/ringbuf.cpp`, `ipc/ringbuf.h`
- Create: `app/src/main/java/com/astra/injector/ipc/RingBuffer.kt` (mmap wrapper)
- Test: `core/src/test/cpp/ringbuf_test.cpp`

**Interfaces:**
- Produces: `void publishEntity(const Entity&)`; `bool consumeEntities(Entity* out, int max)`.
- Consumes: (none)

- [ ] **Step 1: Write failing test** for publish/consume round-trip.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** mmap'd ring buffer with seqlock.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 6: Floating overlay service (Kotlin)

**Files:**
- Create: `app/src/main/java/com/astra/injector/overlay/OverlayService.kt`
- Create: `app/src/main/java/com/astra/injector/overlay/OverlayView.kt`
- Modify: `AndroidManifest.xml` (foreground service + SYSTEM_ALERT_WINDOW)
- Test: `app/src/test/java/com/astra/injector/overlay/OverlayServiceTest.kt`

**Interfaces:**
- Produces: `OverlayService.start(context)`, `OverlayView.drawEntities(List<Entity>)`.
- Consumes: `consumeEntities` via JNI.

- [ ] **Step 1: Write failing test** for service start/stop lifecycle.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** foreground service + WindowManager floating view + canvas renderer.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 7: ESP feature (C++)

**Files:**
- Create: `core/src/main/cpp/features/esp/esp.cpp`
- Test: `core/src/test/cpp/esp_test.cpp`

**Interfaces:**
- Produces: `void updateESP(OffsetTable)` reading entity list → publishes via ring buffer.
- Consumes: `readMem`, `OffsetTable`, `publishEntity`.

- [ ] **Step 1: Write failing test** with mock memory region containing 2 entities; expect 2 published.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** entity list traversal + world-to-screen matrix extraction.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 8: Aimbot + prediction (C++)

**Files:**
- Create: `core/src/main/cpp/features/aimbot/aimbot.cpp`, `features/aimbot/predict.cpp`
- Test: `core/src/test/cpp/aimbot_test.cpp`

**Interfaces:**
- Produces: `void updateAimbot(OffsetTable, float fov, float smoothing)`; `Vec3 predictLead(target, projectileSpeed)`.
- Consumes: `readMem`, `writeMem`, `OffsetTable`.

- [ ] **Step 1: Write failing test** for `predictLead` with known velocity/speed.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** target selection (FOV cone, priority), lead calculation, aim-vector write with sanity checks.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 9: Lobby info hook (C++)

**Files:**
- Create: `core/src/main/cpp/features/lobby/lobby.cpp`, `lobby/hook.cpp`
- Test: `core/src/test/cpp/lobby_test.cpp`

**Interfaces:**
- Produces: `void installLobbyHook()`; `LobbyState getLobbyState()`.
- Consumes: in-process injection primitives (dlopen/payload) from injector; ring buffer.

- [ ] **Step 1: Write failing test** for protobuf parse of a captured sample lobby message.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** hook installation + protobuf decode + validation.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 10: Anti-detection (C++ + Kotlin)

**Files:**
- Create: `core/src/main/cpp/anticheat/anticheat.cpp`
- Modify: `core/CMakeLists.txt` (build-time string randomization)
- Test: `core/src/test/cpp/anticheat_test.cpp`

**Interfaces:**
- Produces: `void applyAntiDetection()` (thread spoof, delayed attach).
- Consumes: injector attach.

- [ ] **Step 1: Write failing test** asserting thread name spoof applied.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** thread-name spoof, attach delay, signature randomization at build.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 11: In-process injection (C++)

**Files:**
- Create: `core/src/main/cpp/injector/inject.cpp`
- Test: `core/src/test/cpp/inject_test.cpp`

**Interfaces:**
- Produces: `bool injectPayload(pid_t, const char* soPath)` via ptrace+dlopen; `bool isInjected(pid_t)`.
- Consumes: `attach`, `readMem`, `writeMem`.

- [ ] **Step 1: Write failing test** (host mock) for injection state machine.
- [ ] **Step 2: Run; fail.**
- [ ] **Step 3: Implement** ptrace attach → save regs → write dlopen shellcode → restore; verify via `/proc/<pid>/maps`.
- [ ] **Step 4: Run; pass.**
- [ ] **Step 5: Commit.**

### Task 12: End-to-end wiring + build

**Files:**
- Modify: `app/src/main/java/.../MainActivity.kt`, service orchestration
- Create: `docs/usage.md`

- [ ] **Step 1: Wire detection → injector → features → overlay in a single foreground service.**
- [ ] **Step 2: Build debug APK; verify install + launch on emulator.**
- [ ] **Step 3: Commit.**

---

**Self-review:** All spec sections (ESP, aimbot, predict, lobby, anti-detection, root/rootless, overlay) map to tasks 7-10 + 2/3/11. Review Focus inputs have owning tasks. Offsets version-gate in Task 4. Done.