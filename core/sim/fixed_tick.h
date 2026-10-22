#pragma once
#include <cstdint>
#include <functional>
namespace nuc::sim {
class FixedTick {
public:
 // Simulation runs at 15 fixed ticks per second by default.
 explicit FixedTick(double step=1.0/15.0,std::uint32_t max_ticks=5);
 std::uint32_t advance(double elapsed,const std::function<void(double,std::uint64_t)>& tick);
 std::uint64_t tick_index()const noexcept{return tick_index_;}
 double step()const noexcept{return step_;}
 double alpha()const noexcept{return accumulator_/step_;}
 void reset()noexcept{accumulator_=0.0;tick_index_=0;}
private:double step_,accumulator_=0.0;std::uint32_t max_ticks_;std::uint64_t tick_index_=0;
};
}