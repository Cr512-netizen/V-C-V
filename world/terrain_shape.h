#pragma once
#include "world/biome.h"
#include "world/generator_settings.h"
#include <FastNoiseLite.h>
#include <cstdint>

namespace nuc::world {
struct ColumnSample {
    float height{};            // continuous column height in blocks
    Biome biome{Biome::temperate};
};

// Pure function of world coordinates: no mutable state, safe to call from any
// thread, and continuous across chunk borders by construction.
class TerrainShape final {
public:
    explicit TerrainShape(GeneratorSettings settings);
    [[nodiscard]] ColumnSample sample(std::int64_t world_x, std::int64_t world_z) const;
private:
    GeneratorSettings settings_;
    FastNoiseLite continent_;
    FastNoiseLite hills_;
    FastNoiseLite temperature_;
};
}
