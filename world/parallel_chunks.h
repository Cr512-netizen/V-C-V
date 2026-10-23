#pragma once
#include "core/jobs.h"
#include "world/chunk.h"
#include "world/gen_resources.h"
#include "world/generator.h"
#include <cstdint>
#include <vector>

namespace nuc::world {
struct ChunkRequest {
    std::int32_t chunk_x{};
    std::int32_t chunk_z{};
};

// Fills every requested chunk across the pool. Results keep the request order,
// so callers never depend on how many threads ran or in which order.
[[nodiscard]] std::vector<Chunk> generate_chunks(Jobs& jobs, const ChunkGenerator& generator,
                                                 const ResourceGenerator& resources, std::uint64_t seed,
                                                 const std::vector<ChunkRequest>& requests);
}
