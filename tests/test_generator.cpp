#include <catch2/catch_test_macros.hpp>
#include "world/generator.h"
#include <algorithm>
#include <cstdint>

TEST_CASE("chunk coordinates and bounds are handled") {
    nuc::world::Chunk chunk(-2, 3);
    REQUIRE(chunk.chunk_x() == -2);
    REQUIRE(chunk.chunk_z() == 3);
    REQUIRE(chunk.at(0,0,0) == 0);
    chunk.set(31,31,31,9);
    REQUIRE(chunk.at(31,31,31) == 9);
    REQUIRE_THROWS(chunk.at(32,0,0));
    REQUIRE_THROWS(chunk.set(0,32,0,1));
}
TEST_CASE("terrain generation is deterministic for a seed and coordinates") {
    nuc::world::ChunkGenerator generator({.seed=42});
    const auto a = generator.generate(-1, 2);
    const auto b = generator.generate(-1, 2);
    REQUIRE(a.voxels() == b.voxels());
    REQUIRE(a.chunk_x() == -1);
    REQUIRE(a.chunk_z() == 2);
    REQUIRE(std::any_of(a.voxels().begin(), a.voxels().end(), [](auto v){ return v == 1; }));
    REQUIRE(std::any_of(a.voxels().begin(), a.voxels().end(), [](auto v){ return v == 3; }));
}
TEST_CASE("terrain generation uses configured seed") {
    nuc::world::ChunkGenerator a({.seed=1});
    nuc::world::ChunkGenerator b({.seed=2});
    REQUIRE(a.generate(0,0).voxels() != b.generate(0,0).voxels());
}
TEST_CASE("terrain generator rejects invalid settings") {
    REQUIRE_THROWS(nuc::world::ChunkGenerator({.frequency=0.0F}));
    REQUIRE_THROWS(nuc::world::ChunkGenerator({.amplitude=-1.0F}));
}
