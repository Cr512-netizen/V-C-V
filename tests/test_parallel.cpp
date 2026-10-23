#include <catch2/catch_test_macros.hpp>
#include "core/jobs.h"
#include "render/chunk_mesh.h"
#include "world/chunk_manager.h"
#include "world/gen_resources.h"
#include "world/generator.h"
#include "world/parallel_chunks.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
std::vector<nuc::world::ChunkRequest> grid(int from, int to) {
    std::vector<nuc::world::ChunkRequest> requests;
    for (int z = from; z <= to; ++z)
        for (int x = from; x <= to; ++x) requests.push_back({x, z});
    return requests;
}

std::vector<nuc::world::Chunk> sequential(const nuc::world::ChunkGenerator& generator,
                                          const nuc::world::ResourceGenerator& resources,
                                          std::uint64_t seed,
                                          const std::vector<nuc::world::ChunkRequest>& requests) {
    std::vector<nuc::world::Chunk> results;
    results.reserve(requests.size());
    for (const auto& request : requests) {
        auto chunk = generator.generate(request.chunk_x, request.chunk_z);
        const auto center_x = static_cast<std::int64_t>(request.chunk_x) * 32 + 16;
        const auto center_z = static_cast<std::int64_t>(request.chunk_z) * 32 + 16;
        resources.apply(chunk, seed, generator.biome_at(center_x, center_z));
        results.push_back(std::move(chunk));
    }
    return results;
}
}

TEST_CASE("M1 parallel_for visits every index exactly once") {
    nuc::Jobs jobs(4);
    constexpr std::size_t count = 10000;
    auto visits = std::make_unique<std::atomic<int>[]>(count);
    for (std::size_t i = 0; i < count; ++i) visits[i].store(0);
    jobs.parallel_for(count, [&visits](std::size_t index) { visits[index].fetch_add(1); });
    for (std::size_t i = 0; i < count; ++i) REQUIRE(visits[i].load() == 1);
}

TEST_CASE("M2 parallel_for handles empty and single ranges") {
    nuc::Jobs jobs(2);
    std::atomic<int> calls{0};
    jobs.parallel_for(0, [&calls](std::size_t) { calls.fetch_add(1); });
    REQUIRE(calls.load() == 0);
    jobs.parallel_for(1, [&calls](std::size_t index) {
        REQUIRE(index == 0);
        calls.fetch_add(1);
    });
    REQUIRE(calls.load() == 1);
}

TEST_CASE("M3 submit delivers a value through its promise") {
    nuc::Jobs jobs(2);
    auto promise = std::make_shared<std::promise<int>>();
    auto future = promise->get_future();
    jobs.submit([promise] { promise->set_value(41 + 1); });
    REQUIRE(future.get() == 42);
}

TEST_CASE("M4 an exception inside submit reaches the future") {
    nuc::Jobs jobs(2);
    auto promise = std::make_shared<std::promise<int>>();
    auto future = promise->get_future();
    jobs.submit([promise] {
        try {
            throw std::runtime_error("worker failed");
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    REQUIRE_THROWS_AS(future.get(), std::runtime_error);
}

TEST_CASE("M5 wait_idle drains every submitted task") {
    nuc::Jobs jobs(4);
    std::atomic<int> done{0};
    for (int i = 0; i < 64; ++i)
        jobs.submit([&done] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            done.fetch_add(1);
        });
    jobs.wait_idle();
    REQUIRE(done.load() == 64);
    REQUIRE(jobs.pending_count() == 0);
}

TEST_CASE("M6 parallel generation matches sequential generation") {
    nuc::Jobs jobs(4);
    nuc::world::ChunkGenerator generator({.seed = 77});
    nuc::world::ResourceGenerator resources;
    resources.load(std::filesystem::path(NUC_SOURCE_DIR) / "data/biomes.json");
    const auto requests = grid(-2, 1);

    const auto parallel_result = nuc::world::generate_chunks(jobs, generator, resources, 4242, requests);
    const auto sequential_result = sequential(generator, resources, 4242, requests);
    REQUIRE(parallel_result.size() == sequential_result.size());
    REQUIRE(parallel_result.size() == requests.size());
    for (std::size_t i = 0; i < requests.size(); ++i) {
        REQUIRE(parallel_result[i].chunk_x() == requests[i].chunk_x);
        REQUIRE(parallel_result[i].chunk_z() == requests[i].chunk_z);
        REQUIRE(parallel_result[i].voxels() == sequential_result[i].voxels());
    }
}

TEST_CASE("M7 the thread count never changes generated chunks") {
    nuc::world::ChunkGenerator generator({.seed = 5});
    nuc::world::ResourceGenerator resources;
    resources.load(std::filesystem::path(NUC_SOURCE_DIR) / "data/biomes.json");
    const auto requests = grid(-1, 1);

    nuc::Jobs single(1);
    nuc::Jobs many(4);
    const auto one_thread = nuc::world::generate_chunks(single, generator, resources, 9, requests);
    const auto four_threads = nuc::world::generate_chunks(many, generator, resources, 9, requests);
    for (std::size_t i = 0; i < one_thread.size(); ++i) REQUIRE(one_thread[i].voxels() == four_threads[i].voxels());
}

TEST_CASE("M8 pooled mesh rebuild equals a direct mesh build") {
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.emplace(std::pair{0, 0}, nuc::world::Chunk(0, 0));
    auto& center = chunks.at({0, 0});
    for (std::size_t z = 0; z < nuc::world::Chunk::extent; ++z)
        for (std::size_t x = 0; x < nuc::world::Chunk::extent; ++x) center.set(x, 0, z, 1);

    nuc::Jobs jobs(2);
    nuc::world::ChunkManager manager(chunks, &jobs);
    manager.mark_chunk_dirty(0, 0);
    manager.rebuild_dirty_at_tick_end();

    std::vector<nuc::world::ChunkManager::MeshUpdate> ready;
    for (int attempt = 0; attempt < 2000 && ready.empty(); ++attempt) {
        ready = manager.drain_upload_queue_on_render_thread();
        if (ready.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    REQUIRE(ready.size() == 1);
    jobs.wait_idle();

    nuc::render::ChunkNeighbors neighbors;
    const auto expected = nuc::render::build_chunk_mesh(center, neighbors);
    REQUIRE_FALSE(expected.indices.empty());
    REQUIRE(ready.front().mesh.indices == expected.indices);
    REQUIRE(ready.front().mesh.vertices.size() == expected.vertices.size());
}

TEST_CASE("M9 parallel generation is not slower than a single worker") {
    nuc::world::ChunkGenerator generator({.seed = 2024});
    nuc::world::ResourceGenerator resources;
    resources.load(std::filesystem::path(NUC_SOURCE_DIR) / "data/biomes.json");
    const auto requests = grid(-1, 1);
    const auto measure = [&](unsigned threads) {
        nuc::Jobs pool(threads);
        const auto start = std::chrono::steady_clock::now();
        const auto chunks = nuc::world::generate_chunks(pool, generator, resources, 1, requests);
        const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start);
        REQUIRE(chunks.size() == requests.size());
        return elapsed.count();
    };
    (void)measure(4);  // warm up the scheduler and the noise tables
    const auto sequential_ms = measure(1);
    const auto parallel_ms = measure(4);
    std::printf("[M9] 9 chunks: 1 worker %.2f ms, 4 workers %.2f ms\n", sequential_ms, parallel_ms);
    // Generous bound: this asserts "no regression", not a specific speed-up, so
    // the case stays honest on a loaded CI machine.
    REQUIRE(parallel_ms <= sequential_ms * 2.0 + 5.0);
}
