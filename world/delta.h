#pragma once
#include "world/chunk.h"
#include "world/raycast.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>
#include <unordered_map>

namespace nuc::world {
// Sparse edit overrides over generated/base voxels; air overrides are retained.
class VoxelDelta final {
public:
    [[nodiscard]] std::optional<std::uint32_t> get(VoxelCoord position) const;
    void set(VoxelCoord position, std::uint32_t material, std::uint32_t base_material);
    void erase(VoxelCoord position);
    [[nodiscard]] std::size_t size() const noexcept { return overrides_.size(); }
    // Returns a stable, sorted value snapshot; air overrides are included.
    [[nodiscard]] std::vector<std::pair<VoxelCoord,std::uint32_t>> entries() const;
    [[nodiscard]] Chunk apply_to(const Chunk& base) const;
    void apply_to(const Chunk& base, Chunk& output) const;
private:
    std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash> overrides_;
};
}
