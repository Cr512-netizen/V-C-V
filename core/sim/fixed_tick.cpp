#include "core/sim/fixed_tick.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace nuc::sim {
FixedTick::FixedTick(double step,std::uint32_t max_ticks):step_(step),max_ticks_(max_ticks){if(!std::isfinite(step_)||step_<=0.0||max_ticks_==0)throw std::invalid_argument("invalid fixed tick settings");}
std::uint32_t FixedTick::advance(double elapsed,const std::function<void(double,std::uint64_t)>& tick){if(!std::isfinite(elapsed)||elapsed<0.0)throw std::invalid_argument("invalid elapsed time");accumulator_+=std::min(elapsed,step_*static_cast<double>(max_ticks_));std::uint32_t count=0;while(accumulator_+step_*1e-12>=step_&&count<max_ticks_){tick(step_,tick_index_);++tick_index_;++count;accumulator_-=step_;}if(count==max_ticks_&&accumulator_>=step_)accumulator_=std::fmod(accumulator_,step_);return count;}
}