#pragma once
#include <array>
#include <cstdint>
namespace nuc::sim {
class Rng {
public:
 explicit Rng(std::uint64_t seed=1)noexcept;
 std::uint64_t next()noexcept;
 double unit()noexcept;
 const std::array<std::uint64_t,4>& state()const noexcept{return s_;}
 void set_state(const std::array<std::uint64_t,4>& state);
 static Rng for_tick(std::uint64_t world_seed,std::uint64_t tick,std::uint64_t stream=0)noexcept;
private:std::array<std::uint64_t,4>s_{};
};
}