#include "sim/belt.h"
#include <stdexcept>
#include <utility>
namespace nuc::sim {
Belt::Belt(std::size_t length):slots_(length){if(length==0)throw std::invalid_argument("belt length must be positive");}
bool Belt::insert(ItemStack stack){if(stack.empty()||slots_.front())return false;slots_.front()=stack;return true;}
void Belt::tick() noexcept{for(std::size_t i=slots_.size()-1;i>0;--i)if(!slots_[i]&&slots_[i-1]){slots_[i]=std::move(slots_[i-1]);slots_[i-1].reset();}}
const std::optional<ItemStack>& Belt::at(std::size_t i) const{if(i>=slots_.size())throw std::out_of_range("belt position");return slots_[i];}
ItemStack Belt::take_end(){if(!slots_.back())return {};auto out=*slots_.back();slots_.back().reset();return out;}
}
