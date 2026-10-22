#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "physics/solver.h"
#include <array>
#include <cmath>

TEST_CASE("sequential impulses conserve momentum in elastic two body collision") {
    auto a=nuc::physics::make_body({0,0,0},{0.5F,0.5F,0.5F},1);
    auto b=nuc::physics::make_body({1,0,0},{0.5F,0.5F,0.5F},1);
    a.restitution=b.restitution=1.0F;
    a.friction=b.friction=0.0F;
    a.velocity={1,0,0};b.velocity={-1,0,0};
    const float before=a.velocity.x+b.velocity.x;
    const std::array contacts{nuc::physics::Contact{0,1,{1,0,0},{0.5F,0,0},0}};
    std::array bodies{a,b};
    nuc::physics::solve_contacts(bodies,contacts,1.0/60.0);
    REQUIRE(bodies[0].velocity.x==Catch::Approx(-1.0F));
    REQUIRE(bodies[1].velocity.x==Catch::Approx(1.0F));
    REQUIRE(bodies[0].velocity.x+bodies[1].velocity.x==Catch::Approx(before).margin(0.01F));
}
TEST_CASE("solver keeps static bodies fixed and friction reduces tangent speed") {
    auto dynamic=nuc::physics::make_body({0,1,0},{0.5F,0.5F,0.5F},1);
    auto fixed=nuc::physics::make_body({0,0,0},{1,0.5F,1},0);
    dynamic.restitution=0.0F;dynamic.friction=1.0F;
    dynamic.velocity={1,-1,0};
    const auto fixed_position=fixed.position;
    const std::array contacts{nuc::physics::Contact{0,1,{0,-1,0},{0,0.5F,0},0.1F}};
    std::array bodies{dynamic,fixed};
    nuc::physics::solve_contacts(bodies,contacts,1.0/60.0);
    REQUIRE(glm::length(bodies[1].position-fixed_position)<1e-6F);
    REQUIRE(std::abs(bodies[0].velocity.x)<1.0F);
    REQUIRE(bodies[0].velocity.y>=0.0F);
}

TEST_CASE("impact wakes a sleeping body") {
    auto sleeping=nuc::physics::make_body({0,0,0},{0.5F,0.5F,0.5F},1);
    auto moving=nuc::physics::make_body({-0.9F,0,0},{0.5F,0.5F,0.5F},1);
    sleeping.awake=false;sleeping.sleep_time=5.0F;
    moving.velocity={2,0,0};
    const std::array contacts{nuc::physics::Contact{0,1,{-1,0,0},{-0.45F,0,0},0.1F}};
    std::array bodies{sleeping,moving};
    nuc::physics::solve_contacts(bodies,contacts,1.0/60.0);
    REQUIRE(bodies[0].awake);
    REQUIRE(bodies[0].sleep_time==0.0F);
}
