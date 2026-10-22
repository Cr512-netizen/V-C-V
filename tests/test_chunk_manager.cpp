#include <catch2/catch_test_macros.hpp>
#include "world/chunk_manager.h"
#include <chrono>
#include <thread>

namespace {
std::vector<nuc::world::ChunkManager::MeshUpdate> wait_meshes(nuc::world::ChunkManager& manager) {
    std::vector<nuc::world::ChunkManager::MeshUpdate> all;
    for(int i=0;i<1000;++i) {
        if(manager.dirty_count()) manager.rebuild_dirty_at_tick_end();
        auto ready=manager.drain_upload_queue_on_render_thread();
        for(auto& mesh:ready) all.push_back(std::move(mesh));
        if(!manager.pending_count()&&!manager.dirty_count()) return all;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return all;
}
}
TEST_CASE("chunk manager maps negative world coordinates and overlays edits") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.emplace(std::pair{-1,0},nuc::world::Chunk(-1,0));
    chunks.at({-1,0}).set(31,1,0,4);
    nuc::world::ChunkManager manager(chunks);
    REQUIRE(manager.get_voxel({-1,1,0})==4);
    REQUIRE(manager.set_voxel({-1,1,0},0));
    REQUIRE(manager.get_voxel({-1,1,0})==0);
    REQUIRE(chunks.at({-1,0}).at(31,1,0)==4);
}
TEST_CASE("chunk manager marks adjacent chunk dirty at an X boundary") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.try_emplace(std::pair{0,0},0,0);
    chunks.try_emplace(std::pair{1,0},1,0);
    nuc::world::ChunkManager manager(chunks);
    REQUIRE(manager.set_voxel({31,2,4},1));
    REQUIRE(manager.dirty_count()==2);
    manager.rebuild_dirty_at_tick_end();
    const auto meshes=wait_meshes(manager);
    REQUIRE(meshes.size()==2);
    REQUIRE(manager.dirty_count()==0);
}
TEST_CASE("chunk manager batches repeated edits into one rebuild per chunk per tick") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.try_emplace(std::pair{0,0},0,0);
    nuc::world::ChunkManager manager(chunks);
    REQUIRE(manager.set_voxel({2,2,2},1));
    REQUIRE(manager.set_voxel({3,2,2},1));
    REQUIRE(manager.dirty_count()==1);
    manager.rebuild_dirty_at_tick_end();
    const auto meshes=wait_meshes(manager);
    REQUIRE(meshes.size()==1);
    REQUIRE(meshes.front().mesh.quad_count()==10);
    REQUIRE(manager.dirty_count()==0);
}

TEST_CASE("chunk manager discards stale asynchronous mesh results") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.try_emplace(std::pair{0,0},0,0);
    nuc::world::ChunkManager manager(chunks);
    REQUIRE(manager.set_voxel({2,2,2},1));
    manager.rebuild_dirty_at_tick_end();
    REQUIRE(manager.set_voxel({3,2,2},1));
    manager.rebuild_dirty_at_tick_end();
    const auto meshes=wait_meshes(manager);
    REQUIRE_FALSE(meshes.empty());
    for(const auto& result:meshes) REQUIRE(result.mesh.quad_count()==10);
}
