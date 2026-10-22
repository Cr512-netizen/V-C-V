#include <catch2/catch_test_macros.hpp>
#include "world/raycast.h"
#include <cmath>
#include <unordered_map>

namespace {
using namespace nuc::world;
auto map_reader(const std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash>& voxels) {
    return [&voxels](int x,int y,int z) {
        const auto it=voxels.find({x,y,z});
        return it==voxels.end()?0U:it->second;
    };
}
}
TEST_CASE("DDA hits axis aligned voxel with previous air and face normal") {
    std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash> voxels{{{3,1,1},7}};
    const auto hit=raycast(map_reader(voxels),{0.5F,1.5F,1.5F},{1,0,0},10);
    REQUIRE(hit);
    REQUIRE(hit->voxel==(VoxelCoord{3,1,1}));
    REQUIRE(hit->previous_air==(VoxelCoord{2,1,1}));
    REQUIRE(hit->normal==(VoxelCoord{-1,0,0}));
    REQUIRE(hit->material==7);
    REQUIRE(hit->has_previous_air);
    REQUIRE(std::abs(hit->distance-2.5F)<0.0001F);
}
TEST_CASE("DDA handles negative world coordinates") {
    std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash> voxels{{{-3,0,0},2}};
    const auto hit=raycast(map_reader(voxels),{-0.5F,0.5F,0.5F},{-1,0,0},5);
    REQUIRE(hit);
    REQUIRE(hit->voxel==(VoxelCoord{-3,0,0}));
    REQUIRE(hit->normal==(VoxelCoord{1,0,0}));
}
TEST_CASE("DDA reports origin inside solid without a placement cell") {
    std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash> voxels{{{0,0,0},1}};
    const auto hit=raycast(map_reader(voxels),{0.2F,0.2F,0.2F},{1,0,0},5);
    REQUIRE(hit);
    REQUIRE_FALSE(hit->has_previous_air);
    REQUIRE(hit->distance==0.0F);
}
TEST_CASE("DDA misses air beyond max distance and rejects invalid rays") {
    const std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash> voxels;
    const auto read=map_reader(voxels);
    REQUIRE_FALSE(raycast(read,{0.5F,0.5F,0.5F},{1,0,0},0.2F));
    REQUIRE_FALSE(raycast(read,{0,0,0},{0,0,0},10));
    REQUIRE_FALSE(raycast(read,{0,0,0},{1,0,0},-1));
    REQUIRE_FALSE(raycast(read,{NAN,0,0},{1,0,0},10));
}
TEST_CASE("DDA chooses deterministic X axis on exact edge ties") {
    std::unordered_map<VoxelCoord,std::uint32_t,VoxelCoordHash> voxels{{{1,0,0},3}};
    const auto hit=raycast(map_reader(voxels),{0.5F,0.5F,0.5F},{1,1,0},5);
    REQUIRE(hit);
    REQUIRE(hit->voxel==(VoxelCoord{1,0,0}));
}
