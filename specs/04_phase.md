# Phase D — Physics and Destruction

Status: reviewed and approved for implementation (2026-10-18).

## Goal
Add a deterministic, testable rigid-body foundation without replacing the ECS, fixed-tick loop, chunk manager or current renderer. A body falls under gravity, collides and bounces, settles and sleeps after about five seconds. Breaking a one-metre block creates boundary micro-voxel rigid bodies while the interior collapses into a non-rigid rubble payload.

## Scope and design constraints
- Keep the logic tick at 15 Hz. `PhysicsSystem` always runs exactly 4 physics substeps per logic tick (60 Hz); render interpolation reads previous/current positions and never advances simulation.
- Use semi-implicit Euler, uniform-grid broadphase, AABB box-box SAT (three separating axes for the current axis-aligned body model), and sequential normal/friction impulses. Angular rotation is not part of this first working milestone; do not add an OBB/quaternion subsystem unless a reviewed test demonstrates it is necessary.
- Static bodies have inverse mass 0. Do not mutate world voxels from the solver.
- Keep world collision behind a `SolidVoxelQuery` callback so tests do not require a renderer/window. Add a direct `point_vs_svdag` query using the existing `world::Svdag`; the live application adapter may use `ChunkManager::get_voxel` because current live chunks are stored as `Chunk`, not as one global SVDAG.
- Keep physics and destruction in C++ engine modules. Gameplay decisions remain in mods/Lua.
- No new dependencies. Keep each new/modified module file below 200 lines, with one responsibility and a contract comment in each public header.

## Required interfaces

### `physics/body.h/.cpp`
- `struct RigidBody`: position, previous_position, velocity, accumulated force, half extents, inverse mass, restitution, friction, sleep timer, awake flag, material ID, and owner chunk coordinates for debris pooling. The ECS entity ID remains the component owner and is not duplicated in the body.
- `RigidBody make_body(glm::vec3 position, glm::vec3 half_extents, float mass)`; mass 0 creates a static body. Reject non-finite values, non-positive extents and negative mass.
- `void apply_force(RigidBody&, glm::vec3)`; ignore invalid force vectors and wake the body on valid nonzero force.
- Specialize the ECS stable-state traits for RigidBody; hash persisted numeric fields by exact IEEE-754 bit patterns and stable integer/bool fields. Exclude accumulated force because it is transient and cleared on restore.

### `physics/integrator.h/.cpp`
- `void integrate(RigidBody&, double dt, glm::vec3 gravity)`.
- Semi-implicit Euler: update velocity from gravity + accumulated force, then position from updated velocity; clear force after integration.
- Ignore sleeping/static bodies. Reject invalid dt/gravity without injecting NaNs. Save `previous_position` immediately before a valid substep.

### `physics/broadphase.h/.cpp`
- `using BodyPair = std::pair<std::size_t,std::size_t>`.
- `std::vector<BodyPair> build_pairs(std::span<const RigidBody>, float cell_size=4.0F)`.
- Insert each body's world AABB into every overlapped uniform-grid cell using floor-based coordinates for negatives. Return unique sorted index pairs, skip static-static pairs, and never return self-pairs.

### `physics/narrowphase.h/.cpp`
- `struct Contact { std::size_t a,b; glm::vec3 normal,point; float penetration; }`; normal points from A toward B.
- `std::optional<Contact> box_box_sat(const RigidBody&, const RigidBody&, std::size_t a, std::size_t b)`: test X/Y/Z separating axes, reject separated boxes, return minimum-penetration axis and deterministic contact point for overlap.
- `bool point_vs_svdag(glm::ivec3 local_point, const world::Svdag&)`: return true only for an in-bounds non-air material.
- `using SolidVoxelQuery = std::function<bool(int,int,int)>`; expose `void resolve_body_vs_voxels(RigidBody&, const SolidVoxelQuery&)` for the live ChunkManager adapter. It must resolve a body out of solid voxels without requiring GLFW/OpenGL.

