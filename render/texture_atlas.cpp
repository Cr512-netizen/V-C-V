#include "render/texture_atlas.h"

namespace nuc::render {
const std::uint8_t* TextureAtlas::tile(int layer_index) const noexcept {
    if (layer_index < 0 || layer_index >= layers) return nullptr;
    return rgba.data() + static_cast<std::size_t>(layer_index) * tile_rgba_bytes;
}

int TextureAtlas::layer(std::uint32_t material) const noexcept {
    const auto it = layer_of.find(material);
    return it == layer_of.end() ? -1 : it->second;
}

TextureAtlas build_atlas(const std::vector<std::uint32_t>& materials) {
    TextureAtlas atlas;
    atlas.rgba.reserve(materials.size() * tile_rgba_bytes);
    for (const std::uint32_t material : materials) {
        if (material == 0) continue;
        if (atlas.layer_of.count(material) != 0) continue;
        const RgbaTile generated = generate_tile(pattern_for(material), material);
        atlas.layer_of.emplace(material, atlas.layers);
        atlas.rgba.insert(atlas.rgba.end(), generated.pixels.begin(), generated.pixels.end());
        ++atlas.layers;
    }
    return atlas;
}
} // namespace nuc::render
