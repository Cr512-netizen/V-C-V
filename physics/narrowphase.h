#pragma once
#include "physics/body.h"
#include "world/svdag.h"
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace nuc::physics {
struct Contact {
    std::size_t a{}, b{};
    glm::vec3 normal{0.0F};
    glm::vec3 point{0.0F};
    float penetration{};
};
using SolidVoxelQuery = std::function<bool(int,int,int)>;
// Three-axis SAT for axis-aligned boxes; normal points from body A to body B.
[[nodiscard]] std::optional<Contact> box_box_sat(const RigidBody& a,const RigidBody& b,
                                                  std::size_t index_a,std::size_t index_b);
[[nodiscard]] bool point_vs_svdag(glm::ivec3 local_point,const world::Svdag& dag);
// Resolves one body's AABB against solid unit voxels without graphics dependencies.
void resolve_body_vs_voxels(RigidBody& body,const SolidVoxelQuery& solid,double dt);
}
