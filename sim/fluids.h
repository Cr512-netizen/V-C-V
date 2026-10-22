#pragma once
#include <cstdint>
#include <vector>
namespace nuc::sim {
struct FluidCell {std::uint32_t fluid{};double volume{};double capacity{1.0};double pressure() const noexcept{return capacity>0?volume/capacity:0.0;}};
class FluidGrid {public:FluidGrid(int x,int y,int z,double diffusion=0.2);const FluidCell& at(int,int,int) const;void set(int,int,int,FluidCell);void tick();int width() const noexcept{return x_;}int height() const noexcept{return y_;}int depth() const noexcept{return z_;}double total_volume() const noexcept;private:std::size_t index(int,int,int) const;int x_,y_,z_;double diffusion_;std::vector<FluidCell> cells_;};
}
