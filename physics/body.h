#pragma once
#include "core/ecs/world.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace nuc::physics {
// Axis-aligned rigid body. Accumulated force is transient; position/velocity are persistent.
struct RigidBody {
    glm::vec3 position{0.0F};
    glm::vec3 previous_position{0.0F};
    glm::vec3 velocity{0.0F};
    glm::vec3 force{0.0F};
    glm::vec3 half_extents{0.5F};
    float inverse_mass{1.0F};
    float restitution{0.1F};
    float friction{0.6F};
    float sleep_time{};
    bool awake{true};
    std::uint32_t material{1};
    int owner_chunk_x{};
    int owner_chunk_z{};
};

[[nodiscard]] RigidBody make_body(glm::vec3 position, glm::vec3 half_extents,
                                  float mass, std::uint32_t material = 1);
void apply_force(RigidBody& body, glm::vec3 force) noexcept;
}

namespace nuc::ecs {
// Stable deterministic hash for persisted rigid-body state; transient force is excluded.
template<> struct ComponentStateTraits<nuc::physics::RigidBody> {
    static constexpr bool registered = true;
    static constexpr std::uint64_t id = 0x50485953424F4459ULL;
    static void append(sim::StateHash& hash, const nuc::physics::RigidBody& body) noexcept;
};
}
