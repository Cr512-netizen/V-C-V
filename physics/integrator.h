#pragma once
#include "physics/body.h"

namespace nuc::physics {
// Advances one awake dynamic body by one fixed physics substep using semi-implicit Euler.
void integrate(RigidBody& body, double dt, glm::vec3 gravity);
}
