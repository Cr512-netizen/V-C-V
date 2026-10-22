#include "physics/body.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace nuc::physics {
namespace {
bool finite(glm::vec3 v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
}
RigidBody make_body(glm::vec3 position,glm::vec3 half_extents,float mass,std::uint32_t material) {
    if(!finite(position)||!finite(half_extents)||!std::isfinite(mass)||mass<0.0F||
       half_extents.x<=0.0F||half_extents.y<=0.0F||half_extents.z<=0.0F)
        throw std::invalid_argument("invalid rigid body parameters");
    RigidBody body;
    body.position=body.previous_position=position;
    body.half_extents=half_extents;
    body.inverse_mass=mass>0.0F?1.0F/mass:0.0F;
    body.material=material;
    // Construction always yields sanitised material parameters, so a body can never
    // start outside the physical range the solver and voxel resolver assume.
    body.restitution=std::clamp(body.restitution,0.0F,1.0F);
    body.friction=std::max(0.0F,body.friction);
    return body;
}
void apply_force(RigidBody& body,glm::vec3 force) noexcept {
    if(!finite(force)) return;
    body.force+=force;
    if(glm::dot(force,force)>0.0F) body.awake=true;
}
}

namespace nuc::ecs {
namespace {
void scalar(sim::StateHash& h,float value) noexcept {
    h.integer(std::bit_cast<std::uint32_t>(value));
}
void vector(sim::StateHash& h,glm::vec3 value) noexcept {
    scalar(h,value.x);scalar(h,value.y);scalar(h,value.z);
}
}
void ComponentStateTraits<nuc::physics::RigidBody>::append(
    sim::StateHash& hash,const nuc::physics::RigidBody& body) noexcept {
    vector(hash,body.position);vector(hash,body.previous_position);vector(hash,body.velocity);
    vector(hash,body.half_extents);scalar(hash,body.inverse_mass);scalar(hash,body.restitution);
    scalar(hash,body.friction);scalar(hash,body.sleep_time);
    hash.integer<std::uint8_t>(body.awake?1:0);hash.integer(body.material);
    hash.integer(body.owner_chunk_x);hash.integer(body.owner_chunk_z);
}
}
