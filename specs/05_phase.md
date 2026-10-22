# Phase E — Networking and deterministic replication

Status: **proposed — awaiting review.** Do not write `net/` code until this is approved.

## Goal
Two clients in one process exchange inputs, world edits and snapshots through an abstract transport, and both agree on the simulation `state hash` at a tick boundary. The deterministic core is never bypassed: the server is authoritative for world edits, clients predict locally and roll back on divergence.

## Scope and design constraints
- **E.0 is a prerequisite, not optional.** Determinism cannot be tested while gameplay input is applied from the render callback. `app/main.cpp` currently calls `EditMode::apply` inside the render lambda and derives the fracture seed from a frame-driven `tick_index`. E.0 moves both onto the logic tick. Until E.0 lands, Phase E must not begin.
- Keep the logic tick at 15 Hz and four physics substeps. The network never advances or rewinds simulation below the tick.
- **No new dependencies.** The brief's dependency list is fixed, so networking sits behind a transport interface: `LoopbackTransport` for tests and single-process play, a platform socket adapter only after this phase is green. Nothing in the deterministic path may touch a socket, a clock or a thread.
- Determinism contract: for the same seed and the same ordered input stream, `ecs::World::deterministic_hash()` and `WorldSave::hash()` are equal at every tick boundary on the server and on every client. Randomness comes only from `sim::Rng::for_tick(world_seed, tick, stream)`.
- Snapshots every 3 ticks (5 Hz at a 15 Hz tick); client input buffer covers 200 ms (3 ticks).
- Keep each new module below 200 lines with one responsibility and a contract comment in each public header.
- Reuse `save::WorldSave` for canonical state and `sim::Rng` for stream state. Do not invent a second serializer for voxel edits.

## Required interfaces

### E.0 — determinism prerequisites (`app/`, `physics/`)
- Move break/place out of the render callback into the 15 Hz tick callback. Input edges observed between ticks are queued in order and consumed at the next tick, so the number of ticks is independent of frame count and frame rate.
- `fracture_seed(world_seed, logic_tick, block_coords)`: a pure function replacing the current frame-derived seed, so identical inputs at identical ticks produce identical debris velocities.
- Debris capacity becomes reusable: `DebrisPool::release` gains a caller through a per-chunk unload/expiry policy that the review must approve before implementation.

### `net/transport.h`
- `class ITransport { virtual void send(std::span<const std::uint8_t>) = 0; virtual std::vector<std::vector<std::uint8_t>> receive() = 0; virtual ~ITransport() = default; }`.
- `class LoopbackTransport final : public ITransport` with a configurable, deterministic latency measured in ticks and in-order delivery. It must not use threads, sleeps or wall clock.

### `net/protocol.h/.cpp`
- `constexpr std::uint32_t protocol_version = 1`.
- `enum class MessageType : std::uint8_t { hello = 1, input = 2, snapshot = 3, edit = 4, desync = 5 }`.
- `struct PlayerInput { std::uint64_t player_id, tick; float move_x, move_z; bool jump, break_block, place_block; glm::vec3 ray_origin, ray_direction; std::uint32_t material; }`.
- `struct VoxelEdit { int x, y, z; std::uint32_t material; }`.
- `struct SnapshotHeader { std::uint64_t tick, state_hash; std::uint32_t entity_count, edit_count; }`.
- `encode`/`decode` for each payload returning `std::vector<std::uint8_t>` / `std::optional<T>`; reject a wrong `protocol_version` and any truncated or oversized frame without throwing.

### `net/serializer.h/.cpp`
- `struct SerializedState { std::uint64_t tick, seed, state_hash; std::vector<VoxelEdit> edits; std::vector<save::SavedEntity> entities; std::array<std::uint64_t, 4> rng; }`.
- `encode_state`/`decode_state`; edits sorted canonically so ordering cannot affect the hash.
- `apply_state(ecs::World&, world::Delta&, sim::Rng&, const SerializedState&)` restores ECS, sparse edits and RNG streams, and clears transient body force exactly as `WorldSave` does.

### `net/server.h/.cpp`
- `class Server final { Server(std::uint64_t world_seed); void submit(std::span<const std::uint8_t>); void tick(double dt); std::uint64_t state_hash() const; std::vector<VoxelEdit> drain_accepted_edits(); }`.
- Authoritative for edits: an edit is applied only if the block exists, is in range and the player is within reach; otherwise it is dropped and counted.
- Emits a snapshot every 3 ticks.

### `net/client.h/.cpp`
- `class Client final { void queue_input(const PlayerInput&); void receive(std::span<const std::uint8_t>); void tick(double dt); std::uint64_t predicted_hash() const; std::size_t rollback_count() const; }`.
- Prediction: apply local input immediately, keep a 3-tick history of predicted states.
- Rollback: on a snapshot whose hash disagrees with the stored prediction for that tick, restore the snapshot and replay buffered inputs; count each rollback.

### `net/replication.h/.cpp`
- `class InterestSet { void set_center(std::pair<int, int> chunk, int radius); bool wants(std::pair<int, int> chunk) const; }`.
- `filter_edits(std::span<const VoxelEdit>, const InterestSet&)` returns only edits in interested chunks, preserving canonical order.
- Delta compression of edits is a stretch item, not an acceptance criterion.

## Test plan
1. **Protocol round trip** — every payload encodes and decodes identically; a wrong version and a truncated frame are rejected.
2. **Server authority** — an out-of-reach edit is dropped and counted; an in-range edit is accepted and appears in `drain_accepted_edits`.
3. **Two-client convergence (mandatory)** — two `Client`s and one `Server` over `LoopbackTransport`, driven 500 ticks with scripted inputs; assert identical `state_hash()` on server and both clients at tick 500, and that A's edit is present in B's world within one snapshot interval.
4. **Rollback (mandatory)** — a reviewer-authored teleport injected into one client forces divergent prediction; assert `rollback_count() > 0`, that the client converges to the server hash, and that the simulation does not regress below the last acknowledged tick.
5. **Desync detection (mandatory)** — corrupt one client's state so its hash diverges; assert a `desync` message is produced and reports the first divergent tick, and that the run fails loudly rather than continuing silently.
6. **Serializer equivalence** — `SerializedState` save/restore preserves the hash and clears transient forces, matching the existing Phase C/D save tests.
7. **Interest management** — edits outside the radius are filtered; inside are kept and ordered.

## Acceptance criteria
- [ ] E.0 landed: edits and fracture seeds are tick-driven and frame-rate independent, demonstrated by a test that runs the same inputs at two different frame rates and compares hashes.
- [ ] Two clients converge to an identical state hash at tick 500.
- [ ] Client A's edit is visible to client B within one snapshot interval.
- [ ] Prediction rollback occurs on the scripted teleport and converges.
- [ ] Desync is detected and reported with the first divergent tick.
- [ ] Every new module is below 200 lines and has unit tests.
- [ ] No new third-party dependency; no socket, clock or thread in the deterministic path.
- [ ] Linux Debug ASan/UBSan and Windows MSVC CTest are green, `ROADMAP.md` updated, one meaningful commit.

## Out of scope
Interest-based bandwidth optimisation, delta compression, NAT traversal, authentication, anti-cheat, and any rendering change. Vulkan work belongs to Phase I.
