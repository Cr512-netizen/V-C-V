#include <catch2/catch_test_macros.hpp>
#include "render/texture_atlas.h"
#include "render/texture_gen.h"
#include "world/registry.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <unordered_map>

namespace {
using nuc::render::RgbaTile;
using nuc::render::TextureAtlas;
using nuc::render::tile_extent;
using nuc::render::tile_rgba_bytes;

std::vector<std::uint32_t> registry_materials() {
    nuc::world::Registry registry;
    registry.load(std::filesystem::path(NUC_SOURCE_DIR) / "data/materials.json",
                  std::filesystem::path(NUC_SOURCE_DIR) / "data/blocks.json");
    std::vector<std::uint32_t> ids;
    for (const auto& material : registry.materials()) ids.push_back(material.id);
    return ids;
}

struct Hsv {
    float hue{};        // degrees, 0..360
    float saturation{}; // 0..1
    float value{};      // 0..1
};

Hsv to_hsv(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    const float rf = static_cast<float>(r) / 255.0F;
    const float gf = static_cast<float>(g) / 255.0F;
    const float bf = static_cast<float>(b) / 255.0F;
    const float high = std::max({rf, gf, bf});
    const float low = std::min({rf, gf, bf});
    const float delta = high - low;
    Hsv hsv;
    hsv.value = high;
    hsv.saturation = high <= 0.0F ? 0.0F : delta / high;
    if (delta <= 0.0F) return hsv;
    if (high == rf) hsv.hue = 60.0F * std::fmod((gf - bf) / delta, 6.0F);
    else if (high == gf) hsv.hue = 60.0F * ((bf - rf) / delta + 2.0F);
    else hsv.hue = 60.0F * ((rf - gf) / delta + 4.0F);
    if (hsv.hue < 0.0F) hsv.hue += 360.0F;
    return hsv;
}

const std::uint8_t* pixels_of(const TextureAtlas& atlas, std::uint32_t material) {
    const int layer = atlas.layer(material);
    return layer < 0 ? nullptr : atlas.tile(layer);
}

float mean_luma(const std::uint8_t* tile) {
    float sum = 0.0F;
    for (int i = 0; i < tile_extent * tile_extent; ++i) {
        sum += 0.299F * static_cast<float>(tile[i * 4 + 0]) + 0.587F * static_cast<float>(tile[i * 4 + 1]) +
               0.114F * static_cast<float>(tile[i * 4 + 2]);
    }
    return sum / static_cast<float>(tile_extent * tile_extent) / 255.0F;
}

float luma_stddev(const std::uint8_t* tile) {
    const float mean = mean_luma(tile) * 255.0F;
    float sum = 0.0F;
    for (int i = 0; i < tile_extent * tile_extent; ++i) {
        const float luma = 0.299F * static_cast<float>(tile[i * 4 + 0]) + 0.587F * static_cast<float>(tile[i * 4 + 1]) +
                           0.114F * static_cast<float>(tile[i * 4 + 2]);
        sum += (luma - mean) * (luma - mean);
    }
    return std::sqrt(sum / static_cast<float>(tile_extent * tile_extent)) / 255.0F;
}

// Share of pixels that are clearly coloured, and the circular mean hue of those.
struct ColourStats {
    float saturated_share{};
    float mean_hue{};
};
ColourStats colour_stats(const std::uint8_t* tile) {
    float x_sum = 0.0F;
    float y_sum = 0.0F;
    int count = 0;
    for (int i = 0; i < tile_extent * tile_extent; ++i) {
        const Hsv hsv = to_hsv(tile[i * 4 + 0], tile[i * 4 + 1], tile[i * 4 + 2]);
        if (hsv.saturation < 0.20F || hsv.value < 0.05F) continue;
        const float radians = hsv.hue * 3.14159265F / 180.0F;
        x_sum += std::cos(radians);
        y_sum += std::sin(radians);
        ++count;
    }
    ColourStats stats;
    stats.saturated_share = static_cast<float>(count) / static_cast<float>(tile_extent * tile_extent);
    if (count > 0) {
        float hue = std::atan2(y_sum, x_sum) * 180.0F / 3.14159265F;
        if (hue < 0.0F) hue += 360.0F;
        stats.mean_hue = hue;
    }
    return stats;
}

float mean_distance(const std::uint8_t* a, const std::uint8_t* b) {
    float sum = 0.0F;
    for (int i = 0; i < tile_extent * tile_extent; ++i) {
        for (int channel = 0; channel < 3; ++channel) {
            sum += std::abs(static_cast<float>(a[i * 4 + channel]) - static_cast<float>(b[i * 4 + channel])) / 255.0F;
        }
    }
    return sum / static_cast<float>(tile_extent * tile_extent * 3);
}
} // namespace

