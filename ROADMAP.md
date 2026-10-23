# Project roadmap and progress

Percentages are engineering estimates, not additive and not a claim that the project is a finished game.

## Current snapshot

- **Engine/runtime foundations: ~72%** — C++20/CMake, fixed-step loop, logging/config/jobs, platform input/window, Lua mod lifecycle, world data structures, deterministic terrain, simulation systems, versioned saves and a deterministic rigid-body/destruction foundation are substantially implemented. Integration and production hardening remain.
- **Playable game as a whole: ~42%** — the generated voxel world renders through the OpenGL 3.3 core path with GPU VAO/VBO/EBO meshes, GLSL 330 shaders, a free-fly camera and FPS logging. Phase B adds world-space DDA raycasting, sparse edit overrides, batched asynchronous chunk remeshing, and LMB break / RMB place. Phase C persists the world seed, sparse edits (including air overrides), ECS/inventory/work state, civilization state and Lua mod state in versioned saves. Phase D adds a rigid-body foundation, block fracture into debris and debris rendering; inventory UI, complete gameplay and UI remain absent. Nothing is playable as a game yet.
- **Windows portable build: ~55%** — the current revision builds with MSVC, passes CTest, assembles a ZIP and uploads the artifact. A clean-machine launch and visual check is still required before calling the package production-ready.

## Milestones

| Milestone | Status | Estimate | Remaining work |
|---|---|---:|---|
| 1. Bootstrap/runtime | Implemented prototype | 85% | Integration hardening, runtime validation on clean Windows, production diagnostics |
| 2.1 Materials and block registry | Implemented | 90% | Larger-data validation and content pipeline |
| 2.2 CPU SVDAG | Prototype implemented | 70% | Profile large inputs, memory/performance improvements |
| 2.3 Deterministic terrain chunks | Implemented prototype | 70% | Streaming, generation hooks and larger-world tests |
| 3. Simulation and mod systems | Substantial systems implemented | 60% | Broader integration scenarios, balancing, runtime observability |
| 4.1–4.5 Modding, automation, energy/fluids/fields and NPCs | Substantial implementation | 55% | End-to-end scenarios, error recovery, balancing and observability |
| 4.6 / Phase C: world saves | **Complete** — version 2 sparse edits, legacy v1 snapshots, ECS/civilization/mod state, deterministic hash | 100% of this phase | Real-world migration testing and future format evolution |
| 5. Phase A: visible GL voxel world | **Complete** — GLSL renderer, GPU chunk buffers, free-fly camera, FPS log | 30% of the full rendering roadmap | Camera polish, lighting/shadows, streaming and later Vulkan path |
| 5. Phase B: dig and place | **Complete** — DDA, sparse voxel delta, dirty-chunk remeshing and mouse editing | 100% of this phase | Larger-world streaming remains later work |
| 5. Phase C: world saves | **Complete** — v2 sparse edit saves, v1 compatibility, canonical state hash and 10k-edit round trip | 100% of this phase | Longer-duration real-world save/migration tests |
| 5. Phase D: physics and destruction | **Implemented** — body/integrator/broadphase/narrowphase/solver/system, block fracture, per-chunk debris pool, ECS RigidBody persistence and shared-mesh debris rendering | 100% of this phase | Debris recycling, sleep wake on world change, interpolation of tick-spanning motion |
| 6. Gameplay, UI and content | Early / not complete | 10% | Inventory UI, crafting, content and feedback |
| 7. Packaging and release checks | CI package produced; clean-machine launch pending | 55% | Test on a Windows machine without developer tools, bundle/verify runtime dependencies, release checks |

## Phase A verification

- Specification and review: [`specs/01_phase.md`](specs/01_phase.md).
- Linux Debug build: **success**, Ninja, ccache installed, ASan/UBSan enabled.
- Linux CTest: **82/82 passed**. The OpenGL pixel smoke test ran under Xvfb and verified that a rendered test voxel changed a pixel from the clear color.
- Windows MSVC build: **success**.
- Windows CTest: **82/82 passed, 0 failed**. The OpenGL pixel smoke test was explicitly skipped because the hosted runner could not create the required hidden OpenGL context; Linux executed and passed it.
- Windows ZIP artifact: uploaded by [the successful Windows workflow](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39142307082) — [download artifact](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39142307082/artifacts/12142361880).

## Phase B verification

