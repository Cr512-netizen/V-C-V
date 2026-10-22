#include "core/sim/rng.h"
#include <stdexcept>
namespace nuc::sim {
namespace {std::uint64_t splitmix(std::uint64_t&x)noexcept{x+=0x9e3779b97f4a7c15ULL;auto z=x;z=(z^(z>>30U))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27U))*0x94d049bb133111ebULL;return z^(z>>31U);}constexpr std::uint64_t rotl(std::uint64_t x,int k)noexcept{return(x<<k)|(x>>(64-k));}}
Rng::Rng(std::uint64_t seed)noexcept{for(auto&v:s_)v=splitmix(seed);}
std::uint64_t Rng::next()noexcept{const auto result=rotl(s_[1]*5,7)*9;const auto t=s_[1]<<17U;s_[2]^=s_[0];s_[3]^=s_[1];s_[1]^=s_[2];s_[0]^=s_[3];s_[2]^=t;s_[3]=rotl(s_[3],45);return result;}
double Rng::unit()noexcept{return static_cast<double>(next()>>11U)*(1.0/9007199254740992.0);}
void Rng::set_state(const std::array<std::uint64_t,4>&state){if((state[0]|state[1]|state[2]|state[3])==0)throw std::invalid_argument("xoshiro state cannot be all zero");s_=state;}
Rng Rng::for_tick(std::uint64_t seed,std::uint64_t tick,std::uint64_t stream)noexcept{Rng r(seed^(tick*0x9e3779b97f4a7c15ULL)^(stream*0xd2b74407b1ce6e93ULL));return r;}
}