#include "world/parallel_chunks.h"
#include <utility>

namespace nuc::world {
std::vector<Chunk> generate_chunks(Jobs& jobs, const ChunkGenerator& generator,
                                   const ResourceGenerator& resources, std::uint64_t seed,
                                   const std::vector<ChunkRequest>& requests) {
    std::vector<Chunk> results(requests.size());
    jobs.parallel_for(requests.size(), [&](std::size_t index) {
        const auto& request = requests[index];
        auto chunk = generator.generate(request.chunk_x, request.chunk_z);
        // The biome comes from the chunk centre, the same rule the streaming path
        // uses, so ore placement does not depend on which thread built the chunk.
        const auto center_x = static_cast<std::int64_t>(request.chunk_x) * 32 + 16;
        const auto center_z = static_cast<std::int64_t>(request.chunk_z) * 32 + 16;
        resources.apply(chunk, seed, generator.biome_at(center_x, center_z));
        results[index] = std::move(chunk);
    });
    return results;
}
}
