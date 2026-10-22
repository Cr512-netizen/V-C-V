#pragma once
#include "physics/body.h"
#include <span>
#include <utility>
#include <vector>

namespace nuc::physics {
using BodyPair = std::pair<std::size_t,std::size_t>;
// Builds sorted unique candidate pairs from a floor-indexed uniform grid.
[[nodiscard]] std::vector<BodyPair> build_pairs(std::span<const RigidBody> bodies,
                                                float cell_size = 4.0F);
}
