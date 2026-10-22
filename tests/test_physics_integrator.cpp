#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "physics/integrator.h"

TEST_CASE("integrator uses semi implicit Euler and clears force") {
    auto body=nuc::physics::make_body({0,0,0},{0.5F,0.5F,0.5F},1.0F);
    nuc::physics::apply_force(body,{10,0,0});
    nuc::physics::integrate(body,0.1,{0,-10,0});
    REQUIRE(body.velocity.x==Catch::Approx(1.0F));
    REQUIRE(body.velocity.y==Catch::Approx(-1.0F));
    REQUIRE(body.position.x==Catch::Approx(0.1F));
    REQUIRE(body.position.y==Catch::Approx(-0.1F));
    REQUIRE(glm::length(body.previous_position)<1e-6F);
    REQUIRE(glm::length(body.force)<1e-6F);
}
TEST_CASE("integrator leaves static sleeping and invalid-step bodies safe") {
    auto fixed=nuc::physics::make_body({1,2,3},{1,1,1},0.0F);
    nuc::physics::integrate(fixed,0.1,{0,-9.81F,0});
    REQUIRE(glm::length(fixed.position-glm::vec3(1,2,3))<1e-6F);
    auto sleeping=nuc::physics::make_body({1,2,3},{1,1,1},1.0F);
    sleeping.awake=false;
    nuc::physics::integrate(sleeping,0.1,{0,-9.81F,0});
    REQUIRE(glm::length(sleeping.position-glm::vec3(1,2,3))<1e-6F);
    auto body=nuc::physics::make_body({1,2,3},{1,1,1},1.0F);
    nuc::physics::integrate(body,0.0,{0,-9.81F,0});
    REQUIRE(glm::length(body.position-glm::vec3(1,2,3))<1e-6F);
}