- Specification and review: [`specs/02_phase.md`](specs/02_phase.md).
- DDA tests cover axis hits, negative coordinates, origin-inside-solid, invalid rays, max distance and deterministic edge ties.
- Delta tests cover sparse override lifecycle, explicit air overrides and chunk-local overlay without mutating the base chunk.
- Chunk-manager tests cover negative coordinate mapping, edge-neighbor dirtying, per-tick batching and rejection of stale async mesh results.
- Edit-mode tests cover place → query → break → query, breaking generated voxels into air overrides, misses and invalid placement.
- Linux Debug CTest: **82/82 passed**.
- Windows Release CTest: **82/82 passed**. The first Windows run exposed stack overflow from large chunk temporaries; snapshots were moved to heap storage and test chunks are now constructed directly in map storage. The final Windows run passed and uploaded the portable ZIP.
- The Linux CI also measures a one-file incremental Ninja+ccache rebuild and fails if it takes 20 seconds or longer. The final Linux CI run measured the one-file incremental Ninja+ccache rebuild at **1,538 ms**, below the 20-second limit.

## Phase C verification

- Specification and review: [`specs/03_phase.md`](specs/03_phase.md).
- Version 2 stores seed + sparse voxel edits, ECS/inventory/work state, civilization state and Lua mod state; version 1 chunk snapshots remain readable and are preserved during migration.
- Tests cover sorted delta snapshots, explicit air overrides, duplicate/out-of-range edits, unknown versions, canonical hashing of edits/entities/civilizations/mod state, legacy v1 loading, base-chunk overlay and a 10,000-edit save/load round trip.
- Linux Debug build: **success** with Ninja, ccache and ASan/UBSan; CTest **90/90 passed**. The GL pixel smoke test ran under Xvfb and passed.
- Linux one-file incremental Ninja+ccache rebuild: **1,066 ms**, under the 20-second limit.
- Windows MSVC build: **success**; CTest **90 tests, 0 failures**. The GL pixel smoke test was explicitly skipped because the hosted Windows runner could not create the required OpenGL context; Linux executed it successfully.
- CI for the tested code commit [`ff0e141`](https://github.com/Cr512-netizen/Nuc-lVuc-l/commit/ff0e14104a6852c6f11f456d7dfdb8b7621a2e57): [Linux](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39148558780) · [Windows](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39148558722).
- Windows ZIP artifact: [Nuc-lVuc-l-Windows-x64](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39148558722/artifacts/12144118567). It is a test package, not a finished game; clean-machine launch and visual validation remain pending.

## Phase D verification

- Specification and review: [`specs/04_phase.md`](specs/04_phase.md) — approved 2026-10-18.
- Scope implemented: `physics/body`, `integrator`, `broadphase`, `narrowphase`, `solver`, `system`, `destruction`; `DebrisPool` per-chunk capacity; ECS `RigidBody` stable state traits and version 2 save/restore with transient force cleared; `EditMode` break callback; four-substep `PhysicsSystem` call from the existing 15 Hz tick; shared unit-cube debris mesh in the GL renderer.
- Unit tests present and wired into `nuc_tests`: `test_physics_body`, `test_physics_integrator`, `test_physics_broadphase`, `test_physics_narrowphase`, `test_physics_solver`, `test_physics_system`, `test_destruction`, plus the physics-body save/hash case in `test_world_save` and the debris pixel assertion in `test_gl_smoke`. These cover every acceptance item 1–7 of the phase specification, including 296 boundary bodies / 216 collapsed interior cells, the per-chunk pool cap, four substeps per tick, sleep after five seconds and canonical-hash save round trip.
- **Windows MSVC build: success; CTest 115/115 passed, 0 failed** (Release, MSVC 19.51.36260, CMake 4.3.1, Ninja, Lua 5.4.7 built locally as a static library; OpenGL pixel smoke tests included in the run). The count grew from the 107 registered before the audit: three regression tests cover the corrections below and five cover the E.0 determinism work. One pre-existing test, `forces wake bodies and transient force is excluded from stable ECS hash`, failed on the first run because it asserted that applying force leaves the ECS hash unchanged while starting from `awake=false`; since the spec both requires `apply_force` to wake the body and includes the persisted `awake` flag in the hash, the test was corrected to start awake and to assert the wake transition separately. No engine code was changed to make it pass.
- **Linux ASan/UBSan CTest: not run for this revision.** The audit environment is Windows-only, so the sanitizer and Xvfb OpenGL legs of `.github/workflows/ci.yml` remain outstanding until CI runs on this commit.

## Phase D post-implementation audit

A static review of the merged Phase D code against `specs/04_phase.md` produced the following defect list. Items 1, 3 and 4 are corrected by the addendum in the specification and are covered by new unit tests; items 2, 5 and 6 are recorded as open work.

1. **Render interpolation lagged three substeps.** `integrate` refreshed `previous_position` on every substep, so after four substeps it described the state one 1/60 s window back while `render_alpha` spans a whole 1/15 s tick. Fixed.
2. **Debris pool capacity is never released.** `DebrisPool::release` has no caller: a chunk stops spawning debris after two broken blocks. Open — needs a debris lifetime decision that no approved specification covers.
3. **Sleeping debris ignored the world.** `resolve_body_vs_voxels` returned early for sleeping bodies, so debris resting on a block that is later dug out hung in the air. Fixed by waking a sleeping body whose support voxels have been removed.
4. **Material parameters were clamped inconsistently.** The solver clamped restitution but the voxel resolver used it raw, so a body with restitution above 1 gained energy on every voxel bounce. Fixed by sanitising at construction and in the voxel resolver.
5. **Degenerate SAT normal.** `box_box_sat` derives its normal from the sign of the centre delta and returns an arbitrary direction when that delta is exactly zero. Latent: current debris uses 0.125 m microvoxels whose centres never coincide with a unit voxel centre. Open.
6. **Edits were applied from the render callback, and the fracture seed mixed the frame-derived tick counter.** Deterministic replay therefore depended on frame timing. **Fixed as E.0** (see below).

## E.0 — determinism prerequisites for networking

The first two items of the E.0 section in [`specs/05_phase.md`](specs/05_phase.md) are implemented; the third is deliberately withheld pending review.

- `app/edit_queue.{h,cpp}` (`EditRequestQueue`) buffers pointer rising edges observed once per frame and releases them, in order, inside the 15 Hz tick. `app/main.cpp` no longer calls `EditMode::apply` from the render lambda, so the number of applied edits no longer depends on the frame rate. The queue is bounded (`max_pending = 8`) so a stuck button cannot grow it without limit. Four tests in `tests/test_edit_queue.cpp` cover queuing, held-button suppression, frame-rate independence and the bound.
- `physics::fracture_seed(world_seed, tick, x, y, z)` is a pure splitmix64 mix of exactly those four values, replacing the previous seed that XORed a frame-derived counter. One test in `tests/test_destruction.cpp` asserts that the seed is stable, that every input (including coordinate sign) changes it, and that identical seeds yield byte-identical debris.
- **Not implemented:** `DebrisPool::release` still has no caller, because choosing a debris lifetime or chunk-unload policy is a design decision the review has not made.

## Phase B1 verification

- Specification and review: [`specs/01_phase_b1.md`](specs/01_phase_b1.md).
- One command per step, no manual `-D` flags: `cmake --preset windows-release`, `cmake --build --preset windows-release`, `ctest --preset windows-release`. Presets also exist for `debug`, `release` and `windows-debug`; `tools/build-windows.ps1` drives them from a plain shell by entering `vcvars64` itself, and `tools/build.sh` / `tools/test.sh` did the same for POSIX shells.
- Lua 5.4.6 is vendored from a pinned URL with a locally computed SHA-256, compiled as `nuc_lua` from an explicit 32-file source list and exposed as `Lua::Lua`; `-DUSE_SYSTEM_LUA=ON` switches to `find_package(Lua 5.4)`, and without a system Lua it fails with CMake's standard `Could NOT find Lua (missing: LUA_LIBRARIES LUA_INCLUDE_DIR)` pointing at `cmake/lua.cmake:18` — the vendored path is the default and the only one verified here.
- Windows MSVC **Release: 115/115 CTest passed**, **Debug: 115/115 passed**, Windows-only host, CMake 4.3.1 + Ninja from Visual Studio 18.
- One-file incremental rebuild (touch `world/chunk.cpp`): **0.14 s** with header tracking working, well under the 20-second limit.
- **Header dependency tracking was silently dead on this host** and is now verified: the Russian MSVC prints a localized `/showIncludes` line, and CMake recorded that prefix into `CMakeFiles/rules.ninja` in the console code page. Under the default OEM code page 866 the recorded prefix is mojibake, so Ninja matched nothing and every object reported `#deps 0` — editing a header produced `ninja: no work to do`. Configuring under `chcp 65001` restores it (`#deps 2` on `chunk_mesh.cpp.obj`, header edits rebuild dependents); `VSLANG=1033` does not translate cl. `tools/build-windows.ps1` sets the code page and now fails the build if any `nuc_render` object reports zero dependencies. The earlier "1.6 s" incremental figure was measured while tracking was broken and has been replaced.
- Two build-system bugs were found and fixed during the run: `FetchContent_Populate` publishes `<name>_SOURCE_DIR` into the calling scope, so wrapping it in a CMake function silently produced `/src/lapi.c` paths, and a `cmake_policy(PUSH)` left without its `POP` when a download failed replaced the real error with a bogus policy imbalance.
- Fifteen tests failed on the first clean run for an environmental reason, not an engine one: they wrote scratch files to `%TEMP%`, and a sandboxed child process is confined to the workspace. They now use `tests/test_scratch.h` with `NUC_TEST_TMP_DIR` inside the build tree, which also makes them hermetic on CI runners.

## Phase T verification (industrial textures)

- Specification: [`specs/02_textures.md`](specs/02_textures.md).
- Six procedural 32×32 tiles (stone, soil, grass, water, copper ore, iron ore) are generated at startup from `splitmix64` noise, packed into one `GL_TEXTURE_2D_ARRAY` layer per material, and sampled with `GL_NEAREST_MIPMAP_NEAREST`. No image files ship with the game; the style is riveted plate, rusted sheet, painted grate, coolant and ore-in-concrete.
- Chunk meshes now carry per-face UVs oriented so the tile's up direction follows world +Y, and every vertex carries its material layer. The fragment shader keeps a `u_textured = 0` fallback to the old flat per-material colours, so a driver without texture-array support loses the art but not the game.
- `windows-release`: **125/125 CTest passed** (8 texture cases, 2 mesh-UV cases), full application run clean, screenshot [`specs/img/textures_32.png`](specs/img/textures_32.png) — 687 distinct colours in frame against 119 for the previous flat shading.
- Deferred on purpose: water stays opaque until Phase J sorts transparent faces.

## Phase W verification (world generation)

- Specification: [`specs/03_worldgen.md`](specs/03_worldgen.md).
- Terrain is now a pure function of `(seed, world_x, world_z)`: a four-octave continent field plus a three-octave detail field, and a separate low-frequency temperature field that labels each column `temperate` or `arid`. Height blends the two biome profiles across a ±0.15 temperature band, so the silhouette stays continuous at a biome border instead of stepping.
- Caves are the intersection of two near-zero isosurfaces of independent 3D noise fields, which produces tunnels rather than a sponge. They are carved only into solid material, never within four blocks of a column's surface and never below `y = 2`, so the crust a player stands on is intact.
- Ore veins were rewritten: `world/gen_resources.cpp` used to be two 4000-character lines with no structure and placed ore voxel-by-voxel; it is now readable, loads the same `data/biomes.json`, and grows a chunk-local random walk per vein, so veins never straddle a chunk border and the cost is a handful of attempts instead of a full-volume scan. The biome now comes from the terrain, not from a region hash.
- `windows-release`: **134/134 CTest passed** (nine new W1–W9 cases). Nine starting chunks generate in **6 ms** against a 250 ms budget.
- Two tuning facts worth keeping: the first tuned world put the spawn in water (16 green pixels in frame), fixed by `base_height` 12 → 14, `amplitude` 8 → 7, `sea_level` 10 → 8; and the camera had to move from `y = 18` to `y = 24` because it started inside the taller hills. Screenshot: [`specs/img/worldgen.png`](specs/img/worldgen.png).

## Phase M verification (multithreading)

- Specification: [`specs/04_parallel.md`](specs/04_parallel.md).
- `core/jobs.*` grew from six minified lines into a real owner of the enkiTS scheduler: a fixed, clamped pool size, `parallel_for`, a `submit` that accepts move-only callables, `wait_idle`, and a destructor that waits for everything still running before the caller's captures die.
- `world/parallel_chunks.*` generates a batch of chunks across the pool and returns them in request order, so callers never depend on thread count or completion order. Generation stayed a pure function of `(seed, world coordinates)`, which is what makes this safe at all.
- `world/chunk_manager.*` now takes an optional `Jobs*`: mesh rebuilds are queued into the pool instead of spawning one `std::async` OS thread per dirty chunk. A boundary edit that dirties four chunks used to create four threads; it now creates four queued tasks. Without a pool the old path still works, which keeps existing tests untouched.
- `windows-release`: **144/144 CTest passed**. Nine starting chunks: **4.55 ms on one worker, 1.60 ms on four** (2.8×).
- Determinism holds across thread counts: 16 chunks generated in parallel are byte-identical to the sequential result, and `Jobs(1)` matches `Jobs(4)`.
- Two API corrections found by the compiler, worth remembering: enkiTS v1.11 waits with `WaitforAll()`, not `WaitforAllTasks()`; and a pool task cannot be a `std::function<void()>` when it captures `std::unique_ptr`, so `submit` is a template that boxes the callable in a `std::shared_ptr`.

## Repository and CI status (2026-10-23)

- The project lives at [Cr512-netizen/V-C-V](https://github.com/Cr512-netizen/V-C-V) on `main`. The repository previously held three "Add files via upload" commits from 2026-10-22; they are preserved on the branch `backup/upload-2026-10-22`, and `main` now carries the real history from `707882f` (baseline, 115/115) to `bb208ee` (phase M).
- Both workflows are green on `bb208ee`: [Linux, ASan/UBSan under Xvfb](https://github.com/Cr512-netizen/V-C-V/actions/runs/39944101682) and [Windows 11, MSVC](https://github.com/Cr512-netizen/V-C-V/actions/runs/39944101715).
- Linux measured **144/144 CTest passed** in 15.44 s and a one-file incremental Ninja+ccache rebuild of **1 690 ms**, well under the 20-second rule. This closes the Linux sanitizer gap that had been open since phase D.
- The two workflow files still pass flags the presets no longer need (`LUA_INCLUDE_DIR`, `LUA_LIBRARY`, and a hand-built Lua 5.4.8 while the tree vendors 5.4.6). CMake accepts them as unused cache variables, so CI is green, but the Windows job spends a step building a Lua library it never links. Cleaning that up is the first task of phase B2.

## Windows installer (2026-10-23)

- Specification: [`specs/06_installer.md`](specs/06_installer.md); script: [`installer/vcv.iss`](installer/vcv.iss).
- CI now publishes a real setup program next to the portable ZIP: `choco install innosetup` on the runner, `ISCC.exe` with `StageDir`/`AppVersion`/`OutDir` defines, and a separate artifact `V-C-V-Installer-Windows-x64` kept for 30 days. The ZIP stays as the no-install option.
- The installer is built **after** the CTest step in the same job, so a red test never publishes a new setup program. A following step then installs the produced file silently, checks `nuc_app.exe`, runs the uninstaller and fails if anything is left behind — producing a file is not the same as producing a usable installer.
- `PrivilegesRequired=lowest`: the game installs into `%LOCALAPPDATA%\Programs\V-C-V` without UAC for the current user. `VersionInfoVersion` accepts only four numeric components, so the version is `0.1.0.<run_number>`.
- Locally (Inno Setup 6.7.3): compile succeeds in 0.74 s and yields a 2.4 MB setup program with correct `ProductVersion`. Installing and uninstalling cannot be completed on this host: the sandbox denies child processes writes to the Start Menu and to `HKCU`, so the installer rolls back cleanly after copying 18 files. That path is covered by the CI smoke-install instead.

## Windows build gate

1. MSVC CMake configure — passed.
2. Build `nuc_app` and `nuc_tests` — passed.
3. CTest — passed.
4. Assemble portable directory — passed.
5. Upload ZIP artifact — passed.
6. Launch and visually smoke-test — **partially verified.** The Release build launches from the project source directory on this machine: Lua mods `core` and `example` load, the nine initial chunks emit chunk-load events, the GL context is created and chunk meshes reach video memory, and the process was still running with an intact loop after seven seconds of manual observation. Launching from the build directory instead exits immediately without diagnostics, because the mod and data paths are resolved relative to the working directory. A visual inspection on a clean Windows desktop is still outstanding.

The current portable ZIP is a test build, not a finished game. Phases A, B, C and D are complete against their automated acceptance gates, with the Phase D build/test gate re-run and green after the audit corrections below. The Windows visual smoke test remains unverified on a real Windows desktop.

## Next

The Phase D gate is closed: the MSVC configure, build and the full 110-test CTest run all pass, and the Linux ASan/UBSan leg is the only remaining verification gap. `specs/05_phase.md` (Phase E, networking) is written and awaits review; it opens with an E.0 prerequisite section, because deterministic replication is untestable while edits are applied from the render callback and the fracture seed mixes a frame-driven counter.

Phase E stays blocked until that specification is approved, and until E.0 makes input and randomness tick-driven. Open audit items 2 (debris lifetime), 5 (degenerate SAT normal) and half of 6 (tick-driven edits) are carried into that review rather than fixed speculatively.
