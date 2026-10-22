#include "sim/fluids.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace nuc::sim {
namespace {
std::size_t checked_cell_count(int x,int y,int z,double diffusion) {
 if(x<=0||y<=0||z<=0||!std::isfinite(diffusion)||diffusion<0.0||diffusion>0.5)
  throw std::invalid_argument("invalid fluid grid dimensions or diffusion");
 const auto sx=static_cast<std::size_t>(x),sy=static_cast<std::size_t>(y),sz=static_cast<std::size_t>(z);
 const auto max=std::numeric_limits<std::size_t>::max();
 if(sx>max/sy||sx*sy>max/sz) throw std::length_error("fluid grid dimensions overflow");
 return sx*sy*sz;
}
}
FluidGrid::FluidGrid(int x,int y,int z,double d)
 :x_(x),y_(y),z_(z),diffusion_(d),cells_(checked_cell_count(x,y,z,d)) {}
std::size_t FluidGrid::index(int x,int y,int z) const {
 if(x<0||y<0||z<0||x>=x_||y>=y_||z>=z_) throw std::out_of_range("fluid grid coordinate");
 return static_cast<std::size_t>(x)+static_cast<std::size_t>(x_)*(static_cast<std::size_t>(y)+static_cast<std::size_t>(y_)*static_cast<std::size_t>(z));
}
const FluidCell& FluidGrid::at(int x,int y,int z) const { return cells_[index(x,y,z)]; }
void FluidGrid::set(int x,int y,int z,FluidCell c) {
 if(!std::isfinite(c.volume)||!std::isfinite(c.capacity)||c.volume<0||c.capacity<=0||c.volume>c.capacity)
  throw std::invalid_argument("fluid volume must fit positive capacity");
 if(c.volume==0)c.fluid=0;else if(c.fluid==0)throw std::invalid_argument("non-empty fluid cell requires fluid ID");
 cells_[index(x,y,z)]=c;
}
double FluidGrid::total_volume() const noexcept { double n=0;for(const auto& c:cells_)n+=c.volume;return n; }
void FluidGrid::tick() {
 const int dirs[3][3]={{1,0,0},{0,1,0},{0,0,1}};
 for(int z=0;z<z_;++z)for(int y=0;y<y_;++y)for(int x=0;x<x_;++x) {
  auto& a=cells_[index(x,y,z)];
  for(const auto& d:dirs) {
   int nx=x+d[0],ny=y+d[1],nz=z+d[2];if(nx>=x_||ny>=y_||nz>=z_)continue;
   auto& b=cells_[index(nx,ny,nz)];
   if(a.fluid!=0&&b.fluid!=0&&a.fluid!=b.fluid)continue;
   if(a.volume<=b.volume)continue;
   double amount=std::min({(a.volume-b.volume)*diffusion_,a.volume,b.capacity-b.volume});
   if(amount<=0)continue;
   if(b.fluid==0)b.fluid=a.fluid;
   a.volume-=amount;b.volume+=amount;
   if(a.volume<1e-12){a.volume=0;a.fluid=0;}
  }
 }
}
}