### `physics/solver.h/.cpp`
- `void solve_contacts(std::span<RigidBody>, std::span<const Contact>, double dt, unsigned iterations=8)`.
- Sequential impulse solver: restitution normal impulse, Coulomb friction bounded by the normal impulse, and small positional correction to prevent persistent overlap.
- Static bodies never move. Reject invalid contacts and avoid division by zero for two immovable bodies. Clamp restitution to [0,1] and friction to a nonnegative value at body construction.

### `physics/system.h/.cpp`
- `void tick(ecs::World&, double logic_dt, const SolidVoxelQuery& solid={})`; collect RigidBody ECS components, execute four `logic_dt/4` substeps, and run integration → broadphase → narrowphase → solver each substep.
- Wake bodies on collision/force. A body with speed below 0.05 m/s for 5 seconds becomes sleeping; impacts wake it.
- `glm::vec3 interpolated_position(const RigidBody&, float alpha)` clamps alpha to [0,1] and interpolates previous/current positions.
- Preserve the existing 15 Hz logic tick; do not change `Loop` timing.

### `physics/destruction.h/.cpp`
- `struct FractureResult { std::vector<RigidBody> boundary_bodies; std::uint32_t collapsed_interior; std::uint32_t interior_material; }`.
- `FractureResult fracture_block(glm::ivec3 block_origin, std::uint32_t material, std::uint64_t seed, glm::vec3 impulse)`: split a 1 m block into 8×8×8 microvoxels of side 0.125 m. The 296 boundary microvoxels become RigidBody instances; the 216 interior microvoxels collapse into one compact rubble payload count. Use seeded deterministic radial impulse variation.
- `struct PoolReport { std::size_t accepted, rejected; std::vector<std::size_t> accepted_indices; }`. `class DebrisPool`: expose `PoolReport reserve(std::span<const RigidBody>)`, `void release(int chunk_x,int chunk_z,std::size_t count)` and `std::size_t count(int chunk_x,int chunk_z) const`; this is a per-chunk capacity tracker, while accepted body state lives as ECS components. Enforce a configurable cap (default 512, enough for one complete block fracture), report accepted/rejected counts and never exceed the cap. Bodies retain owner chunk coordinates and material for accounting/rendering.
- Extend world saves to persist/restore ECS RigidBody state, so physics bodies do not silently disappear across save/load. Transient accumulated force is cleared on restore.

### Edit and application integration
- Add a break callback to `EditMode`; invoke it only after a successful break and pass the hit voxel coordinate/material.
- The app fractures the broken block, reserves debris capacity, and creates ECS entities with RigidBody for accepted boundary bodies. Keep excess bodies collapsed/rejected when the per-chunk cap is reached.
- Render active debris as small cubes using one shared GPU unit-cube mesh, material tint and interpolated body positions in the existing GL renderer. Do not allocate a separate GPU mesh per body.
- Call `PhysicsSystem::tick` from the existing 15 Hz logic-tick callback, with four internal substeps and a solid-voxel query backed by `ChunkManager::get_voxel`.

## Tests and acceptance criteria
1. Body/integrator: finite state validation, static-body invariance, semi-implicit Euler values, force reset and previous-position update.
2. Broadphase: negative grid coordinates, AABB spanning multiple cells, sorted unique pairs, static-static exclusion and no self-pairs.
3. Narrowphase: separated boxes miss; overlapping boxes produce a unit normal and positive minimum-axis penetration; touching boxes are deterministic; SVDAG point query hits solid and misses air/out-of-bounds.
4. Solver: static body never moves, impulse direction is correct, restitution causes bounce, friction reduces tangential velocity, and an elastic two-body collision preserves total linear momentum within ±1%.
5. System: exactly four substeps per logic tick, render interpolation does not mutate state, a cube falls onto a floor, bounces, settles and sleeps after five seconds; an impact wakes a sleeping body.
6. Destruction: exactly 296 boundary bodies + 216 collapsed interior cells; deterministic output for the same seed; per-chunk pool cap is never exceeded.
7. Save regression: physics-body state survives save→load with identical canonical hash, with transient forces cleared.
8. All existing tests remain green. Linux Debug uses Ninja + ccache + ASan/UBSan; Windows MSVC build and CTest green. Report the GL-context skip explicitly.
9. Update `ROADMAP.md` with evidence and commit Phase D. Do not begin Phase E networking until every criterion is green.

