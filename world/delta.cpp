#include "world/delta.h"
#include <algorithm>

namespace nuc::world {
namespace {
int floor_chunk(int value) noexcept {
    constexpr int extent=static_cast<int>(Chunk::extent);
    int q=value/extent;
    if(value%extent<0) --q;
    return q;
}
}
std::vector<std::pair<VoxelCoord,std::uint32_t>> VoxelDelta::entries() const {
    std::vector<std::pair<VoxelCoord,std::uint32_t>> out(overrides_.begin(),overrides_.end());
    std::sort(out.begin(),out.end(),[](const auto& a,const auto& b) {
        if(a.first.x!=b.first.x) return a.first.x<b.first.x;
        if(a.first.y!=b.first.y) return a.first.y<b.first.y;
        return a.first.z<b.first.z;
    });
    return out;
}
std::optional<std::uint32_t> VoxelDelta::get(VoxelCoord p) const {
    const auto it=overrides_.find(p);
    if(it==overrides_.end()) return std::nullopt;
    return it->second;
}
void VoxelDelta::set(VoxelCoord p,std::uint32_t material,std::uint32_t base_material) {
    if(material==base_material) { erase(p); return; }
    overrides_[p]=material;
}
void VoxelDelta::erase(VoxelCoord p) { overrides_.erase(p); }
Chunk VoxelDelta::apply_to(const Chunk& base) const {
    Chunk result=base;
    apply_to(base,result);
    return result;
}
void VoxelDelta::apply_to(const Chunk& base,Chunk& result) const {
    result=base;
    const int base_x=base.chunk_x(),base_z=base.chunk_z();
    for(const auto& [p,material]:overrides_) {
        if(p.y<0||p.y>=static_cast<int>(Chunk::extent) ||
           floor_chunk(p.x)!=base_x||floor_chunk(p.z)!=base_z) continue;
        const int lx=p.x-base_x*static_cast<int>(Chunk::extent);
        const int lz=p.z-base_z*static_cast<int>(Chunk::extent);
        result.set(static_cast<std::size_t>(lx),static_cast<std::size_t>(p.y),
                   static_cast<std::size_t>(lz),material);
    }
}
}