TEST_CASE("atlas covers every registry material with a 32x32 layer") {
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());
    REQUIRE(atlas.layers > 0);
    REQUIRE(atlas.rgba.size() == static_cast<std::size_t>(atlas.layers) * tile_rgba_bytes);
    REQUIRE(atlas.layer(0) == -1); // air has no faces to texture
    for (const std::uint32_t material : {1U, 2U, 3U, 4U, 10U, 11U}) {
        const int layer = atlas.layer(material);
        REQUIRE(layer >= 0);
        REQUIRE(atlas.tile(layer) != nullptr);
    }
    REQUIRE(atlas.layer_of.size() == static_cast<std::size_t>(atlas.layers));
}

TEST_CASE("tile generation is deterministic") {
    for (const std::uint32_t material : {1U, 2U, 3U, 4U, 10U, 11U}) {
        const RgbaTile first = nuc::render::generate_tile(nuc::render::pattern_for(material), material);
        const RgbaTile second = nuc::render::generate_tile(nuc::render::pattern_for(material), material);
        REQUIRE(first.pixels == second.pixels);
    }
}

TEST_CASE("tiles carry structure instead of flat colour") {
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());
    for (const std::uint32_t material : {1U, 2U, 3U, 4U, 10U, 11U}) {
        const auto* tile = pixels_of(atlas, material);
        INFO("material " << material);
        REQUIRE(tile != nullptr);
        REQUIRE(luma_stddev(tile) > 0.02F);
    }
}

TEST_CASE("tile palettes match the industrial style") {
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());

    const auto stone = colour_stats(pixels_of(atlas, 1));
    REQUIRE(stone.saturated_share < 0.30F); // steel reads as grey
    REQUIRE(mean_luma(pixels_of(atlas, 1)) > 0.20F);

    const auto soil = colour_stats(pixels_of(atlas, 2));
    REQUIRE(soil.saturated_share > 0.50F);
    REQUIRE(soil.mean_hue > 10.0F);
    REQUIRE(soil.mean_hue < 45.0F); // rust orange-brown

    const auto grass = colour_stats(pixels_of(atlas, 3));
    REQUIRE(grass.saturated_share > 0.30F);
    REQUIRE(grass.mean_hue > 80.0F);
    REQUIRE(grass.mean_hue < 165.0F); // painted green

    const auto water = colour_stats(pixels_of(atlas, 4));
    REQUIRE(water.saturated_share > 0.50F);
    REQUIRE(water.mean_hue > 170.0F);
    REQUIRE(water.mean_hue < 240.0F); // technical coolant blue

    const auto copper = colour_stats(pixels_of(atlas, 10));
    REQUIRE(copper.saturated_share > 0.02F); // ore inclusions are visible in concrete

    const auto iron = colour_stats(pixels_of(atlas, 11));
    REQUIRE(iron.saturated_share < 0.30F); // dark rock with metallic glints
}

TEST_CASE("coolant is the only translucent tile") {
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());
    for (const std::uint32_t material : {1U, 2U, 3U, 10U, 11U}) {
        const auto* tile = pixels_of(atlas, material);
        for (int i = 0; i < tile_extent * tile_extent; ++i) REQUIRE(tile[i * 4 + 3] == 255);
    }
    const auto* water = pixels_of(atlas, 4);
    REQUIRE(water[3] == 170);
}

TEST_CASE("materials stay visually distinct from each other") {
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());
    const std::array<std::uint32_t, 6> materials{1, 2, 3, 4, 10, 11};
    for (std::size_t i = 0; i < materials.size(); ++i) {
        for (std::size_t j = i + 1; j < materials.size(); ++j) {
            INFO("materials " << materials[i] << " and " << materials[j]);
            REQUIRE(mean_distance(pixels_of(atlas, materials[i]), pixels_of(atlas, materials[j])) > 0.04F);
        }
    }
}

TEST_CASE("atlas size stays inside GL texture array limits") {
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());
    REQUIRE(tile_extent * atlas.layers <= 256);
}

TEST_CASE("atlas generation stays well under one frame") {
    const auto start = std::chrono::steady_clock::now();
    const TextureAtlas atlas = nuc::render::build_atlas(registry_materials());
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    REQUIRE(atlas.layers > 0);
    REQUIRE(elapsed.count() < 50);
}
