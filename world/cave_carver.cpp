#include "world/cave_carver.h"
#include <cmath>

namespace nuc::world {
namespace {
// Vertically stretched so tunnels run flatter than they are wide.
constexpr float vertical_stretch = 2.0F;
constexpr float frequency_scale = 1.6F;
}

CaveCarver::CaveCarver(GeneratorSettings settings) : settings_(settings) {
    const float frequency = settings_.frequency * frequency_scale;
    tunnel_a_.SetSeed(settings_.seed + 303);
    tunnel_a_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    tunnel_a_.SetFrequency(frequency);

    tunnel_b_.SetSeed(settings_.seed + 404);
    tunnel_b_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    tunnel_b_.SetFrequency(frequency);
}

bool CaveCarver::is_cave(std::int64_t world_x, std::int64_t world_y, std::int64_t world_z) const {
    const auto x = static_cast<float>(world_x);
    const auto y = static_cast<float>(world_y) * vertical_stretch;
    const auto z = static_cast<float>(world_z);
    const float a = tunnel_a_.GetNoise(x, y, z);
    if (std::fabs(a) >= settings_.cave_density) return false;
    const float b = tunnel_b_.GetNoise(x, y, z);
    return std::fabs(b) < settings_.cave_density;
}
}
