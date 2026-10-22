#include "physics/solver.h"
#include <algorithm>
#include <cmath>

namespace nuc::physics {
void solve_contacts(std::span<RigidBody> bodies,std::span<const Contact> contacts,
                    double dt,unsigned iterations) {
    if(!std::isfinite(dt)||dt<=0.0||iterations==0) return;
    for(unsigned iteration=0;iteration<iterations;++iteration) for(const auto& c:contacts) {
        if(c.a>=bodies.size()||c.b>=bodies.size()||c.a==c.b||
           !std::isfinite(c.penetration)||c.penetration<0.0F) continue;
        auto& a=bodies[c.a];auto& b=bodies[c.b];
        const float inv_sum=a.inverse_mass+b.inverse_mass;
        if(inv_sum<=0.0F) continue;
        const float nlen=glm::length(c.normal);
        if(!std::isfinite(nlen)||nlen<0.99F||nlen>1.01F) continue;
        const glm::vec3 relative=b.velocity-a.velocity;
        const float vn=glm::dot(relative,c.normal);
        float normal_impulse=0.0F;
        if(vn<0.0F) {
            const float restitution=std::abs(vn)<1.0F?0.0F:std::clamp(std::max(a.restitution,b.restitution),0.0F,1.0F);
            normal_impulse=-(1.0F+restitution)*vn/inv_sum;
            const glm::vec3 impulse=normal_impulse*c.normal;
            if(a.inverse_mass>0.0F){a.velocity-=impulse*a.inverse_mass;if(!a.awake&&-vn>0.5F){a.awake=true;a.sleep_time=0.0F;}}
            if(b.inverse_mass>0.0F){b.velocity+=impulse*b.inverse_mass;if(!b.awake&&-vn>0.5F){b.awake=true;b.sleep_time=0.0F;}}
            const glm::vec3 after=b.velocity-a.velocity;
            const glm::vec3 tangent_raw=after-c.normal*glm::dot(after,c.normal);
            const float tangent_length=glm::length(tangent_raw);
            if(tangent_length>1e-6F) {
                const glm::vec3 tangent=tangent_raw/tangent_length;
                float jt=-glm::dot(after,tangent)/inv_sum;
                const float mu=std::sqrt(std::max(0.0F,a.friction*b.friction));
                jt=std::clamp(jt,-normal_impulse*mu,normal_impulse*mu);
                const glm::vec3 friction_impulse=jt*tangent;
                if(a.inverse_mass>0.0F)a.velocity-=friction_impulse*a.inverse_mass;
                if(b.inverse_mass>0.0F)b.velocity+=friction_impulse*b.inverse_mass;
            }
        }
        if(iteration==0&&c.penetration>0.001F) {
            const glm::vec3 correction=c.normal*(0.2F*(c.penetration-0.001F)/inv_sum);
            if(a.inverse_mass>0.0F)a.position-=correction*a.inverse_mass;
            if(b.inverse_mass>0.0F)b.position+=correction*b.inverse_mass;
        }
    }
}
}
