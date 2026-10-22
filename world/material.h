#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace nuc::world {
enum MaterialFlag : std::uint32_t {
    MaterialSolid = 1u << 0u,
    MaterialTransparent = 1u << 1u,
    MaterialEmissive = 1u << 2u
};

struct Material {
    std::uint32_t id{};
    std::string name;
    std::array<float, 4> color{1.F, 1.F, 1.F, 1.F};
    std::uint32_t flags{MaterialSolid};
    std::string texture_path;

    [[nodiscard]] bool has_flag(MaterialFlag flag) const noexcept {
        return (flags & static_cast<std::uint32_t>(flag)) != 0;
    }
};

struct BlockDefinition {
    static constexpr int extent = 8;
    static constexpr std::size_t voxel_count = extent * extent * extent;
    std::uint32_t id{};
    std::string name;
    std::array<std::uint32_t, voxel_count> voxels{};

    [[nodiscard]] std::uint32_t at(int x, int y, int z) const;
};
} // namespace nuc::world
