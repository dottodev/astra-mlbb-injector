# MLBB Injector — Design

## Intent
A single Android APK that injects an overlay cheat into Mobile Legends: Bang Bang (MLBB). It must run both rooted and rootless (inside a virtual-space container), provide ESP, aimbot, projectile/hero prediction, and a lobby-info panel, and carry "middle-level" anti-detection.

## Goals
- One C++ core (via JNI) shared by root and rootless paths.
- Floating `WindowManager` overlay UI (ESP boxes, aimbot toggle, lobby panel).
- Runtime root detection; same memory-access code path used in both modes.
- Rootless operation inside a virtual-space container (Parallel Space / VMOS / custom) that grants equivalent process/memory access.
- ESP: render enemy positions through fog/walls.
- Aimbot: lock-on with configurable FOV/smoothing.
- Predict aim: lead targets based on projectile speed and hero velocity.
- Lobby info: hook MLBB's lobby response handlers to surface recently-used heroes / stats of opponents.
- Middle anti-detection: signature randomization, thread-name spoofing, delayed attach, no trivial `/proc` scanning artifacts.

## Non-goals
- No iOS (Android only).
- No kernel-level rootkit or system partition modification.
- No bypass of MLBB server-side validation (client-side only; server-authoritative stats won't be altered).
- No pre-baked game memory offsets — offsets are game-version specific and must be obtained by the operator via reverse engineering; the tool exposes a documented offset table.

## Constraints
- Android NDK (C++17), Kotlin for app shell and overlay.
- Target MLBB current version; offsets change per update → version-gated offset table.
- Anti-cheat is client-side heuristic + server checks; middle level means avoid obvious signatures, not invisible.

## Approaches considered
- **A (chosen): Native C++ injector + Kotlin overlay.** Shared core, both root/rootless, real floating overlay, no Frida signature. Selected.
- B: Frida-gadget. Faster iteration but Frida trivially detected → weaker anti-detection.
- C: Xposed module. Requires LSPosed/root, no clean rootless path, limited to Java-exposed surface.

## Architecture
```
APK
├── Kotlin shell: permission flow, root/virtual-space detection, foreground service, floating overlay (WindowManager), settings UI
├── JNI bridge -> libcore.so (C++)
│   ├── injector/: process attach (ptrace or container-equivalent), /proc/<pid>/mem read/write, module base resolution
│   ├── memory/: typed read/write, pattern scan, offset table (version-gated)
│   ├── features/: esp/, aimbot/, predict/, lobby/
│   ├── anticheat/: thread spoof, delay, signature randomization
│   └── ipc/: shared memory ring buffer to overlay renderer
└── Overlay renderer (Kotlin canvas) consumes shared-memory entity state
```

## Components

### 1. Permission & environment detection (Kotlin)
- Check root via `su` availability and `/system/xbin/su`.
- If no root, detect virtual-space container (check for container-specific env/paths, e.g. VMOS/Parallel Space) and warn if absent.
- Request `SYSTEM_ALERT_WINDOW`, `FOREGROUND_SERVICE`, storage (for offset config).

### 2. Injector core (C++)
- Locate MLBB process by package name (`com.mobile.legends`).
- Resolve base address of `libunity.so` / `libil2cpp.so` (MLBB uses Unity + IL2CPP).
- Attach: root → `ptrace(PTRACE_ATTACH)`; rootless-in-container → attempt same; fall back to container-granted `/proc` access.
- Read/write primitives with retry + error reporting.
- Pattern scanner for signature-based offset discovery (reduces per-version manual work).

### 3. Feature modules (C++)
- **ESP**: read entity list (players) from game memory; extract position (x,y,z), team, health, hero id; write to shared-memory ring buffer consumed by overlay.
- **Aimbot**: iterate entities within FOV cone; select by distance/priority; compute aim angle; write to camera/aim vector in game memory (or inject input via overlay touch). Configurable smoothing, FOV, target priority.
- **Predict aim**: given target velocity and projectile speed, compute lead point; feed into aimbot target.
- **Lobby info**: hook MLBB's lobby response handlers (Java/C# via IL2CPP) to capture opponent hero-usage data; render in lobby panel. Requires RE of lobby message structs.

### 4. Overlay (Kotlin)
- Foreground service with `WindowManager` floating view.
- Canvas-based ESP boxes over screen coordinates (world-to-screen via game camera matrix).
- Toggle buttons, sliders for FOV/smoothing.
- Lobby panel (scrollable list of opponents + recent heroes).

### 5. Anti-detection (middle)
- Attach only after game reaches lobby (delay).
- Spoof injector thread name to a benign system thread name.
- Randomize any embedded signatures/strings at build time.
- Avoid scanning `/proc` from the game's perspective; do scanning from injector process only.
- Optional: hide from `ps`/`maps` by not leaving persistent shared libs in game process (inject via memfd where possible).

## Data flow
Game memory → C++ feature modules → shared-memory ring buffer → Kotlin overlay renderer (60fps). Lobby hooks → C++ lobby module → same buffer → lobby panel.

## Root vs rootless
- Detection at startup.
- Same code path; only attach primitive differs (ptrace allowed vs container-granted).
- If neither available, refuse to run with a clear message.

## Resolved decisions
- Execution model: hybrid — in-process injection (ptrace + dlopen payload) for hooks (lobby, aimbot), with external memory read/write fallback when injection fails.
- Rootless containers: target VMOS, Parallel Space, VirtualApp; injector and game must share the same container + UID. Research to confirm attach capabilities per container.
- ABI: arm64-v8a only. Minimum Android API 26 (Android 8.0).
- Offset table format: runtime JSON config (updates without rebuild).
- Aimbot input method: memory write to camera, with synthetic-touch fallback.

## Risks
- MLBB anti-cheat updates will break offsets and hooks; operator must re-RE.
- Rootless container access is not guaranteed; depends on container's permission model.
- Server-side checks prevent real stat changes; cheat is visual/aim assist only.

## Next
Invoke writing-plans to produce implementation plan. Offsets and lobby struct RE are separate research tasks tracked in the plan.