## Review checklist
- [x] Physics substeps remain 4× inside the existing 15 Hz logic tick.
- [x] AABB SAT and translational sequential impulses are an intentional first working implementation; no unreviewed angular subsystem.
- [x] Negative-coordinate broadphase uses floor division and emits deterministic unique pairs.
- [x] ECS RigidBody has a stable hash trait and is included in world save/restore.
- [x] Voxel collision uses an injectable query; direct SVDAG query is unit-tested.
- [x] Fracture count is 296 boundary bodies and 216 collapsed interior cells; debris cap is per chunk.
- [x] Every module has unit tests; no networking, automation or NPC behavior is added in this phase. Minimal shared-mesh rendering is in scope only to make debris observable.

## Review notes
The current ECS supports arbitrary components but deterministic hashing requires explicit stable component traits. The current world keeps active voxel data in 32³ `Chunk` objects and exposes `Svdag` separately, so collision must not pretend a global SVDAG already exists. This spec keeps live collision behind a query adapter and tests the direct SVDAG query independently. The first milestone deliberately uses axis-aligned boxes so the 15 Hz / 4-substep simulation, contact response, sleeping and destruction pool can be made correct before rotational dynamics are considered.

### Review decision
Approved. The current ECS remains the owner of body identity; transient accumulated force is excluded from persisted/hash state and cleared on restore. The physics solver remains axis-aligned and translational for this first working milestone, with a three-axis SAT contact test and sequential normal/friction impulses. World collision is injected via a callback because active chunks are not globally represented as SVDAGs. Destruction material IDs and per-chunk pool accounting are explicit in the interface. Proceed with Phase D only.

### Review addendum
The implementation review identified that ECS debris would otherwise be simulated but invisible. The reviewed scope now includes a shared unit-cube mesh and a pixel smoke assertion for material-tinted rigid-body rendering. This is a small integration into the existing renderer, not a new renderer architecture or a per-body GPU allocation strategy.

### Corrections addendum (2026-10-22)
A post-implementation audit found three defects that contradict interfaces or acceptance items already approved above, so they stay in scope for this phase. Nothing here changes the body model, the substep count or the solver.

1. **Interpolation must span one logic tick, not one substep.** `integrate` keeps saving `previous_position` before each valid substep, but `PhysicsSystem::tick` additionally snapshots every body position before substep 0 and restores that snapshot as `previous_position` after the last substep. `interpolated_position(body, alpha)` then spans exactly the tick the renderer is catching up on. Test: after one tick from rest, `previous_position` equals the pre-tick position, `interpolated_position(body,0)` equals it, and `interpolated_position(body,1)` equals the current position.
2. **A sleeping body must notice the world changing.** The voxel resolver still applies no impulse to a sleeping body, but it now queries the unit voxels immediately beneath the body's AABB (layer `floor(lo.y)-1`, over the covered x/z columns) and wakes the body when no support remains, so debris cannot hang in the air after its support block is dug out. Test: a sleeping body above a solid voxel stays asleep; the same call with no solid voxel wakes it and clears its sleep timer.
3. **Material parameters are sanitised where they are consumed.** `make_body` clamps restitution to `[0,1]` and friction to nonnegative, and `resolve_body_vs_voxels` clamps its own copies so a hand-set value cannot inject energy. Test: a body constructed with restitution 5 does not leave a voxel bounce faster than it arrived.
