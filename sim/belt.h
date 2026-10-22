#pragma once
#include "sim/items.h"
#include <cstddef>
#include <optional>
#include <vector>
namespace nuc::sim {
class Belt { public: explicit Belt(std::size_t length);std::size_t length() const noexcept{return slots_.size();}bool insert(ItemStack);void tick() noexcept;const std::optional<ItemStack>& at(std::size_t) const;ItemStack take_end();private:std::vector<std::optional<ItemStack>> slots_;};
}
