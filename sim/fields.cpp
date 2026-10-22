#include "sim/fields.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace nuc::sim {
namespace {
std::size_t checked_cell_count(int x,int y,int z,double diffusion,double decay) {
 if(x<=0||y<=0||z<=0||!std::isfinite(diffusion)||diffusion<0||diffusion>0.25||
    !std::isfinite(decay)||decay<0||decay>1)
  throw std::invalid_argument("invalid field grid settings");
 const auto sx=static_cast<std::size_t>(x),sy=static_cast<std::size_t>(y),sz=static_cast<std::size_t>(z);
 const auto max=std::numeric_limits<std::size_t>::max();
 if(sx>max/sy||sx*sy>max/sz) throw std::length_error("field grid dimensions overflow");
 return sx*sy*sz;
}
}
FieldGrid::FieldGrid(int x,int y,int z,double d,double decay)
 :x_(x),y_(y),z_(z),diffusion_(d),decay_(decay),
  values_(checked_cell_count(x,y,z,d,decay)),sources_(values_.size()) {}
std::size_t FieldGrid::index(int x,int y,int z) const {
 if(x<0||y<0||z<0||x>=x_||y>=y_||z>=z_) throw std::out_of_range("field grid coordinate");
 return static_cast<std::size_t>(x)+static_cast<std::size_t>(x_)*(static_cast<std::size_t>(y)+static_cast<std::size_t>(y_)*static_cast<std::size_t>(z));
}
void FieldGrid::add_source(int x,int y,int z,double amount) {
 if(!std::isfinite(amount)||amount<0) throw std::invalid_argument("field source must be finite and nonnegative");
 auto& source=sources_[index(x,y,z)];
 if(amount>std::numeric_limits<double>::max()-source) throw std::overflow_error("field source overflow");
 source+=amount;
}
double FieldGrid::at(int x,int y,int z) const { return values_[index(x,y,z)]; }
void FieldGrid::set(int x,int y,int z,double v) {
 if(!std::isfinite(v)||v<0) throw std::invalid_argument("field value must be finite and nonnegative");
 values_[index(x,y,z)]=v;
}
double FieldGrid::total() const noexcept { double s=0;for(double v:values_)s+=v;return s; }
void FieldGrid::tick() {
 // Diffuse pairwise across each undirected edge. This conserves field mass before decay
 // and avoids artificial mass gain at grid boundaries.
 std::vector<double> next=values_;
 const double rate=diffusion_/6.0;
 const int dirs[3][3]={{1,0,0},{0,1,0},{0,0,1}};
 for(int z=0;z<z_;++z) for(int y=0;y<y_;++y) for(int x=0;x<x_;++x) {
  const auto i=index(x,y,z);
  for(const auto& d:dirs) {
   const int nx=x+d[0],ny=y+d[1],nz=z+d[2];
   if(nx>=x_||ny>=y_||nz>=z_) continue;
   const auto j=index(nx,ny,nz);
   const double transfer=(values_[i]-values_[j])*rate;
   next[i]-=transfer;
   next[j]+=transfer;
  }
 }
 for(std::size_t i=0;i<next.size();++i) {
  const double decayed=next[i]*(1.0-decay_);
  if(!std::isfinite(decayed+sources_[i])) throw std::overflow_error("field value overflow");
  next[i]=std::max(0.0,decayed+sources_[i]);
 }
 values_.swap(next);
}
}
