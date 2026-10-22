#include <catch2/catch_test_macros.hpp>
#include "world/delta.h"

TEST_CASE("delta adds, changes and removes sparse overrides") {
    nuc::world::VoxelDelta delta;
    const nuc::world::VoxelCoord p{2,3,4};
    REQUIRE_FALSE(delta.get(p));
    delta.set(p,2,1);
    REQUIRE(delta.get(p)==2);
    delta.set(p,3,1);
    REQUIRE(delta.get(p)==3);
    delta.erase(p);
    REQUIRE_FALSE(delta.get(p));
    REQUIRE(delta.size()==0);
}
TEST_CASE("delta retains air overrides and removes redundant base values") {
    nuc::world::VoxelDelta delta;
    const nuc::world::VoxelCoord p{-1,2,32};
    delta.set(p,0,4);
    REQUIRE(delta.get(p)==0);
    delta.set(p,4,4);
    REQUIRE_FALSE(delta.get(p));
}
TEST_CASE("delta overlays only matching chunk coordinates") {
    nuc::world::Chunk base(-1,0);
    base.set(31,1,0,4);
    nuc::world::VoxelDelta delta;
    delta.set({-1,1,0},0,4);
    delta.set({0,1,0},2,0);
    const auto result=delta.apply_to(base);
    REQUIRE(result.at(31,1,0)==0);
    REQUIRE(base.at(31,1,0)==4);
    REQUIRE(result.chunk_x()==-1);
    REQUIRE(result.at(0,1,0)==0);
}

TEST_CASE("delta entries are sorted snapshots and preserve air overrides") {
    nuc::world::VoxelDelta delta;
    delta.set({2,0,-1},0,4);
    delta.set({-3,5,8},7,0);
    delta.set({-3,1,9},2,0);
    const auto snapshot=delta.entries();
    REQUIRE(snapshot.size()==3);
    REQUIRE((snapshot[0].first==nuc::world::VoxelCoord{-3,1,9}));
    REQUIRE((snapshot[1].first==nuc::world::VoxelCoord{-3,5,8}));
    REQUIRE((snapshot[2].first==nuc::world::VoxelCoord{2,0,-1}));
    REQUIRE(snapshot[2].second==0);
    delta.erase({-3,1,9});
    REQUIRE(snapshot.size()==3);
}
