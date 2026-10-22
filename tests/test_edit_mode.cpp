#include <catch2/catch_test_macros.hpp>
#include "app/edit_mode.h"
#include <utility>
#include <vector>

TEST_CASE("edit mode places, queries and breaks a block through the world overlay") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.emplace(std::pair{0,0},nuc::world::Chunk(0,0));
    chunks.at({0,0}).set(2,1,0,4);
    nuc::world::ChunkManager manager(chunks);
    nuc::EditMode edit(manager,3);
    std::vector<std::pair<nuc::world::VoxelCoord,std::uint32_t>> broken;
    edit.set_break_callback([&](auto position,auto material){broken.emplace_back(position,material);});
    const glm::vec3 origin{0.5F,1.5F,0.5F};
    const glm::vec3 direction{1,0,0};
    REQUIRE(edit.apply(nuc::EditAction::place_block,origin,direction,8));
    REQUIRE(manager.get_voxel({1,1,0})==3);
    REQUIRE(edit.apply(nuc::EditAction::break_block,origin,direction,8));
    REQUIRE(manager.get_voxel({1,1,0})==0);
    REQUIRE(manager.get_voxel({2,1,0})==4);
    REQUIRE(edit.apply(nuc::EditAction::break_block,origin,direction,8));
    REQUIRE(manager.get_voxel({2,1,0})==0);
    REQUIRE(manager.delta().get({2,1,0})==0);
    REQUIRE(broken.size()==2);
    REQUIRE(broken[0].first.x==1);
    REQUIRE(broken[0].second==3);
    REQUIRE(broken[1].first.x==2);
    REQUIRE(broken[1].second==4);
}
TEST_CASE("edit mode ignores misses and refuses to place inside solid voxels") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.emplace(std::pair{0,0},nuc::world::Chunk(0,0));
    chunks.at({0,0}).set(0,1,0,4);
    nuc::world::ChunkManager manager(chunks);
    nuc::EditMode edit(manager,3);
    REQUIRE_FALSE(edit.apply(nuc::EditAction::place_block,{0.5F,1.5F,0.5F},{1,0,0},5));
    REQUIRE_FALSE(edit.apply(nuc::EditAction::break_block,{10.5F,1.5F,10.5F},{1,0,0},2));
}
