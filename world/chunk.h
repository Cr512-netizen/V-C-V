#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace nuc::world {
class Chunk {
public:
    static constexpr std::size_t extent = 32;
    static constexpr std::size_t voxel_count = extent * extent * extent;
    using Voxels = std::array<std::uint32_t, voxel_count>;

    Chunk(std::int32_t chunk_x = 0, std::int32_t chunk_z = 0) noexcept;
    [[nodiscard]] std::int32_t chunk_x() const noexcept { return chunk_x_; }
    [[nodiscard]] std::int32_t chunk_z() const noexcept { return chunk_z_; }
    [[nodiscard]] std::uint32_t at(std::size_t x, std::size_t y, std::size_t z) const;
    void set(std::size_t x, std::size_t y, std::size_t z, std::uint32_t material);
    [[nodiscard]] const Voxels& voxels() const noexcept { return voxels_; }
    [[nodiscard]] Voxels& voxels() noexcept { return voxels_; }
    [[nodiscard]] static constexpr std::size_t index(std::size_t x, std::size_t y, std::size_t z) noexcept {
        return x + extent * (y + extent * z);
    }
private:
    std::int32_t chunk_x_{};
    std::int32_t chunk_z_{};
    Voxels voxels_{};
};
}
