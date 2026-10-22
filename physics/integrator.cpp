#include "physics/integrator.h"
#include <cmath>

namespace nuc::physics {
void integrate(RigidBody& body,double dt,glm::vec3 gravity) {
    if(!std::isfinite(dt)||dt<=0.0||!std::isfinite(gravity.x)||
       !std::isfinite(gravity.y)||!std::isfinite(gravity.z)) return;
    if(body.inverse_mass<=0.0F||!body.awake) { body.force=glm::vec3(0.0F); return; }
    body.previous_position=body.position;
    const float step=static_cast<float>(dt);
    body.velocity+=(gravity+body.force*body.inverse_mass)*step;
    body.position+=body.velocity*step;
    body.force=glm::vec3(0.0F);
}
}
