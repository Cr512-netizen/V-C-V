# Phase C — World Saves

Status: complete; Linux and Windows build/test gates green (2026-10-18).

## Goal
A save/load cycle restores the same deterministic world state: world seed, sparse player voxel edits (including air overrides), ECS entities/inventories/work state, civilization state, and Lua mod state. The required stress case performs 10,000 voxel edits and verifies an identical canonical state hash after reload.

## Existing behavior and compatibility constraints
- `save/world_save.*` already writes a version-1 JSON document containing seed, chunk voxel snapshots, entities, civilizations and mod state.
- `world::VoxelDelta` is currently held by `world::ChunkManager`, but it is not persisted. The application currently serializes loaded chunk snapshots at shutdown.
- Keep version-1 loading supported. Do not reinterpret version-1 chunk snapshots as sparse edits: their data is a materialized chunk state and must remain a valid legacy base.
- Version 2 stores sparse edits separately from optional legacy chunk snapshots. New worlds save only seed + sparse edits and simulation state; legacy worlds may retain their version-1 chunk snapshots while adding new edits on top.
- Keep the JSON format versioned and deterministic. Do not add dependencies or change the ECS architecture.

## Required interfaces

### `world/delta.h/.cpp`
- Add `entries() const` returning a value snapshot of all `(VoxelCoord, material_id)` overrides.
- Returned entries must be sorted by `x`, then `y`, then `z`; air (`material_id == 0`) is valid.
- Do not expose the internal unordered map or return references into it.

### `save/world_save.h/.cpp`
- Add `SavedVoxelEdit { int x, y, z; uint32_t material; }` and `WorldSaveData::voxel_edits`.
- New saves default to format version 2. Preserve the existing `chunks` field as optional legacy materialized snapshots.
- Version 2 JSON contains `version: 2`, `seed`, `chunks` (empty for new worlds; preserved for legacy migrations), `voxel_edits`, `entities`, `civilizations`, and `mod_state`.
- Version 1 remains readable with its current `chunks` schema. Version 2 accepts zero material for explicit air edits; reject invalid Y, duplicate edit coordinates, malformed arrays, and unsupported versions with clear errors.
- `WorldSaveData::hash()` must canonicalize order and include format version, seed, chunk snapshots, voxel edits, entities/inventories, civilization state, and mod state. Hashing must not depend on unordered-container iteration.
- Preserve safe temp-file write + replacement behavior.

### `world/chunk_manager.h/.cpp`
- Add a materialized snapshot API that returns copies of loaded chunks with the current sparse delta applied. It must not mutate base chunk storage.
- Saved edits are restored only after the relevant base chunks are loaded; restore them through the existing `set_voxel`/delta path so edits equal to base are canonicalized away.
- Rendering uses materialized snapshots for initial meshes so saved edits are visible on the first frame.

### Application integration
- On load, use saved legacy chunk snapshots when present; otherwise regenerate chunks from the saved seed.
- Ensure every chunk containing a saved edit is loaded before applying edits.
- On save, persist the seed, current delta entries, captured ECS state, civilization state and mod state. Do not overwrite legacy snapshots during migration.
- Keep save operations at orderly shutdown; atomic replacement must avoid leaving a truncated main save after write failure.

## Tests and acceptance criteria
1. Delta snapshot test: stable sort order, including negative coordinates and air overrides; mutating the delta after snapshot does not mutate the snapshot.
2. Format tests: version-2 round trip, version-1 fixture migration/read compatibility, malformed/duplicate/out-of-range edit rejection, unknown-version rejection.
3. Hash tests: canonical hash is unchanged by input ordering; changing one edit, entity, civilization or mod-state value changes the hash.
4. Stress test: 10,000 distinct edits survive save → load with identical hash and identical sorted edit entries.
5. Integration test: edits applied over a deterministic generated base produce the same queried voxel values after reload; an air override must continue to remove a generated solid block.
6. Existing ECS/inventory, chunk, GL smoke and all previous tests remain green. Linux Debug uses Ninja + ccache + ASan/UBSan and CTest; Windows MSVC build and CTest green. Report GL-context skips explicitly.
7. Update `ROADMAP.md` with actual evidence, then commit Phase C. Do not begin Phase D until every criterion is green.

## Review checklist
- [x] Version-1 data is not accidentally treated as sparse delta.
- [x] Version-2 hash includes air edits and all persisted simulation state in canonical order.
- [x] Saved edits load after seed/base generation and before initial mesh creation.
- [x] Legacy saves remain writable after migration without losing their materialized chunk snapshots.
- [x] Tests cover 10,000 edits, hash identity, air overrides, invalid input and version compatibility.
- [x] No physics, networking, tier simulation, automation, NPC features or renderer rewrite are mixed into this phase.

## Review notes
The existing save layer already handles seed, entity records, civilization records and mod-state strings, but the application's current chunk snapshot save is not a sparse edit log and cannot independently represent an air override against deterministic terrain. Version 2 therefore adds a separate edit list and keeps version-1 chunk snapshots as a compatibility base. The chunk manager remains the source of truth for active edits; persistence only snapshots that public state.

### Review decision
Approved. The version split preserves old chunk snapshots without silently reinterpreting them as edits. Sparse edits are loaded only after the saved seed's base chunks exist and are applied through `ChunkManager`, including material ID 0 for breaking generated blocks. Canonical hashing and the 10,000-edit round trip are mandatory regression gates. Keep Phase C limited to persistence and its integration; do not begin physics or other later phases.

## Verification record

- Code commit tested: [`ff0e141`](https://github.com/Cr512-netizen/Nuc-lVuc-l/commit/ff0e14104a6852c6f11f456d7dfdb8b7621a2e57).
- Linux Debug: build success, ASan/UBSan enabled, CTest **90/90 passed**; OpenGL pixel smoke executed under Xvfb and passed.
- Incremental Ninja + ccache rebuild: **1,066 ms** (required below 20,000 ms).
- Windows MSVC: build success, CTest **90 tests / 0 failures**; pixel smoke explicitly skipped because the hosted runner could not create the required hidden OpenGL context.
- [Linux CI](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39148558780) · [Windows CI](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39148558722) · [Windows package artifact](https://github.com/Cr512-netizen/Nuc-lVuc-l/actions/runs/39148558722/artifacts/12144118567).
- Acceptance cases verified: 10,000 sparse edits round-trip with identical hash; legacy v1 snapshot read; invalid version/height/duplicate rejection; canonical hash responds to edit, entity, civilization and mod-state changes; air override remains visible in materialized chunk snapshots without mutating base chunks.
