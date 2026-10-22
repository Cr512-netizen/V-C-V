#include "world/raycast.h"
#include <cmath>
#include <limits>
#include <climits>

namespace nuc::world {
std::size_t VoxelCoordHash::operator()(const VoxelCoord& p) const noexcept {
    std::size_t h=static_cast<std::size_t>(static_cast<std::uint32_t>(p.x));
    h^=static_cast<std::size_t>(static_cast<std::uint32_t>(p.y))+0x9e3779b9U+(h<<6U)+(h>>2U);
    h^=static_cast<std::size_t>(static_cast<std::uint32_t>(p.z))+0x9e3779b9U+(h<<6U)+(h>>2U);
    return h;
}
std::optional<RayHit> raycast(const VoxelReader& read, glm::vec3 origin,
                              glm::vec3 direction, float max_distance) {
    if (!read || !std::isfinite(max_distance) || max_distance<0.0F ||
        !std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) ||
        !std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z))
        return std::nullopt;
    const float length=glm::length(direction);
    if (!std::isfinite(length) || length<=std::numeric_limits<float>::epsilon()) return std::nullopt;
    const double fx=std::floor(static_cast<double>(origin.x));
    const double fy=std::floor(static_cast<double>(origin.y));
    const double fz=std::floor(static_cast<double>(origin.z));
    if(fx<static_cast<double>(INT_MIN)+1.0 || fx>static_cast<double>(INT_MAX)-1.0 ||
       fy<static_cast<double>(INT_MIN)+1.0 || fy>static_cast<double>(INT_MAX)-1.0 ||
       fz<static_cast<double>(INT_MIN)+1.0 || fz>static_cast<double>(INT_MAX)-1.0)
        return std::nullopt;
    direction/=length;
    VoxelCoord cell{static_cast<int>(fx),static_cast<int>(fy),static_cast<int>(fz)};
    const auto first=read(cell.x,cell.y,cell.z);
    if (first) return RayHit{cell,cell,{0,0,0},first,0.0F,false};
    const int step[3]={direction.x>0?1:(direction.x<0?-1:0),
                       direction.y>0?1:(direction.y<0?-1:0),
                       direction.z>0?1:(direction.z<0?-1:0)};
    const float o[3]={origin.x,origin.y,origin.z};
    const float d[3]={direction.x,direction.y,direction.z};
    int* c[3]={&cell.x,&cell.y,&cell.z};
    float tmax[3],tdelta[3];
    for(int a=0;a<3;++a) {
        if(step[a]==0) { tmax[a]=tdelta[a]=std::numeric_limits<float>::infinity(); continue; }
        const float boundary=static_cast<float>(*c[a]+(step[a]>0?1:0));
        tmax[a]=(boundary-o[a])/d[a];
        tdelta[a]=std::abs(1.0F/d[a]);
    }
    float distance=0.0F;
    VoxelCoord previous=cell, normal{};
    while(true) {
        int axis=0;
        if(tmax[1]<tmax[axis]) axis=1;
        if(tmax[2]<tmax[axis]) axis=2;
        distance=tmax[axis];
        if(!std::isfinite(distance) || distance>max_distance) return std::nullopt;
        previous=cell;
        if((step[axis]>0 && *c[axis]==INT_MAX) ||
           (step[axis]<0 && *c[axis]==INT_MIN)) return std::nullopt;
        *c[axis]+=step[axis];
        normal={0,0,0};
        if(axis==0) normal.x=-step[axis];
        else if(axis==1) normal.y=-step[axis];
        else normal.z=-step[axis];
        tmax[axis]+=tdelta[axis];
        const auto material=read(cell.x,cell.y,cell.z);
        if(material) return RayHit{cell,previous,normal,material,distance,true};
    }
}
}
