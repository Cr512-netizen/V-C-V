#include "core/sim/state_hash.h"
namespace nuc::sim {
void StateHash::bytes(std::span<const std::byte> data)noexcept{for(auto b:data)mix(std::to_integer<std::uint8_t>(b));}
void StateHash::text(std::string_view value)noexcept{integer<std::uint64_t>(value.size());for(unsigned char c:value)mix(c);}
}