#include "world/generator.h"
#include <FastNoiseLite.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nuc::world {
ChunkGenerator::ChunkGenerator(GeneratorSettings settings) : settings_(settings) {
    if (!std::isfinite(settings_.frequency) || settings_.frequency <= 0.0F)
        throw std::invalid_argument("Terrain noise frequency must be finite and positive");
    if (!std::isfinite(settings_.base_height) || !std::isfinite(settings_.amplitude) || settings_.amplitude < 0.0F)
        throw std::invalid_argument("Terrain height settings must be finite and amplitude non-negative");
    if (settings_.sea_level < 0 || settings_.sea_level >= static_cast<std::int32_t>(Chunk::extent))
        throw std::invalid_argument("Sea level must fit inside the chunk");
}
Chunk ChunkGenerator::generate(std::int32_t chunk_x, std::int32_t chunk_z) const {
    FastNoiseLite noise(settings_.seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(settings_.frequency);
    Chunk chunk(chunk_x, chunk_z);
    const std::int64_t origin_x = static_cast<std::int64_t>(chunk_x) * static_cast<std::int64_t>(Chunk::extent);
    const std::int64_t origin_z = static_cast<std::int64_t>(chunk_z) * static_cast<std::int64_t>(Chunk::extent);
    for (std::size_t z = 0; z < Chunk::extent; ++z) {
        for (std::size_t x = 0; x < Chunk::extent; ++x) {
            const auto wx = static_cast<float>(origin_x + static_cast<std::int64_t>(x));
            const auto wz = static_cast<float>(origin_z + static_cast<std::int64_t>(z));
            const auto height = static_cast<std::int32_t>(std::lround(settings_.base_height + noise.GetNoise(wx, wz) * settings_.amplitude));
            const auto surface = std::clamp(height, 0, static_cast<int>(Chunk::extent) - 1);
            for (std::size_t y = 0; y < Chunk::extent; ++y) {
                std::uint32_t material = 0; // air
                if (static_cast<int>(y) <= surface) {
                    if (static_cast<int>(y) == surface) material = 3; // grass
                    else if (static_cast<int>(y) >= surface - 3) material = 2; // soil
                    else material = 1; // stone
                } else if (static_cast<int>(y) <= settings_.sea_level) {
                    material = 4; // water
                }
                chunk.voxels()[Chunk::index(x,y,z)] = material;
            }
        }
    }
    return chunk;
}
}
