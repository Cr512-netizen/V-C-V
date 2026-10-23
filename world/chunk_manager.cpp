#include "world/chunk_manager.h"
#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <utility>

namespace nuc::world {
namespace {
int chunk_of(int value) noexcept {
    constexpr int n=static_cast<int>(Chunk::extent);
    int q=value/n;
    if(value%n<0) --q;
    return q;
}
int local_of(int value,int chunk) noexcept { return value-chunk*static_cast<int>(Chunk::extent); }
}
ChunkManager::ChunkMap ChunkManager::materialized_snapshot() const {
    ChunkMap result;
    for(const auto& [coord,base]:chunks_) {
        auto snapshot=overlay(base);
        result.emplace(coord,std::move(*snapshot));
    }
    return result;
}
const Chunk* ChunkManager::find_chunk(int x,int z) const {
    const auto it=chunks_.find({x,z});
    return it==chunks_.end()?nullptr:&it->second;
}
std::unique_ptr<Chunk> ChunkManager::overlay(const Chunk& base) const {
    auto result=std::make_unique<Chunk>(base);
    delta_.apply_to(base,*result);
    return result;
}
std::uint32_t ChunkManager::get_voxel(VoxelCoord p) const {
    if(p.y<0||p.y>=static_cast<int>(Chunk::extent)) return 0;
    if(const auto edited=delta_.get(p)) return *edited;
    const auto* chunk=find_chunk(chunk_of(p.x),chunk_of(p.z));
    if(!chunk) return 0;
    return chunk->at(static_cast<std::size_t>(local_of(p.x,chunk->chunk_x())),
                     static_cast<std::size_t>(p.y),
                     static_cast<std::size_t>(local_of(p.z,chunk->chunk_z())));
}
bool ChunkManager::set_voxel(VoxelCoord p,std::uint32_t material) {
    if(p.y<0||p.y>=static_cast<int>(Chunk::extent)) return false;
    const int cx=chunk_of(p.x),cz=chunk_of(p.z);
    const auto it=chunks_.find({cx,cz});
    if(it==chunks_.end()) return false;
    const auto current=get_voxel(p);
    if(current==material) return false;
    const auto base=it->second.at(static_cast<std::size_t>(local_of(p.x,cx)),
                                  static_cast<std::size_t>(p.y),
                                  static_cast<std::size_t>(local_of(p.z,cz)));
    delta_.set(p,material,base);
    mark_dirty(p);
    return true;
}
void ChunkManager::mark_chunk_dirty(int cx,int cz) {
    if(!find_chunk(cx,cz)) return;
    const auto key=std::pair{cx,cz}; dirty_.insert(key); ++revisions_[key];
}
void ChunkManager::mark_dirty(VoxelCoord p) {
    if(p.y<0||p.y>=static_cast<int>(Chunk::extent)) return;
    const int cx=chunk_of(p.x),cz=chunk_of(p.z);
    if(!find_chunk(cx,cz)) return;
    const int lx=local_of(p.x,cx),lz=local_of(p.z,cz);
    const auto add=[&](int x,int z) {
        if(!find_chunk(x,z)) return;
        const auto key=std::pair{x,z};
        dirty_.insert(key);
        ++revisions_[key];
    };
    add(cx,cz);
    if(lx==0) add(cx-1,cz);
    if(lx==static_cast<int>(Chunk::extent)-1) add(cx+1,cz);
    if(lz==0) add(cx,cz-1);
    if(lz==static_cast<int>(Chunk::extent)-1) add(cx,cz+1);
}
void ChunkManager::rebuild_dirty_at_tick_end() {
    const auto dirty=std::move(dirty_);
    dirty_.clear();
    for(const auto& coord:dirty) {
        const bool in_flight=std::any_of(pending_.begin(),pending_.end(),
            [&](const Pending& task){ return task.coord==coord; });
        if(in_flight) { dirty_.insert(coord); continue; }
        const auto it=chunks_.find(coord);
        if(it==chunks_.end()) continue;
        const auto revision=revisions_[coord];
        auto center=overlay(it->second);
        std::unique_ptr<Chunk> west,east,north,south;
        if(const auto* c=find_chunk(coord.first-1,coord.second)) west=overlay(*c);
        if(const auto* c=find_chunk(coord.first+1,coord.second)) east=overlay(*c);
        if(const auto* c=find_chunk(coord.first,coord.second-1)) north=overlay(*c);
        if(const auto* c=find_chunk(coord.first,coord.second+1)) south=overlay(*c);
        auto build = [coord, revision, center=std::move(center), west=std::move(west), east=std::move(east),
                      north=std::move(north), south=std::move(south)]() mutable {
            render::ChunkNeighbors neighbors;
            if(west) neighbors.west=west.get();
            if(east) neighbors.east=east.get();
            if(north) neighbors.north=north.get();
            if(south) neighbors.south=south.get();
            return MeshUpdate{coord,render::build_chunk_mesh(*center,neighbors),revision};
        };
        try {
            if(jobs_!=nullptr) {
                // Pool path: one queued task per chunk instead of one OS thread per chunk.
                auto promise=std::make_shared<std::promise<MeshUpdate>>();
                auto future=promise->get_future();
                jobs_->submit([build=std::move(build),promise]() mutable {
                    try {
                        promise->set_value(build());
                    } catch(...) {
                        promise->set_exception(std::current_exception());
                    }
                });
                pending_.push_back({coord,revision,std::move(future)});
            } else {
                pending_.push_back({coord,revision,std::async(std::launch::async,std::move(build))});
            }
        } catch(...) {
            dirty_.insert(coord);
        }
    }
}
std::vector<ChunkManager::MeshUpdate> ChunkManager::drain_upload_queue_on_render_thread() {
    for(auto it=pending_.begin();it!=pending_.end();) {
        if(it->result.wait_for(std::chrono::seconds(0))!=std::future_status::ready) { ++it; continue; }
        try {
            auto result=it->result.get();
            if(revisions_[result.coord]==result.revision) upload_queue_.push_back(std::move(result));
            else dirty_.insert(result.coord);
        } catch(...) {
            dirty_.insert(it->coord);
        }
        it=pending_.erase(it);
    }
    auto ready=std::move(upload_queue_);
    upload_queue_.clear();
    return ready;
}
}
