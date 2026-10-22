#pragma once
#include "physics/body.h"
#include <map>
#include <span>
#include <utility>
#include <vector>

namespace nuc::physics {
struct FractureResult {
    std::vector<RigidBody> boundary_bodies;
    std::uint32_t collapsed_interior{};
    std::uint32_t interior_material{};
};
struct PoolReport {
    std::size_t accepted{}, rejected{};
    std::vector<std::size_t> accepted_indices;
};
// Splits one 1 m block into 512 microvoxels; only its boundary becomes rigid bodies.
[[nodiscard]] FractureResult fracture_block(glm::ivec3 block_origin,std::uint32_t material,
                                            std::uint64_t seed,glm::vec3 impulse);
// Pure, world-derived seed for a fracture. It depends only on the world seed, the logic
// tick and the block coordinates, so the same edit on the same tick always yields the
// same debris: no frame counter, clock or address may enter this function.
[[nodiscard]] std::uint64_t fracture_seed(std::uint64_t world_seed,std::uint64_t tick,
                                          int x,int y,int z) noexcept;
// Tracks a hard per-chunk debris capacity; accepted bodies are stored as ECS components.
class DebrisPool final {
public:
    explicit DebrisPool(std::size_t per_chunk_cap=512);
    [[nodiscard]] PoolReport reserve(std::span<const RigidBody> bodies);
    void release(int chunk_x,int chunk_z,std::size_t count);
    [[nodiscard]] std::size_t count(int chunk_x,int chunk_z) const;
private:
    std::size_t cap_;
    std::map<std::pair<int,int>,std::size_t> counts_;
};
}
