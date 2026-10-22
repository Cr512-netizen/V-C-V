#include <catch2/catch_test_macros.hpp>
#include "render/chunk_mesh.h"

TEST_CASE("chunk mesher emits six faces for an isolated voxel") {
    nuc::world::Chunk chunk;
    chunk.set(2,3,4,7);
    const auto mesh=nuc::render::build_chunk_mesh(chunk);
    REQUIRE(mesh.quad_count()==6);
    REQUIRE(mesh.vertices.size()==24);
    REQUIRE(mesh.indices.size()==36);
    REQUIRE(mesh.vertices.front().material==7);
}
TEST_CASE("chunk mesher culls faces between adjacent solid voxels") {
    nuc::world::Chunk chunk;
    chunk.set(2,3,4,1);
    chunk.set(3,3,4,2);
    const auto mesh=nuc::render::build_chunk_mesh(chunk);
    REQUIRE(mesh.quad_count()==10);
    REQUIRE(mesh.vertices.size()==40);
    REQUIRE(mesh.indices.size()==60);
}
TEST_CASE("chunk mesher culls shared faces across loaded chunk boundaries") {
    nuc::world::Chunk west(-1,0), east(0,0);
    west.set(31,5,7,1);
    east.set(0,5,7,1);
    nuc::render::ChunkNeighbors wn; wn.east=&east;
    nuc::render::ChunkNeighbors en; en.west=&west;
    REQUIRE(nuc::render::build_chunk_mesh(west,wn).quad_count()==5);
    REQUIRE(nuc::render::build_chunk_mesh(east,en).quad_count()==5);
}
TEST_CASE("empty chunk produces no mesh") {
    nuc::world::Chunk chunk;
    const auto mesh=nuc::render::build_chunk_mesh(chunk);
    REQUIRE(mesh.vertices.empty());
    REQUIRE(mesh.indices.empty());
}
