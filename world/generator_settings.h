#pragma once
#include <cstdint>

namespace nuc::world {
struct GeneratorSettings {
    std::int32_t seed{1337};
    float frequency{0.035F};
    float base_height{14.0F};
    float amplitude{7.0F};
    std::int32_t sea_level{8};
    float cave_density{0.055F};
};
}
