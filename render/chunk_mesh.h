#pragma once
#include "world/chunk.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nuc::render {
struct MeshVertex {
    float x{}, y{}, z{};
    float nx{}, ny{}, nz{};
    float u{}, v{};
    std::uint32_t material{};
};
struct ChunkNeighbors {
    const world::Chunk* west{};
    const world::Chunk* east{};
    const world::Chunk* north{};
    const world::Chunk* south{};
};
struct ChunkMesh {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
    [[nodiscard]] std::size_t quad_count() const noexcept { return indices.size() / 6; }
};
// Builds a non-greedy, face-culled mesh. Vertices are chunk-local; missing
// neighbors are treated as air, so a later neighbor load should remesh edges.
[[nodiscard]] ChunkMesh build_chunk_mesh(const world::Chunk& chunk,
                                         const ChunkNeighbors& neighbors = {});
}
