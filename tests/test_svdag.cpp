#include <catch2/catch_test_macros.hpp>
#include "world/svdag.h"
#include <cstdint>
#include <vector>

TEST_CASE("SVDAG round-trips a mixed voxel grid") {
    constexpr std::size_t n = 8;
    std::vector<std::uint32_t> voxels(n*n*n, 0);
    for (std::size_t z=2; z<6; ++z)
        for (std::size_t y=1; y<4; ++y)
            for (std::size_t x=3; x<7; ++x)
                voxels[x+n*(y+n*z)] = 7;
    nuc::world::Svdag dag;
    dag.build(n, voxels);
    REQUIRE(dag.extent() == n);
    REQUIRE(dag.decode() == voxels);
    REQUIRE(dag.at(4,2,3) == 7);
    REQUIRE(dag.at(0,0,0) == 0);
}
TEST_CASE("SVDAG deduplicates equal subtrees") {
    std::vector<std::uint32_t> voxels(16*16*16, 3);
    nuc::world::Svdag dag;
    dag.build(16, voxels);
    REQUIRE(dag.node_count() == 1);
    REQUIRE(dag.at(15,15,15) == 3);
}
TEST_CASE("SVDAG handles all-empty and sparse data") {
    std::vector<std::uint32_t> empty(8*8*8, 0);
    nuc::world::Svdag dag;
    dag.build(8, empty);
    REQUIRE(dag.node_count() == 1);
    REQUIRE(dag.decode() == empty);
    empty[7 + 8*(7 + 8*7)] = 9;
    dag.build(8, empty);
    REQUIRE(dag.decode() == empty);
}
TEST_CASE("SVDAG validates dimensions and bounds") {
    nuc::world::Svdag dag;
    REQUIRE_THROWS(dag.build(3, std::vector<std::uint32_t>(27)));
    REQUIRE_THROWS(dag.build(4, std::vector<std::uint32_t>(8)));
    REQUIRE_THROWS(dag.at(0,0,0));
}
