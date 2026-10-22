#include "world/chunk.h"
#include <stdexcept>
namespace nuc::world {
Chunk::Chunk(std::int32_t x, std::int32_t z) noexcept : chunk_x_(x), chunk_z_(z) { voxels_.fill(0); }
std::uint32_t Chunk::at(std::size_t x, std::size_t y, std::size_t z) const {
    if (x >= extent || y >= extent || z >= extent) throw std::out_of_range("Chunk voxel coordinate out of bounds");
    return voxels_[index(x,y,z)];
}
void Chunk::set(std::size_t x, std::size_t y, std::size_t z, std::uint32_t material) {
    if (x >= extent || y >= extent || z >= extent) throw std::out_of_range("Chunk voxel coordinate out of bounds");
    voxels_[index(x,y,z)] = material;
}
}
