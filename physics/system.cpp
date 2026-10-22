#include "physics/system.h"
#include "core/ecs/view.h"
#include "physics/broadphase.h"
#include "physics/integrator.h"
#include "physics/solver.h"
#include <algorithm>
#include <cmath>

namespace nuc::physics {
void PhysicsSystem::tick(ecs::World& world,double logic_dt,const SolidVoxelQuery& solid) {
    if(!std::isfinite(logic_dt)||logic_dt<=0.0) return;
    std::vector<RigidBody*> components;
    ecs::view<RigidBody>(world).each([&](ecs::Entity,RigidBody& body){components.push_back(&body);});
    if(components.empty()) return;
    std::vector<RigidBody> bodies;
    bodies.reserve(components.size());
    for(auto* body:components)bodies.push_back(*body);
    const double step=logic_dt/static_cast<double>(substeps);
    constexpr glm::vec3 gravity(0.0F,-9.81F,0.0F);
    // Interpolation must span the whole tick, so the tick-start positions are kept
    // aside; per-substep saves inside integrate() are overwritten by this snapshot.
    std::vector<glm::vec3> tick_start;
    tick_start.reserve(bodies.size());
    for(const auto& body:bodies)tick_start.push_back(body.position);
    for(unsigned substep=0;substep<substeps;++substep) {
        for(auto& body:bodies)integrate(body,step,gravity);
        const auto pairs=build_pairs(bodies);
        std::vector<Contact> contacts;
        contacts.reserve(pairs.size());
        for(const auto [a,b]:pairs) {
            if(const auto contact=box_box_sat(bodies[a],bodies[b],a,b))contacts.push_back(*contact);
        }
        solve_contacts(bodies,contacts,step);
        for(auto& body:bodies)resolve_body_vs_voxels(body,solid,step);
        for(auto& body:bodies) {
            if(body.inverse_mass<=0.0F)continue;
            if(glm::dot(body.velocity,body.velocity)<0.0025F) {
                body.sleep_time+=static_cast<float>(step);
                if(body.sleep_time>=5.0F){body.awake=false;body.velocity=glm::vec3(0.0F);}
            } else {body.sleep_time=0.0F;body.awake=true;}
        }
    }
    for(std::size_t i=0;i<bodies.size();++i)bodies[i].previous_position=tick_start[i];
    for(std::size_t i=0;i<components.size();++i)*components[i]=bodies[i];
}
glm::vec3 interpolated_position(const RigidBody& body,float alpha) noexcept {
    const float t=std::clamp(std::isfinite(alpha)?alpha:0.0F,0.0F,1.0F);
    return glm::mix(body.previous_position,body.position,t);
}
}
