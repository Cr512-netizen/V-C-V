#include "physics/broadphase.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace nuc::physics {
namespace {
using Cell=std::tuple<int,int,int>;
int cell(float value,float size) {
    const double v=std::floor(static_cast<double>(value)/size);
    if(!std::isfinite(v)||v<std::numeric_limits<int>::min()||v>std::numeric_limits<int>::max())
        throw std::out_of_range("rigid body exceeds broadphase grid range");
    return static_cast<int>(v);
}
}
std::vector<BodyPair> build_pairs(std::span<const RigidBody> bodies,float cell_size) {
    if(!std::isfinite(cell_size)||cell_size<=0.0F)
        throw std::invalid_argument("broadphase cell size must be positive");
    std::map<Cell,std::vector<std::size_t>> grid;
    for(std::size_t i=0;i<bodies.size();++i) {
        const auto& b=bodies[i];
        const auto lo=b.position-b.half_extents,hi=b.position+b.half_extents;
        const int x0=cell(lo.x,cell_size),x1=cell(hi.x,cell_size);
        const int y0=cell(lo.y,cell_size),y1=cell(hi.y,cell_size);
        const int z0=cell(lo.z,cell_size),z1=cell(hi.z,cell_size);
        for(int z=z0;z<=z1;++z) for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x)
            grid[{x,y,z}].push_back(i);
    }
    std::set<BodyPair> unique;
    for(const auto& [cell_key,indices]:grid) {
        (void)cell_key;
        for(std::size_t i=0;i<indices.size();++i) for(std::size_t j=i+1;j<indices.size();++j) {
            auto a=indices[i],b=indices[j];
            if(a==b||(bodies[a].inverse_mass<=0.0F&&bodies[b].inverse_mass<=0.0F)) continue;
            if(a>b) std::swap(a,b);
            unique.emplace(a,b);
        }
    }
    return {unique.begin(),unique.end()};
}
}
