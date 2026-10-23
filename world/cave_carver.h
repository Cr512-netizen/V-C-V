#pragma once
#include "world/generator_settings.h"
#include <FastNoiseLite.h>
#include <cstdint>

namespace nuc::world {
// A cave is the intersection of two near-zero isosurfaces of independent 3D
// noise fields, which yields long tunnels instead of a sponge of holes.
class CaveCarver final {
public:
    explicit CaveCarver(GeneratorSettings settings);
    [[nodiscard]] bool is_cave(std::int64_t world_x, std::int64_t world_y, std::int64_t world_z) const;
private:
    GeneratorSettings settings_;
    FastNoiseLite tunnel_a_;
    FastNoiseLite tunnel_b_;
};
}
