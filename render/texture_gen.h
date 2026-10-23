#pragma once
#include <array>
#include <cstdint>

namespace nuc::render {
// Every material gets one 32x32 RGBA tile, generated in process from an
// integer hash. Nothing is read from disk, so a texture cannot go missing and
// two machines render the same pixels.
inline constexpr int tile_extent = 32;
inline constexpr int tile_rgba_bytes = tile_extent * tile_extent * 4;

enum class TilePattern : std::uint8_t {
    riveted_plate,   // steel plate with corner rivets and a darker seam
    rusted_sheet,    // corroded sheet metal with runs and flaking paint
    painted_grate,   // anti-slip painted plate worn down to bare metal
    coolant,         // dark technical fluid behind inspection glass
    ore_in_concrete, // concrete matrix with ore inclusions
    dark_rock        // dark strata with metallic glints
};

struct RgbaTile {
    std::array<std::uint8_t, tile_rgba_bytes> pixels{};
};

[[nodiscard]] TilePattern pattern_for(std::uint32_t material) noexcept;
[[nodiscard]] RgbaTile generate_tile(TilePattern pattern, std::uint32_t material);
} // namespace nuc::render
