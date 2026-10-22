#pragma once
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

namespace nuc::world {
// Integer world-space voxel coordinate. X/Z may be negative; Y is world height.
struct VoxelCoord {
    int x{}, y{}, z{};
    friend bool operator==(const VoxelCoord&, const VoxelCoord&) = default;
};
struct VoxelCoordHash {
    [[nodiscard]] std::size_t operator()(const VoxelCoord& p) const noexcept;
};
// previous_air is valid only when has_previous_air is true (ray began in air).
struct RayHit {
    VoxelCoord voxel;
    VoxelCoord previous_air;
    VoxelCoord normal;
    std::uint32_t material{};
    float distance{};
    bool has_previous_air{};
};
using VoxelReader = std::function<std::uint32_t(int,int,int)>;
// Traverses voxels with deterministic X, then Y, then Z tie-breaking.
[[nodiscard]] std::optional<RayHit> raycast(const VoxelReader& read,
    glm::vec3 origin, glm::vec3 direction, float max_distance);
}
