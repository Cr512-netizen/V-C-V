#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nuc::world {
// Biomes are a property of a world column, never of a chunk, so a column looks
// the same whichever chunk asks for it.
enum class Biome : std::uint8_t { temperate = 0, arid = 1 };
inline constexpr std::size_t biome_count = 2;

struct BiomeProfile {
    std::uint32_t surface{};      // material of the top block
    std::uint32_t subsurface{};   // material of the three blocks below the top
    float height_bias{};          // blocks added to the column height
    float amplitude_scale{1.0F};  // multiplier on the terrain amplitude
};

[[nodiscard]] BiomeProfile biome_profile(Biome biome) noexcept;
[[nodiscard]] const char* biome_name(Biome biome) noexcept;
[[nodiscard]] Biome biome_from_name(std::string_view name) noexcept;
}
