#pragma once
#include "world/biome.h"
#include "world/cave_carver.h"
#include "world/chunk.h"
#include "world/generator_settings.h"
#include "world/terrain_shape.h"
#include <cstdint>

namespace nuc::world {
// Builds terrain only: heights, layers, water and caves. Ore veins are applied
// afterwards by ResourceGenerator, so this class stays about shape.
class ChunkGenerator final {
public:
    explicit ChunkGenerator(GeneratorSettings settings = {});
    [[nodiscard]] Chunk generate(std::int32_t chunk_x, std::int32_t chunk_z) const;
    [[nodiscard]] Biome biome_at(std::int64_t world_x, std::int64_t world_z) const;
    [[nodiscard]] const GeneratorSettings& settings() const noexcept { return settings_; }
private:
    GeneratorSettings settings_;
    TerrainShape shape_;
    CaveCarver caves_;
};
}
