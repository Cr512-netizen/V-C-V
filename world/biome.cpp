#include "world/biome.h"

namespace nuc::world {
BiomeProfile biome_profile(Biome biome) noexcept {
    switch (biome) {
    case Biome::arid:
        // Bare soil over stone, lifted a little and flattened: the silhouette
        // alone should tell the two biomes apart, not only the colour.
        return BiomeProfile{.surface = 2, .subsurface = 1, .height_bias = 2.0F, .amplitude_scale = 0.6F};
    case Biome::temperate:
    default:
        return BiomeProfile{.surface = 3, .subsurface = 2, .height_bias = 0.0F, .amplitude_scale = 1.0F};
    }
}

const char* biome_name(Biome biome) noexcept {
    switch (biome) {
    case Biome::arid: return "arid";
    case Biome::temperate: default: return "temperate";
    }
}

Biome biome_from_name(std::string_view name) noexcept {
    if (name == "arid") return Biome::arid;
    return Biome::temperate;
}
}
