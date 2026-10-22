#include "physics/destruction.h"
#include "core/sim/rng.h"
#include "world/chunk.h"
#include <cmath>
#include <stdexcept>

namespace nuc::physics {
namespace {
int chunk_of(int v) noexcept {
    constexpr int n=static_cast<int>(world::Chunk::extent);
    int q=v/n;if(v%n<0)--q;return q;
}
}
std::uint64_t fracture_seed(std::uint64_t world_seed,std::uint64_t tick,
                            int x,int y,int z) noexcept {
    // splitmix64 finaliser: an avalanche mix of the four inputs and nothing else.
    const auto mix=[](std::uint64_t v) {
        v^=v>>30U;v*=0xBF58476D1CE4E5B9ULL;
        v^=v>>27U;v*=0x94D049BB133111EBULL;
        v^=v>>31U;return v;
    };
    std::uint64_t h=world_seed+0x9E3779B97F4A7C15ULL;
    h=mix(h^tick);
    h=mix(h^(static_cast<std::uint64_t>(static_cast<std::uint32_t>(x))<<32U)
          ^static_cast<std::uint32_t>(z));
    return mix(h^static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)));
}
FractureResult fracture_block(glm::ivec3 origin,std::uint32_t material,
                              std::uint64_t seed,glm::vec3 impulse) {
    if(material==0||!std::isfinite(impulse.x)||!std::isfinite(impulse.y)||!std::isfinite(impulse.z))
        throw std::invalid_argument("invalid block fracture input");
    FractureResult result;result.boundary_bodies.reserve(296);
    result.collapsed_interior=216;result.interior_material=material;
    nuc::sim::Rng rng(seed);
    const glm::vec3 center=glm::vec3(origin)+glm::vec3(0.5F);
    for(int z=0;z<8;++z)for(int y=0;y<8;++y)for(int x=0;x<8;++x) {
        if(x!=0&&x!=7&&y!=0&&y!=7&&z!=0&&z!=7)continue;
        const glm::vec3 position=glm::vec3(origin)+glm::vec3((x+0.5F)/8.0F,
            (y+0.5F)/8.0F,(z+0.5F)/8.0F);
        auto body=make_body(position,glm::vec3(0.0625F),1.0F/512.0F,material);
        glm::vec3 direction=position-center;
        const float length=glm::length(direction);
        if(length>0.0F)direction/=length;
        const float variation=0.5F+static_cast<float>(rng.unit())*1.5F;
        body.velocity=impulse+direction*variation;
        body.owner_chunk_x=chunk_of(origin.x);body.owner_chunk_z=chunk_of(origin.z);
        result.boundary_bodies.push_back(body);
    }
    return result;
}
DebrisPool::DebrisPool(std::size_t cap):cap_(cap) {
    if(cap_==0)throw std::invalid_argument("debris pool cap must be positive");
}
PoolReport DebrisPool::reserve(std::span<const RigidBody> bodies) {
    PoolReport report;report.accepted_indices.reserve(bodies.size());
    for(std::size_t i=0;i<bodies.size();++i) {
        const auto& body=bodies[i];auto& count=counts_[{body.owner_chunk_x,body.owner_chunk_z}];
        if(count<cap_){++count;++report.accepted;report.accepted_indices.push_back(i);}
        else ++report.rejected;
    }
    return report;
}
void DebrisPool::release(int x,int z,std::size_t count) {
    const auto it=counts_.find({x,z});if(it==counts_.end())return;
    if(count>=it->second)counts_.erase(it);else it->second-=count;
}
std::size_t DebrisPool::count(int x,int z) const {
    const auto it=counts_.find({x,z});return it==counts_.end()?0:it->second;
}
}
