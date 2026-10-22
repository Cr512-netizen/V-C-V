#pragma once
#include "physics/narrowphase.h"
#include <span>

namespace nuc::physics {
// Resolves contacts with sequential normal and Coulomb-friction impulses.
void solve_contacts(std::span<RigidBody> bodies,std::span<const Contact> contacts,
                    double dt,unsigned iterations=8);
}
