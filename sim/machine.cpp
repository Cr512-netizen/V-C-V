#include "sim/machine.h"
#include "core/ecs/view.h"
#include <stdexcept>
namespace nuc::sim {
void MachineSystem::tick(ecs::World& world,const RecipeBook& recipes,unsigned tier){if(tier>1)throw std::invalid_argument("machine tier must be 0 or 1");ecs::view<MachineInput,MachineOutput,MachineRecipe,MachineTimer>(world).each([&](ecs::Entity,MachineInput& input,MachineOutput& output,MachineRecipe& selected,MachineTimer& timer){if(!timer.pending_recipe.empty()){if(timer.remaining_ticks>0)--timer.remaining_ticks;if(timer.remaining_ticks>0)return;const Recipe* pending=recipes.find(timer.pending_recipe);if(!pending){timer.pending_recipe.clear();return;}if(recipes.produce_outputs(*pending,output.inventory))timer.pending_recipe.clear();return;}const Recipe* recipe=recipes.find(selected.id);if(!recipe||!recipes.can_craft(*recipe,input.inventory)||!recipes.can_output(*recipe,output.inventory))return;if(!recipes.consume_inputs(*recipe,input.inventory))return;timer.pending_recipe=recipe->id;timer.remaining_ticks=recipe->craft_ticks;if(timer.remaining_ticks>0)--timer.remaining_ticks;if(timer.remaining_ticks==0&&recipes.produce_outputs(*recipe,output.inventory))timer.pending_recipe.clear();});}
}
