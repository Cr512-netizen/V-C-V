#include <catch2/catch_test_macros.hpp>
#include "physics/body.h"
#include <cmath>

TEST_CASE("rigid body validates construction and supports static bodies") {
    const auto body=nuc::physics::make_body({1,2,3},{0.5F,1,2},2.0F,7);
    REQUIRE(body.inverse_mass==0.5F);
    REQUIRE(body.material==7);
    REQUIRE(glm::length(body.position-body.previous_position)<1e-6F);
    const auto fixed=nuc::physics::make_body({0,0,0},{1,1,1},0.0F);
    REQUIRE(fixed.inverse_mass==0.0F);
    REQUIRE_THROWS(nuc::physics::make_body({0,0,0},{0,1,1},1.0F));
    REQUIRE_THROWS(nuc::physics::make_body({0,0,0},{1,1,1},-1.0F));
}
TEST_CASE("forces wake bodies and transient force is excluded from stable ECS hash") {
    nuc::ecs::World world;
    const auto entity=world.create();
    auto& body=world.emplace<nuc::physics::RigidBody>(entity,
        nuc::physics::make_body({0,0,0},{0.5F,0.5F,0.5F},1.0F));
    // The body starts awake, so applying force must not disturb the hashed state:
    // only the transient accumulated force changes, and that field is excluded.
    const auto before=world.deterministic_hash();
    nuc::physics::apply_force(body,{2,0,0});
    REQUIRE(body.awake);
    REQUIRE(body.force.x==2.0F);
    REQUIRE(world.deterministic_hash()==before);
    // The sleep flag is persisted state, so waking is expected to change the hash.
    body.awake=false;
    REQUIRE(world.deterministic_hash()!=before);
    body.awake=true;
    REQUIRE(world.deterministic_hash()==before);
    body.position.x=4.0F;
    REQUIRE(world.deterministic_hash()!=before);
}
