#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "physics/narrowphase.h"
#include <cmath>
#include <vector>

TEST_CASE("AABB SAT returns minimum penetration and rejects separated boxes") {
    auto a=nuc::physics::make_body({0,0,0},{0.5F,0.5F,0.5F},1);
    auto b=nuc::physics::make_body({0.8F,0,0},{0.5F,0.5F,0.5F},1);
    const auto hit=nuc::physics::box_box_sat(a,b,2,4);
    REQUIRE(hit.has_value());
    REQUIRE(hit->a==2);
    REQUIRE(hit->b==4);
    REQUIRE(hit->normal.x==1.0F);
    REQUIRE(hit->penetration==Catch::Approx(0.2F));
    b.position.x=1.1F;
    REQUIRE_FALSE(nuc::physics::box_box_sat(a,b,0,1).has_value());
    b.position.x=1.0F;
    const auto touching=nuc::physics::box_box_sat(a,b,0,1);
    REQUIRE(touching.has_value());
    REQUIRE(touching->penetration==0.0F);
}
TEST_CASE("SVDAG point query distinguishes solid air and out of bounds") {
    std::vector<std::uint32_t> voxels(64,0);
    voxels[1+4*(1+4*1)]=3;
    nuc::world::Svdag dag;
    dag.build(4,voxels);
    REQUIRE(nuc::physics::point_vs_svdag({1,1,1},dag));
    REQUIRE_FALSE(nuc::physics::point_vs_svdag({0,0,0},dag));
    REQUIRE_FALSE(nuc::physics::point_vs_svdag({4,0,0},dag));
}
TEST_CASE("voxel query resolves an overlapping body out of a solid cell") {
    auto body=nuc::physics::make_body({0.5F,0.8F,0.5F},{0.4F,0.4F,0.4F},1);
    nuc::physics::resolve_body_vs_voxels(body,[](int x,int y,int z){return x==0&&y==0&&z==0;},1.0/60.0);
    REQUIRE(body.position.y>0.9F);
}
TEST_CASE("a sleeping body wakes when its support voxel disappears") {
    auto body=nuc::physics::make_body({0.5F,1.5F,0.5F},{0.4F,0.4F,0.4F},1);
    body.awake=false;body.sleep_time=5.0F;
    nuc::physics::resolve_body_vs_voxels(body,[](int,int y,int){return y==0;},1.0/60.0);
    REQUIRE_FALSE(body.awake);
    REQUIRE(body.sleep_time==5.0F);
    nuc::physics::resolve_body_vs_voxels(body,[](int,int,int){return false;},1.0/60.0);
    REQUIRE(body.awake);
    REQUIRE(body.sleep_time==0.0F);
}
TEST_CASE("voxel resolution clamps an out of range restitution") {
    auto body=nuc::physics::make_body({0.5F,0.8F,0.5F},{0.4F,0.4F,0.4F},1);
    body.restitution=5.0F;
    body.velocity={0.0F,-1.0F,0.0F};
    nuc::physics::resolve_body_vs_voxels(body,[](int x,int y,int z){return x==0&&y==0&&z==0;},1.0/60.0);
    REQUIRE(std::abs(body.velocity.y)<=1.0F+1e-4F);
}
