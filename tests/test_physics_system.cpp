#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "physics/system.h"
#include <cmath>

TEST_CASE("physics system runs four substeps and interpolation is read only") {
    nuc::ecs::World world;
    const auto id=world.create();
    auto& body=world.emplace<nuc::physics::RigidBody>(id,
        nuc::physics::make_body({0,10,0},{0.5F,0.5F,0.5F},1));
    nuc::physics::PhysicsSystem::tick(world,1.0/15.0);
    const float expected=10.0F-9.81F*(1.0F/60.0F)*(1.0F/60.0F)*10.0F;
    REQUIRE(body.position.y==Catch::Approx(expected).margin(0.0001F));
    const auto position=body.position;
    const auto halfway=nuc::physics::interpolated_position(body,0.5F);
    REQUIRE(halfway.y==Catch::Approx((body.previous_position.y+body.position.y)*0.5F));
    REQUIRE(glm::length(body.position-position)<1e-6F);
    REQUIRE(glm::length(nuc::physics::interpolated_position(body,2.0F)-body.position)<1e-6F);
}
TEST_CASE("interpolation spans the whole tick and is read only") {
    nuc::ecs::World world;
    const auto id=world.create();
    auto& body=world.emplace<nuc::physics::RigidBody>(id,
        nuc::physics::make_body({0,10,0},{0.5F,0.5F,0.5F},1));
    const auto start=body.position;
    nuc::physics::PhysicsSystem::tick(world,1.0/15.0);
    REQUIRE(glm::length(body.previous_position-start)<1e-6F);
    REQUIRE(glm::length(nuc::physics::interpolated_position(body,0.0F)-start)<1e-6F);
    REQUIRE(glm::length(nuc::physics::interpolated_position(body,1.0F)-body.position)<1e-6F);
    const auto position=body.position;
    REQUIRE(glm::length(body.position-position)<1e-6F);
}

TEST_CASE("physics body falls, bounces, settles and sleeps on a floor") {
    nuc::ecs::World world;
    const auto floor=world.create();
    world.emplace<nuc::physics::RigidBody>(floor,
        nuc::physics::make_body({0,-0.5F,0},{10,0.5F,10},0));
    const auto cube=world.create();
    auto& body=world.emplace<nuc::physics::RigidBody>(cube,
        nuc::physics::make_body({0,2,0},{0.5F,0.5F,0.5F},1));
    body.restitution=0.45F;
    bool bounced=false;
    for(int i=0;i<15*12;++i) {
        nuc::physics::PhysicsSystem::tick(world,1.0/15.0);
        if(body.velocity.y>0.15F)bounced=true;
    }
    REQUIRE(bounced);
    REQUIRE(body.position.y>=0.48F);
    REQUIRE_FALSE(body.awake);
    REQUIRE(body.sleep_time>=5.0F);
}
