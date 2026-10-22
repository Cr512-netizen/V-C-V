#pragma once
#include <cstddef>
#include <vector>
namespace nuc::sim {
class FieldGrid {public:FieldGrid(int x,int y,int z,double diffusion=0.15,double decay=0.01);void add_source(int,int,int,double);double at(int,int,int) const;void set(int,int,int,double);void tick();double total() const noexcept;private:std::size_t index(int,int,int) const;int x_,y_,z_;double diffusion_,decay_;std::vector<double> values_,sources_;};
}
