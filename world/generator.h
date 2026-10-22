#pragma once
#include "world/chunk.h"
#include <cstdint>

namespace nuc::world {
struct GeneratorSettings {
    std::int32_t seed{1337};
    float frequency{0.035F};
    float base_height{12.0F};
    float amplitude{8.0F};
    std::int32_t sea_level{10};
};
class ChunkGenerator {
public:
    explicit ChunkGenerator(GeneratorSettings settings = {});
    [[nodiscard]] Chunk generate(std::int32_t chunk_x, std::int32_t chunk_z) const;
    [[nodiscard]] const GeneratorSettings& settings() const noexcept { return settings_; }
private:
    GeneratorSettings settings_;
};
}
