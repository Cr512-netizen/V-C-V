#include "physics/narrowphase.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace nuc::physics {
std::optional<Contact> box_box_sat(const RigidBody& a,const RigidBody& b,
                                   std::size_t ia,std::size_t ib) {
    const glm::vec3 delta=b.position-a.position;
    const glm::vec3 overlap=a.half_extents+b.half_extents-glm::abs(delta);
    if(overlap.x<0.0F||overlap.y<0.0F||overlap.z<0.0F) return std::nullopt;
    int axis=0;
    if(overlap.y<overlap.x) axis=1;
    if(overlap.z<overlap[axis]) axis=2;
    glm::vec3 normal(0.0F);
    normal[axis]=delta[axis]<0.0F?-1.0F:1.0F;
    const glm::vec3 lo=glm::max(a.position-a.half_extents,b.position-b.half_extents);
    const glm::vec3 hi=glm::min(a.position+a.half_extents,b.position+b.half_extents);
    return Contact{ia,ib,normal,(lo+hi)*0.5F,overlap[axis]};
}
bool point_vs_svdag(glm::ivec3 p,const world::Svdag& dag) {
    const auto extent=static_cast<int>(dag.extent());
    if(p.x<0||p.y<0||p.z<0||p.x>=extent||p.y>=extent||p.z>=extent||dag.empty()) return false;
    return dag.at(static_cast<std::size_t>(p.x),static_cast<std::size_t>(p.y),
                  static_cast<std::size_t>(p.z))!=0;
}
void resolve_body_vs_voxels(RigidBody& body,const SolidVoxelQuery& solid,double dt) {
    if(!solid||body.inverse_mass<=0.0F||!std::isfinite(dt)||dt<=0.0) return;
    const glm::vec3 lo=body.position-body.half_extents,hi=body.position+body.half_extents;
    const auto valid=[](float v){return std::isfinite(v)&&v>static_cast<float>(std::numeric_limits<int>::min()+2)&&v<static_cast<float>(std::numeric_limits<int>::max()-2);};
    if(!valid(lo.x)||!valid(lo.y)||!valid(lo.z)||!valid(hi.x)||!valid(hi.y)||!valid(hi.z)) return;
    const int x0=static_cast<int>(std::floor(lo.x)),x1=static_cast<int>(std::floor(hi.x));
    const int y0=static_cast<int>(std::floor(lo.y)),y1=static_cast<int>(std::floor(hi.y));
    const int z0=static_cast<int>(std::floor(lo.z)),z1=static_cast<int>(std::floor(hi.z));
    if(!body.awake) {
        // A sleeping body still depends on the world: wake it when its support is gone,
        // so debris cannot hang in the air after the block beneath it is dug out.
        const int below=y0-1;
        bool supported=false;
        for(int z=z0;z<=z1&&!supported;++z) for(int x=x0;x<=x1&&!supported;++x)
            if(solid(x,below,z))supported=true;
        if(!supported){body.awake=true;body.sleep_time=0.0F;}
        return;
    }
    const float restitution=std::clamp(body.restitution,0.0F,1.0F);
    const float friction=std::max(0.0F,body.friction);
    for(int z=z0;z<=z1;++z) for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x) {
        if(!solid(x,y,z)) continue;
        RigidBody voxel;voxel.position={x+0.5F,y+0.5F,z+0.5F};voxel.half_extents=glm::vec3(0.5F);
        const auto hit=box_box_sat(body,voxel,0,1);
        if(!hit||hit->penetration<=0.0F) continue;
        body.position-=hit->normal*(hit->penetration+0.001F);
        const float vn=glm::dot(body.velocity,hit->normal);
        if(vn>0.0F) {
            const float bounce=std::abs(vn)<1.0F?0.0F:restitution;
            body.velocity-=hit->normal*((1.0F+bounce)*vn);
            const glm::vec3 tangent=body.velocity-hit->normal*glm::dot(body.velocity,hit->normal);
            body.velocity-=tangent*std::clamp(friction*static_cast<float>(dt)*8.0F,0.0F,1.0F);
        }
    }
}
}
