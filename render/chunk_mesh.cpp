#include "render/chunk_mesh.h"
#include <array>
#include <cstddef>
#include <iterator>

namespace nuc::render {
namespace {
using Point = std::array<float, 3>;
struct Face {
    Point normal;
    std::array<Point, 4> corners;
};
constexpr Face faces[] = {
    {{-1,0,0}, {Point{0,0,0}, Point{0,0,1}, Point{0,1,1}, Point{0,1,0}}},
    {{ 1,0,0}, {Point{1,0,0}, Point{1,1,0}, Point{1,1,1}, Point{1,0,1}}},
    {{0,-1,0}, {Point{0,0,0}, Point{1,0,0}, Point{1,0,1}, Point{0,0,1}}},
    {{0, 1,0}, {Point{0,1,0}, Point{0,1,1}, Point{1,1,1}, Point{1,1,0}}},
    {{0,0,-1}, {Point{0,0,0}, Point{0,1,0}, Point{1,1,0}, Point{1,0,0}}},
    {{0,0, 1}, {Point{0,0,1}, Point{1,0,1}, Point{1,1,1}, Point{0,1,1}}}
};
bool visible(const world::Chunk& chunk, const ChunkNeighbors& n,
             std::size_t x, std::size_t y, std::size_t z, std::size_t face) {
    switch (face) {
    case 0: return x ? chunk.at(x-1,y,z)==0 : (!n.west || n.west->at(world::Chunk::extent-1,y,z)==0);
    case 1: return x+1<world::Chunk::extent ? chunk.at(x+1,y,z)==0 : (!n.east || n.east->at(0,y,z)==0);
    case 2: return y ? chunk.at(x,y-1,z)==0 : true;
    case 3: return y+1<world::Chunk::extent ? chunk.at(x,y+1,z)==0 : true;
    case 4: return z ? chunk.at(x,y,z-1)==0 : (!n.north || n.north->at(x,y,world::Chunk::extent-1)==0);
    default:return z+1<world::Chunk::extent ? chunk.at(x,y,z+1)==0 : (!n.south || n.south->at(x,y,0)==0);
    }
}
}
ChunkMesh build_chunk_mesh(const world::Chunk& chunk, const ChunkNeighbors& neighbors) {
    ChunkMesh mesh;
    mesh.vertices.reserve(8192);
    mesh.indices.reserve(12288);
    for (std::size_t z=0; z<world::Chunk::extent; ++z)
    for (std::size_t y=0; y<world::Chunk::extent; ++y)
    for (std::size_t x=0; x<world::Chunk::extent; ++x) {
        const auto material=chunk.at(x,y,z);
        if (!material) continue;
        for (std::size_t f=0; f<std::size(faces); ++f) {
            if (!visible(chunk,neighbors,x,y,z,f)) continue;
            const auto base=static_cast<std::uint32_t>(mesh.vertices.size());
            const auto& face=faces[f];
            for (const auto& p:face.corners) {
                mesh.vertices.push_back({static_cast<float>(x)+p[0],static_cast<float>(y)+p[1],
                    static_cast<float>(z)+p[2],face.normal[0],face.normal[1],face.normal[2],material});
            }
            mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base,base+2,base+3});
        }
    }
    return mesh;
}
}
