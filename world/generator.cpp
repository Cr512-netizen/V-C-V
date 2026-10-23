#include "world/generator.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nuc::world {
namespace {
constexpr std::uint32_t air = 0;
constexpr std::uint32_t stone = 1;
constexpr std::uint32_t water = 4;
constexpr int soil_depth = 3;    // blocks of subsurface material below the top
constexpr int cave_crust = 4;    // never carve the top four blocks of a column
constexpr int cave_floor = 2;    // never carve the bottom two blocks either
}

ChunkGenerator::ChunkGenerator(GeneratorSettings settings) : settings_(settings), shape_(settings), caves_(settings) {
    if (!std::isfinite(settings_.frequency) || settings_.frequency <= 0.0F)
        throw std::invalid_argument("Terrain noise frequency must be finite and positive");
    if (!std::isfinite(settings_.base_height) || !std::isfinite(settings_.amplitude) || settings_.amplitude < 0.0F)
        throw std::invalid_argument("Terrain height settings must be finite and amplitude non-negative");
    if (settings_.sea_level < 0 || settings_.sea_level >= static_cast<std::int32_t>(Chunk::extent))
        throw std::invalid_argument("Sea level must fit inside the chunk");
    if (!std::isfinite(settings_.cave_density) || settings_.cave_density <= 0.0F || settings_.cave_density >= 0.5F)
        throw std::invalid_argument("Cave density must be finite and inside (0, 0.5)");
}

Biome ChunkGenerator::biome_at(std::int64_t world_x, std::int64_t world_z) const {
    return shape_.sample(world_x, world_z).biome;
}

Chunk ChunkGenerator::generate(std::int32_t chunk_x, std::int32_t chunk_z) const {
    Chunk chunk(chunk_x, chunk_z);
    const std::int64_t origin_x = static_cast<std::int64_t>(chunk_x) * static_cast<std::int64_t>(Chunk::extent);
    const std::int64_t origin_z = static_cast<std::int64_t>(chunk_z) * static_cast<std::int64_t>(Chunk::extent);
    const int top = static_cast<int>(Chunk::extent) - 1;

    for (std::size_t z = 0; z < Chunk::extent; ++z) {
        for (std::size_t x = 0; x < Chunk::extent; ++x) {
            const std::int64_t world_x = origin_x + static_cast<std::int64_t>(x);
            const std::int64_t world_z = origin_z + static_cast<std::int64_t>(z);
            const ColumnSample column = shape_.sample(world_x, world_z);
            const int surface = std::clamp(static_cast<int>(std::lround(column.height)), 0, top);
            const BiomeProfile profile = biome_profile(column.biome);
            const bool submerged = surface <= settings_.sea_level;

            for (int y = 0; y <= top; ++y) {
                std::uint32_t material = air;
                if (y <= surface) {
                    if (y == surface) material = submerged ? profile.subsurface : profile.surface;
                    else if (y >= surface - soil_depth) material = profile.subsurface;
                    else material = stone;
                } else if (y <= settings_.sea_level) {
                    material = water;
                }
                const bool carvable = material != air && material != water;
                if (carvable && y >= cave_floor && y <= surface - cave_crust &&
                    caves_.is_cave(world_x, y, world_z)) {
                    material = air;
                }
                chunk.voxels()[Chunk::index(x, static_cast<std::size_t>(y), z)] = material;
            }
        }
    }
    return chunk;
}
}
