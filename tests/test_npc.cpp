#include <catch2/catch_test_macros.hpp>
#include "sim/npc.h"
TEST_CASE("NPC reaches work target and gathers items") {
 nuc::ecs::World world;auto e=nuc::sim::NpcSystem::spawn(world,0,0,0);auto& goal=world.get<nuc::sim::NpcGoal>(e);goal.target_x=3;goal.work_item=2;goal.work_count=1;goal.work_ticks=2;
 for(int i=0;i<6;++i)nuc::sim::NpcSystem::tick(world);
 REQUIRE(world.get<nuc::sim::NpcPosition>(e).x==3);REQUIRE(world.get<nuc::sim::NpcInventory>(e).inventory.count(2)==1);REQUIRE(goal.completed);
}
TEST_CASE("civilization accumulates deterministic resources at one hertz") {
 nuc::sim::Civilization civ;civ.id=1;civ.population=20;civ.production_per_second[3]=2;std::vector<nuc::sim::Civilization> civs{civ};
 for(int i=0;i<1000;++i)nuc::sim::CivilizationSystem::tick(civs,15);
 REQUIRE(civs[0].resources[3]==132);REQUIRE(civs[0].ticks_into_second==10);
}
TEST_CASE("NPC tier migration preserves inventory") {
 nuc::ecs::World world;auto e=nuc::sim::NpcSystem::spawn(world,100,0,0);auto& inv=world.get<nuc::sim::NpcInventory>(e).inventory;inv.insert({5,4,64});
 nuc::sim::NpcSystem::update_tiers(world,0,0,0,10);REQUIRE(world.get<nuc::sim::NpcLod>(e).tier==nuc::sim::NpcTier::Tier2);
 nuc::sim::NpcSystem::update_tiers(world,100,0,0,10);REQUIRE(world.get<nuc::sim::NpcLod>(e).tier==nuc::sim::NpcTier::Tier0);REQUIRE(inv.count(5)==4);
}
