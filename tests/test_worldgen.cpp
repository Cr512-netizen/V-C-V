#include <catch2/catch_test_macros.hpp>
#include "world/biome.h"
#include "world/cave_carver.h"
#include "world/generator.h"
#include "world/gen_resources.h"
#include "world/terrain_shape.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace {
constexpr std::size_t extent = nuc::world::Chunk::extent;

int surface_of(const nuc::world::Chunk& chunk, std::size_t x, std::size_t z) {
    for (int y = static_cast<int>(extent) - 1; y >= 0; --y) {
        const auto material = chunk.at(x, static_cast<std::size_t>(y), z);
        if (material != 0 && material != 4) return y;
    }
    return -1;
}

std::uint64_t fnv(const nuc::world::Chunk& chunk) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto material : chunk.voxels()) {
        hash ^= material;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::size_t count_material(const nuc::world::Chunk& chunk, std::uint32_t material) {
    return static_cast<std::size_t>(std::count(chunk.voxels().begin(), chunk.voxels().end(), material));
}

std::size_t count_cave_voxels(const nuc::world::Chunk& chunk) {
    std::size_t caves = 0;
    for (std::size_t z = 0; z < extent; ++z)
        for (std::size_t x = 0; x < extent; ++x) {
            const int surface = surface_of(chunk, x, z);
            for (int y = 2; y <= surface - 4; ++y)
                if (chunk.at(x, static_cast<std::size_t>(y), z) == 0) ++caves;
        }
    return caves;
}

}

TEST_CASE("W1 terrain height is continuous across a chunk seam") {
    const nuc::world::TerrainShape shape({.seed = 42});
    for (int offset = 0; offset < 64; ++offset) {
        const auto before_x = shape.sample(-1, offset - 32).height;
        const auto after_x = shape.sample(0, offset - 32).height;
        REQUIRE(std::abs(before_x - after_x) <= 3.0F);
        const auto before_z = shape.sample(offset - 32, -1).height;
        const auto after_z = shape.sample(offset - 32, 0).height;
        REQUIRE(std::abs(before_z - after_z) <= 3.0F);
    }
}

TEST_CASE("W2 biomes are spatially coherent and both appear") {
    const nuc::world::ChunkGenerator generator({.seed = 7});
    std::size_t temperate = 0;
    std::size_t arid = 0;
    for (int z = 0; z < 64; ++z)
        for (int x = 0; x < 64; ++x)
            (generator.biome_at(x, z) == nuc::world::Biome::arid ? arid : temperate) += 1;
    REQUIRE(temperate > 0);
    REQUIRE(arid > 0);

    std::size_t equal_pairs = 0;
    std::size_t pairs = 0;
    for (int z = 0; z < 8; ++z)
        for (int x = 0; x + 1 < 8; ++x) {
            ++pairs;
            if (generator.biome_at(x, z) == generator.biome_at(x + 1, z)) ++equal_pairs;
        }
    REQUIRE(equal_pairs * 10 >= pairs * 9);
}

TEST_CASE("W3 a column keeps its biome and surface material inside any chunk") {
    const nuc::world::ChunkGenerator generator({.seed = 11});
    for (std::int32_t chunk_x = -2; chunk_x <= 2; ++chunk_x)
        for (std::int32_t chunk_z = -2; chunk_z <= 2; ++chunk_z) {
            const auto chunk = generator.generate(chunk_x, chunk_z);
            for (std::size_t z = 0; z < extent; z += 7)
                for (std::size_t x = 0; x < extent; x += 7) {
                    const auto surface = surface_of(chunk, x, z);
                    if (surface < 6) continue;
                    const auto world_x = static_cast<std::int64_t>(chunk_x) * 32 + static_cast<std::int64_t>(x);
                    const auto world_z = static_cast<std::int64_t>(chunk_z) * 32 + static_cast<std::int64_t>(z);
                    const auto biome = generator.biome_at(world_x, world_z);
                    const auto profile = nuc::world::biome_profile(biome);
                    const auto top = chunk.at(x, static_cast<std::size_t>(surface), z);
                    const bool submerged = surface <= generator.settings().sea_level;
                    REQUIRE(top == (submerged ? profile.subsurface : profile.surface));
                }
        }
}

