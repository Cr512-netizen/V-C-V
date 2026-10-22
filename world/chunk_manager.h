#pragma once
#include "render/chunk_mesh.h"
#include "world/delta.h"
#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace nuc::world {
// Coordinates edits and CPU mesh rebuilds; GL uploads are drained on the render thread.
class ChunkManager final {
public:
    using ChunkMap = std::map<std::pair<int,int>,Chunk>;
    struct MeshUpdate { std::pair<int,int> coord; render::ChunkMesh mesh; std::uint64_t revision{}; };
    explicit ChunkManager(ChunkMap& chunks) : chunks_(chunks) {}
    [[nodiscard]] std::uint32_t get_voxel(VoxelCoord position) const;
    bool set_voxel(VoxelCoord position, std::uint32_t material);
    void mark_dirty(VoxelCoord position);
    void mark_chunk_dirty(int chunk_x,int chunk_z);
    void rebuild_dirty_at_tick_end();
    [[nodiscard]] std::vector<MeshUpdate> drain_upload_queue_on_render_thread();
    [[nodiscard]] std::size_t dirty_count() const noexcept { return dirty_.size(); }
    [[nodiscard]] std::size_t pending_count() const noexcept { return pending_.size(); }
    [[nodiscard]] const VoxelDelta& delta() const noexcept { return delta_; }
    // Copies loaded base chunks and overlays edits without mutating base storage.
    [[nodiscard]] ChunkMap materialized_snapshot() const;
private:
    struct Pending { std::pair<int,int> coord; std::uint64_t revision; std::future<MeshUpdate> result; };
    [[nodiscard]] const Chunk* find_chunk(int x,int z) const;
    [[nodiscard]] std::unique_ptr<Chunk> overlay(const Chunk& base) const;
    ChunkMap& chunks_;
    VoxelDelta delta_;
    std::set<std::pair<int,int>> dirty_;
    std::map<std::pair<int,int>,std::uint64_t> revisions_;
    std::vector<Pending> pending_;
    std::vector<MeshUpdate> upload_queue_;
};
}
