#include "sim/items.h"
#include <algorithm>
#include <stdexcept>
namespace nuc::sim {
const ItemStack& Inventory::slot(std::size_t i) const {if(i>=slots_.size())throw std::out_of_range("inventory slot");return slots_[i];}
ItemStack& Inventory::slot(std::size_t i) {if(i>=slots_.size())throw std::out_of_range("inventory slot");return slots_[i];}
std::uint32_t Inventory::count(ItemId item) const noexcept {std::uint32_t total=0;for(const auto& s:slots_)if(s.item==item)total+=s.count;return total;}
bool Inventory::can_insert(const ItemStack& stack) const noexcept {if(stack.empty()||stack.max_stack==0)return false;std::uint64_t room=0;for(const auto& s:slots_){if(s.empty())room+=stack.max_stack;else if(s.item==stack.item&&s.max_stack==stack.max_stack&&s.count<s.max_stack)room+=s.max_stack-s.count;}return room>=stack.count;}
std::uint32_t Inventory::insert(ItemStack stack) {if(stack.empty()||stack.max_stack==0)return 0;const auto original=stack.count;for(auto& s:slots_)if(s.item==stack.item&&s.max_stack==stack.max_stack&&s.count<s.max_stack){const auto n=std::min(stack.count,s.max_stack-s.count);s.count+=n;stack.count-=n;if(stack.count==0)return original;}for(auto& s:slots_)if(s.empty()){const auto n=std::min(stack.count,stack.max_stack);s={stack.item,n,stack.max_stack};stack.count-=n;if(stack.count==0)return original;}return original-stack.count;}
ItemStack Inventory::extract(std::size_t i,std::uint32_t amount){auto& s=slot(i);if(s.empty()||amount==0)return {};const auto n=std::min(amount,s.count);ItemStack out{s.item,n,s.max_stack};s.count-=n;if(s.count==0)s={};return out;}
std::uint32_t Inventory::remove(ItemId item,std::uint32_t amount){std::uint32_t removed=0;for(auto& s:slots_)if(s.item==item&&amount){const auto n=std::min(amount,s.count);s.count-=n;amount-=n;removed+=n;if(s.count==0)s={};}return removed;}
bool Inventory::move_to(std::size_t from,Inventory& dest,std::size_t to,std::uint32_t amount){auto& src=slot(from);auto& target=dest.slot(to);if(src.empty()||amount==0)return false;if(!target.empty()&&(target.item!=src.item||target.max_stack!=src.max_stack))return false;const auto room=target.empty()?src.max_stack:target.max_stack-target.count;const auto n=std::min({amount,src.count,room});if(n==0)return false;if(target.empty())target={src.item,0,src.max_stack};target.count+=n;src.count-=n;if(src.count==0)src={};return true;}
}
