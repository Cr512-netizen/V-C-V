#include <catch2/catch_test_macros.hpp>
#include "core/ecs/world.h"
#include "sim/belt.h"
#include "sim/machine.h"
TEST_CASE("recipe machine consumes input and outputs after craft ticks") {
 nuc::sim::RecipeBook book;book.add({"crush",{{1,2}},{{2,1}},3});nuc::ecs::World world;auto e=world.create();
 auto& input=world.emplace<nuc::sim::MachineInput>(e);input.inventory.insert({1,2,64});world.emplace<nuc::sim::MachineOutput>(e);world.emplace<nuc::sim::MachineRecipe>(e,nuc::sim::MachineRecipe{"crush"});world.emplace<nuc::sim::MachineTimer>(e);
 nuc::sim::MachineSystem::tick(world,book);REQUIRE(input.inventory.count(1)==0);REQUIRE(world.get<nuc::sim::MachineOutput>(e).inventory.count(2)==0);
 nuc::sim::MachineSystem::tick(world,book);REQUIRE(world.get<nuc::sim::MachineOutput>(e).inventory.count(2)==0);
 nuc::sim::MachineSystem::tick(world,book);REQUIRE(world.get<nuc::sim::MachineOutput>(e).inventory.count(2)==1);
}
TEST_CASE("belt moves items to its end one position per tick") {
 nuc::sim::Belt belt(4);REQUIRE(belt.insert({7,1,64}));belt.tick();REQUIRE(belt.at(1).has_value());belt.tick();belt.tick();auto item=belt.take_end();REQUIRE(item.item==7);REQUIRE(item.count==1);REQUIRE(belt.take_end().empty());
}
TEST_CASE("Tier 0 and Tier 1 machines produce identical counts") {
 nuc::sim::RecipeBook book;book.add({"crush",{{1,2}},{{2,1}},3});auto setup=[](nuc::ecs::World& w){auto e=w.create();w.emplace<nuc::sim::MachineInput>(e).inventory.insert({1,20,64});w.emplace<nuc::sim::MachineOutput>(e);w.emplace<nuc::sim::MachineRecipe>(e,nuc::sim::MachineRecipe{"crush"});w.emplace<nuc::sim::MachineTimer>(e);return e;};
 nuc::ecs::World a,b;auto ea=setup(a),eb=setup(b);for(int i=0;i<30;++i){nuc::sim::MachineSystem::tick(a,book,0);nuc::sim::MachineSystem::tick(b,book,1);}
 REQUIRE(a.get<nuc::sim::MachineInput>(ea).inventory.count(1)==b.get<nuc::sim::MachineInput>(eb).inventory.count(1));REQUIRE(a.get<nuc::sim::MachineOutput>(ea).inventory.count(2)==b.get<nuc::sim::MachineOutput>(eb).inventory.count(2));
}
