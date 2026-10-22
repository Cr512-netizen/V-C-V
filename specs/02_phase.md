# Phase B — Dig and Place

Status: draft for review. No Phase B implementation may begin until this interface/test review is approved.

## Goal
Support deterministic voxel queries and block editing while keeping the existing 32³ chunk representation, SVDAG data structures, 15 Hz simulation tick, and current GL renderer. User edits are stored as sparse overrides over generated/base chunk data; edited chunks are remeshed at tick boundaries. Do not start saves, physics, networking, or later phases.

## Coordinate and ownership decisions
- World coordinates are signed integer voxel coordinates `(x,y,z)`; chunks are addressed by signed `(chunk_x,chunk_z)`, with Y local to the current 32-high chunk model.
- Negative X/Z use mathematical floor division, never C++ truncation toward zero.
- `material_id == 0` means air. `get_voxel` returns air outside the supported vertical range.
- The base world remains owned by the existing generator/chunk map. `VoxelDelta` stores only edits and never mutates the generator's base data.
- OpenGL resource creation/upload remains on the render thread because the current context is not shared. CPU meshing may run asynchronously; completed mesh uploads are queued and applied at the render-frame boundary. Do not claim an OpenGL staging buffer is asynchronous GPU upload; use a reusable batched/orphaned upload path only if the GL implementation can verify it.
- Chunk extent stays 32³ for this phase; optimize/batch edits so a burst does not rebuild a chunk multiple times in one tick. Do not change the core chunk size in this phase.

## Required interfaces

### `world/raycast.h/.cpp`
- `struct VoxelCoord { int x, y, z; equality and hash/order support; }`
- `struct RayHit { VoxelCoord voxel; VoxelCoord previous_air; VoxelCoord normal; std::uint32_t material; float distance; bool has_previous_air; }`. If the ray starts inside a solid voxel, `has_previous_air` is false; placement must not use an invalid previous-air coordinate.
- `using VoxelReader = std::function<std::uint32_t(int,int,int)>;`
- `std::optional<RayHit> raycast(const VoxelReader&, glm::vec3 origin, glm::vec3 direction, float max_distance)`
- Implement grid DDA in world space; normalize direction safely; reject non-finite inputs, zero direction and negative max distance. First solid voxel is hit. `previous_air` is the last empty voxel before the hit and `normal` is the entry face normal. Boundary/tie behavior must be deterministic and documented.
- Must not depend on GLFW or renderer.

### `world/delta.h/.cpp`
- `class VoxelDelta` exposes `get(VoxelCoord) -> optional<uint32_t>`, `set(VoxelCoord,uint32_t material,uint32_t base_material)`, `erase(VoxelCoord)`, `size()`, and a tested chunk-overlay API. `set` removes the override when `material == base_material`; setting air over a non-air base voxel keeps an explicit air override.
- Setting an override to the base material removes the redundant override when the base material is supplied to the API. Air is a valid explicit override for breaking a generated block.
- The reader path checks delta first, then base chunks/generator. No gameplay rules in this module.

### `world/chunk_manager.h/.cpp`
- Owns or coordinates loaded chunks, CPU meshes, dirty state and a bounded queue of completed rebuilds; preserve the existing chunk storage interfaces where practical.
- `get_voxel(VoxelCoord)`, `set_voxel(VoxelCoord,uint32_t)`, `mark_dirty(VoxelCoord)`, `rebuild_dirty_at_tick_end()`, and `drain_upload_queue_on_render_thread()` are the conceptual public operations; final signatures may use existing project types.
- A successful edit marks the containing chunk dirty and also its neighbor when the edit is on a chunk boundary. Rebuild at most once per dirty chunk per tick, after all edits for that tick are collected.
- Async work is CPU-only. Generation/rebuild results carry a revision/generation token so stale worker results are discarded. GL upload and resource destruction happen only on the render thread with a current context.
- On allocation/job failure, retain the last valid mesh and leave the chunk dirty for retry; never display freed GPU resources.

### `app/edit_mode.h/.cpp`
- Receives a camera ray, edit action and reach; uses raycast plus ChunkManager.
- Left mouse breaks the hit block by writing air to the delta. Right mouse places the selected material into `previous_air` only when the target is air. Do not allow placement inside a solid block.
- Add a minimal selected-material default and key to cycle through known materials only if registry integration is already available; otherwise use a fixed valid material ID and document it.
- UI/crafting/inventory are out of scope.

## Tests and acceptance criteria
1. DDA tests: axis-aligned hit, diagonal traversal, origin inside solid, miss through air, max distance, negative world coordinates/chunk boundaries, zero/non-finite direction, deterministic edge ties, previous-air and face normal.
2. Delta tests: add/change/remove override; air override breaks base voxel; base value erases redundant override; untouched voxels remain unchanged.
3. Chunk manager tests: world-to-chunk mapping for positive/negative coordinates; edits dirty only affected chunk plus edge neighbor; many edits in one tick cause one rebuild per dirty chunk; stale async results discarded; queue drains safely without GL context for CPU-only tests.
4. Edit tests: place→query→break→query; placement rejected when target is occupied; ray miss does not edit.
5. Existing GL smoke test and all prior tests remain green. Linux Debug uses Ninja with ASan/UBSan; Windows MSVC build and CTest green. Report skipped context-backed tests explicitly.
6. Incremental Ninja build target is under 20 seconds when CI cache is warm; record actual measurement rather than promising it.
7. Update ROADMAP with evidence and exact status, then commit Phase B. Do not begin Phase C before all acceptance criteria pass.

## Review checklist
- [x] Interface and coordinate rules align with current `world::Chunk` (32³) and `ChunkGenerator`.
- [x] DDA API does not require a renderer or window.
- [x] Delta overrides do not mutate deterministic generator output.
- [x] CPU background work never calls GL.
- [x] Chunk boundary neighbor remeshing is specified.
- [x] Failure/revision handling avoids stale mesh uploads and freed-resource use.
- [x] All new files stay under 200 lines and headers include concise contract comments.
- [x] No save-format, physics, networking, tier simulation, automation or NPC scope added.

## Phase A gate
Phase A implementation and tests are green on the latest CI commit. Linux executed the OpenGL pixel smoke test; Windows CTest passed with an explicit skip for the unavailable hidden OpenGL context. A clean-machine launch of the packaged Windows ZIP remains a release-validation task and is not claimed as verified. Phase B implementation must not start until this spec review is approved.

## Review decision (2026-10-18)
Approved for implementation within Phase B only. Reviewed against the current `world::Chunk` (32³), `ChunkGenerator`, `render::build_chunk_mesh`, `core::Jobs`, and render-thread GL ownership. Two ambiguities were corrected before approval: a ray starting inside a solid has no valid placement cell, and redundant delta entries are removed by comparing the edited material with the base material at `set` time. DDA and delta will be implemented and tested first; chunk manager integration follows only after those modules pass. No Phase C+ work is authorized.
