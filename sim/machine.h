#pragma once
#include "core/ecs/world.h"
#include "sim/items.h"
#include "sim/recipes.h"
#include <string>
namespace nuc::sim {
struct MachineInput { Inventory inventory{8}; };
struct MachineOutput { Inventory inventory{8}; };
struct MachineRecipe { std::string id; };
struct MachineTimer { unsigned remaining_ticks{}; std::string pending_recipe; };
class MachineSystem { public: static void tick(ecs::World&,const RecipeBook&,unsigned tier=0); };
}
