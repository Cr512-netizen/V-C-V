#pragma once
#include "sim/items.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
namespace nuc::sim {
struct ItemAmount {ItemId item{};std::uint32_t count{};};
struct Recipe {std::string id;std::vector<ItemAmount> inputs,outputs;std::uint32_t craft_ticks{1};};
class RecipeBook { public: void add(Recipe);void load(const std::filesystem::path&);const Recipe* find(const std::string&) const noexcept;bool can_craft(const Recipe&,const Inventory&) const noexcept;bool can_output(const Recipe&,const Inventory&) const noexcept;bool consume_inputs(const Recipe&,Inventory&) const;bool produce_outputs(const Recipe&,Inventory&) const; private:std::unordered_map<std::string,Recipe> recipes_;};
}
