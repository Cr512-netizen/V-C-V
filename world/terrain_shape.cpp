#include "world/terrain_shape.h"
#include <algorithm>
#include <cmath>

namespace nuc::world {
namespace {
// Temperature is compared against this value to label a column; the height
// blend uses a band around it so the silhouette stays continuous.
constexpr float temperature_threshold = 0.05F;
constexpr float blend_half_width = 0.15F;
}

TerrainShape::TerrainShape(GeneratorSettings settings) : settings_(settings) {
    continent_.SetSeed(settings_.seed);
    continent_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continent_.SetFractalType(FastNoiseLite::FractalType_FBm);
    continent_.SetFractalOctaves(4);
    continent_.SetFrequency(settings_.frequency);

    hills_.SetSeed(settings_.seed + 101);
    hills_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    hills_.SetFractalType(FastNoiseLite::FractalType_FBm);
    hills_.SetFractalOctaves(3);
    hills_.SetFrequency(settings_.frequency * 3.5F);

    temperature_.SetSeed(settings_.seed + 202);
    temperature_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    temperature_.SetFrequency(settings_.frequency * 0.25F);
}

ColumnSample TerrainShape::sample(std::int64_t world_x, std::int64_t world_z) const {
    const auto x = static_cast<float>(world_x);
    const auto z = static_cast<float>(world_z);
    const float continent = continent_.GetNoise(x, z);
    const float hills = hills_.GetNoise(x, z);
    const float shape = 0.65F * continent + 0.35F * hills;

    const float temperature = temperature_.GetNoise(x, z);
    const float warmth = std::clamp((temperature - (temperature_threshold - blend_half_width)) /
                                        (2.0F * blend_half_width), 0.0F, 1.0F);

    const auto temperate = biome_profile(Biome::temperate);
    const auto arid = biome_profile(Biome::arid);
    const float scale = temperate.amplitude_scale + (arid.amplitude_scale - temperate.amplitude_scale) * warmth;
    const float bias = temperate.height_bias + (arid.height_bias - temperate.height_bias) * warmth;

    ColumnSample result;
    result.height = settings_.base_height + settings_.amplitude * scale * shape + bias;
    result.biome = temperature > temperature_threshold ? Biome::arid : Biome::temperate;
    return result;
}
}
