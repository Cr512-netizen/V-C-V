#include <catch2/catch_test_macros.hpp>
#include "physics/destruction.h"
#include <algorithm>

TEST_CASE("block fracture creates boundary rigid bodies and collapses the interior") {
    const auto first=nuc::physics::fracture_block({-1,4,2},3,99,{0,2,0});
    const auto second=nuc::physics::fracture_block({-1,4,2},3,99,{0,2,0});
    REQUIRE(first.boundary_bodies.size()==296);
    REQUIRE(first.collapsed_interior==216);
    REQUIRE(first.interior_material==3);
    REQUIRE(first.boundary_bodies.size()==second.boundary_bodies.size());
    for(std::size_t i=0;i<first.boundary_bodies.size();++i) {
        REQUIRE(glm::length(first.boundary_bodies[i].position-second.boundary_bodies[i].position)<1e-6F);
        REQUIRE(glm::length(first.boundary_bodies[i].velocity-second.boundary_bodies[i].velocity)<1e-6F);
        REQUIRE(first.boundary_bodies[i].material==3);
        REQUIRE(first.boundary_bodies[i].owner_chunk_x==-1);
    }
}
TEST_CASE("fracture seed depends only on world seed tick and block coordinates") {
    const auto a=nuc::physics::fracture_seed(1234,500,-7,12,9);
    REQUIRE(a==nuc::physics::fracture_seed(1234,500,-7,12,9));
    // Any change of an input must change the seed, and negation must not alias.
    REQUIRE(a!=nuc::physics::fracture_seed(1234,501,-7,12,9));
    REQUIRE(a!=nuc::physics::fracture_seed(1235,500,-7,12,9));
    REQUIRE(a!=nuc::physics::fracture_seed(1234,500,7,12,9));
    REQUIRE(a!=nuc::physics::fracture_seed(1234,500,-7,13,9));
    REQUIRE(a!=nuc::physics::fracture_seed(1234,500,-7,12,10));
    // Same inputs must also give identical debris, which is what replay relies on.
    const auto first=nuc::physics::fracture_block({-7,12,9},3,nuc::physics::fracture_seed(1234,500,-7,12,9),{0,2,0});
    const auto second=nuc::physics::fracture_block({-7,12,9},3,nuc::physics::fracture_seed(1234,500,-7,12,9),{0,2,0});
    for(std::size_t i=0;i<first.boundary_bodies.size();++i) {
        REQUIRE(glm::length(first.boundary_bodies[i].position-second.boundary_bodies[i].position)<1e-6F);
        REQUIRE(glm::length(first.boundary_bodies[i].velocity-second.boundary_bodies[i].velocity)<1e-6F);
    }
}
TEST_CASE("debris pool enforces a per chunk cap and releases reservations") {
    auto fracture=nuc::physics::fracture_block({0,0,0},2,5,{0,1,0});
    nuc::physics::DebrisPool pool(100);
    const auto report=pool.reserve(fracture.boundary_bodies);
    REQUIRE(report.accepted==100);
    REQUIRE(report.rejected==196);
    REQUIRE(report.accepted_indices.size()==100);
    REQUIRE(pool.count(0,0)==100);
    pool.release(0,0,40);
    REQUIRE(pool.count(0,0)==60);
    const auto rest=pool.reserve(std::span(fracture.boundary_bodies).subspan(100));
    REQUIRE(rest.accepted==40);
    REQUIRE(pool.count(0,0)==100);
}
