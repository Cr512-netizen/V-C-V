#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace nuc::sim {
using ItemId=std::uint32_t;
struct ItemStack { ItemId item{}; std::uint32_t count{}; std::uint32_t max_stack{64}; bool empty() const noexcept { return item==0||count==0; } };
class Inventory { public: explicit Inventory(std::size_t slots=16):slots_(slots){} std::size_t size() const noexcept{return slots_.size();} const ItemStack& slot(std::size_t) const; ItemStack& slot(std::size_t); std::uint32_t count(ItemId) const noexcept; bool can_insert(const ItemStack&) const noexcept; std::uint32_t insert(ItemStack); ItemStack extract(std::size_t,std::uint32_t); std::uint32_t remove(ItemId,std::uint32_t); bool move_to(std::size_t,Inventory&,std::size_t,std::uint32_t); private: std::vector<ItemStack> slots_; };
}