TEST_CASE("W4 caves keep a solid crust and exist underground") {
    const nuc::world::ChunkGenerator generator({.seed = 3});
    std::size_t cave_voxels = 0;
    for (std::int32_t chunk_x = 0; chunk_x <= 2; ++chunk_x)
        for (std::int32_t chunk_z = 0; chunk_z <= 2; ++chunk_z) {
            const auto chunk = generator.generate(chunk_x, chunk_z);
            for (std::size_t z = 0; z < extent; ++z)
                for (std::size_t x = 0; x < extent; ++x) {
                    const auto surface = surface_of(chunk, x, z);
                    if (surface < 6) continue;
                    for (int depth = 0; depth < 4; ++depth)
                        REQUIRE(chunk.at(x, static_cast<std::size_t>(surface - depth), z) != 0);
                }
            for (int y = 0; y < 2; ++y)
                for (std::size_t z = 0; z < extent; ++z)
                    for (std::size_t x = 0; x < extent; ++x)
                        REQUIRE(chunk.at(x, static_cast<std::size_t>(y), z) != 0);
            cave_voxels += count_cave_voxels(chunk);
        }
    REQUIRE(cave_voxels > 0);
}

TEST_CASE("W5 cave carving is a pure function of world coordinates") {
    const nuc::world::CaveCarver carver({.seed = 5});
    for (std::int64_t y = 2; y < 24; y += 3) {
        const bool first = carver.is_cave(10, y, -7);
        const bool second = carver.is_cave(10, y, -7);
        REQUIRE(first == second);
    }
    const nuc::world::ChunkGenerator generator({.seed = 5});
    const auto a = generator.generate(1, 1);
    const auto b = generator.generate(1, 1);
    REQUIRE(a.voxels() == b.voxels());
}

TEST_CASE("W6 ore veins are deterministic, bounded and never replace air") {
    nuc::world::ResourceGenerator resources;
    resources.load(std::filesystem::path(NUC_SOURCE_DIR) / "data/biomes.json");
    const auto* rules = resources.rule_for(nuc::world::Biome::temperate);
    REQUIRE(rules != nullptr);
    REQUIRE_FALSE(rules->resources.empty());
    const auto& copper = rules->resources.front();

    nuc::world::Chunk stone_chunk(2, -1);
    stone_chunk.voxels().fill(1);
    resources.apply(stone_chunk, 1234, nuc::world::Biome::temperate);
    const auto ore_count = count_material(stone_chunk, copper.id);
    REQUIRE(ore_count >= 1);
    REQUIRE(ore_count <= copper.vein_size * 8);

    for (std::size_t z = 0; z < extent; ++z)
        for (std::size_t y = 0; y < extent; ++y)
            for (std::size_t x = 0; x < extent; ++x) {
                if (stone_chunk.at(x, y, z) != copper.id) continue;
                REQUIRE(static_cast<int>(y) >= copper.min_y);
                REQUIRE(static_cast<int>(y) <= copper.max_y);
            }

    nuc::world::Chunk repeat(2, -1);
    repeat.voxels().fill(1);
    resources.apply(repeat, 1234, nuc::world::Biome::temperate);
    REQUIRE(repeat.voxels() == stone_chunk.voxels());

    nuc::world::Chunk air_chunk(2, -1);
    resources.apply(air_chunk, 1234, nuc::world::Biome::temperate);
    REQUIRE(count_material(air_chunk, copper.id) == 0);
}

TEST_CASE("W7 generation is deterministic per seed and coordinates") {
    const nuc::world::ChunkGenerator generator({.seed = 99});
    const auto first = generator.generate(-3, 4);
    const auto second = generator.generate(-3, 4);
    REQUIRE(first.voxels() == second.voxels());
    REQUIRE(fnv(first) == fnv(second));

    const nuc::world::ChunkGenerator other({.seed = 100});
    REQUIRE(fnv(other.generate(-3, 4)) != fnv(first));
}

TEST_CASE("W8 water fills above ground and grass never grows underwater") {
    // An explicit high sea level keeps the rule under test independent of the
    // tuned world defaults, which leave only a few lakes near the spawn.
    const nuc::world::ChunkGenerator generator({.seed = 21, .sea_level = 13});
    std::size_t submerged_columns = 0;
    for (std::int32_t chunk_z = -1; chunk_z <= 1; ++chunk_z) {
        const auto chunk = generator.generate(0, chunk_z);
        for (std::size_t z = 0; z < extent; ++z)
            for (std::size_t x = 0; x < extent; ++x) {
                const auto surface = surface_of(chunk, x, z);
                if (surface < 0 || surface >= generator.settings().sea_level) continue;
                ++submerged_columns;
                REQUIRE(chunk.at(x, static_cast<std::size_t>(surface + 1), z) == 4);
                REQUIRE(chunk.at(x, static_cast<std::size_t>(surface), z) != 3);
            }
    }
    REQUIRE(submerged_columns > 0);
}

TEST_CASE("W9 nine chunks generate well inside the frame budget") {
    const nuc::world::ChunkGenerator generator({.seed = 1337});
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (std::int32_t z = -1; z <= 1; ++z)
        for (std::int32_t x = -1; x <= 1; ++x) checksum += fnv(generator.generate(x, z));
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start);
    REQUIRE(checksum != 0);
    REQUIRE(elapsed.count() < 250.0);
}
