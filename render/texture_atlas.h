#pragma once
#include "render/texture_gen.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace nuc::render {
// One flat array of RGBA8 tiles plus the material -> layer map. The GPU upload
// turns it into a GL_TEXTURE_2D_ARRAY, so tiles never bleed into each other the
// way they would in a packed atlas.
struct TextureAtlas {
    int layers{};
    std::vector<std::uint8_t> rgba;
    std::unordered_map<std::uint32_t, int> layer_of;

    [[nodiscard]] const std::uint8_t* tile(int layer) const noexcept;
    [[nodiscard]] int layer(std::uint32_t material) const noexcept;
};

// Air (material 0) is skipped: it has no faces to texture.
[[nodiscard]] TextureAtlas build_atlas(const std::vector<std::uint32_t>& materials);
} // namespace nuc::render
