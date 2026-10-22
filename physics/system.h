#pragma once
#include "physics/narrowphase.h"
#include "core/ecs/world.h"

namespace nuc::physics {
// Runs four physics substeps per 15 Hz logic tick; rendering only reads interpolation.
class PhysicsSystem final {
public:
    static constexpr unsigned substeps = 4;
    static void tick(ecs::World& world,double logic_dt,const SolidVoxelQuery& solid = {});
};
[[nodiscard]] glm::vec3 interpolated_position(const RigidBody& body,float alpha) noexcept;
}
