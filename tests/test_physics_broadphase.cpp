#include <catch2/catch_test_macros.hpp>
#include "physics/broadphase.h"

TEST_CASE("uniform grid broadphase emits sorted unique pairs across cells") {
    std::vector<nuc::physics::RigidBody> bodies;
    bodies.push_back(nuc::physics::make_body({-0.5F,0,0},{1,1,1},1));
    bodies.push_back(nuc::physics::make_body({0.2F,0,0},{1,1,1},1));
    bodies.push_back(nuc::physics::make_body({8,0,0},{0.5F,0.5F,0.5F},1));
    const auto pairs=nuc::physics::build_pairs(bodies,0.5F);
    REQUIRE(pairs.size()==1);
    REQUIRE((pairs[0]==nuc::physics::BodyPair{0,1}));
}
TEST_CASE("uniform grid uses floor for negative coordinates and skips static pairs") {
    std::vector<nuc::physics::RigidBody> bodies;
    bodies.push_back(nuc::physics::make_body({-4.0F,0,0},{0.5F,0.5F,0.5F},0));
    bodies.push_back(nuc::physics::make_body({-3.5F,0,0},{0.5F,0.5F,0.5F},0));
    bodies.push_back(nuc::physics::make_body({-3.25F,0,0},{0.5F,0.5F,0.5F},1));
    const auto pairs=nuc::physics::build_pairs(bodies,1.0F);
    REQUIRE(pairs.size()==2);
    REQUIRE((pairs[0]==nuc::physics::BodyPair{0,2}));
    REQUIRE((pairs[1]==nuc::physics::BodyPair{1,2}));
    REQUIRE_THROWS(nuc::physics::build_pairs(bodies,0.0F));
}
