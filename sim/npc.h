#pragma once
#include "core/ecs/world.h"
#include "sim/items.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>
namespace nuc::sim {
enum class NpcState {Idle,GoTo,Work,Eat,Sleep};
enum class NpcTier {Tier0,Tier1,Tier2};
struct NpcTag {};
struct NpcGoal {NpcState state{NpcState::Idle};int target_x{},target_y{},target_z{};ItemId work_item{1};std::uint32_t work_count{1},work_ticks{1},remaining_ticks{};bool completed{};};
struct NpcPosition {int x{},y{},z{};};
struct NpcInventory {Inventory inventory{12};};
struct NpcLod {NpcTier tier{NpcTier::Tier0};};
class NpcSystem {public:static ecs::Entity spawn(ecs::World&,int x,int y,int z);static void tick(ecs::World&);static void update_tiers(ecs::World&,int px,int py,int pz,int radius);};
struct Civilization {std::uint64_t id{};std::uint64_t population{};std::map<ItemId,std::int64_t> resources,production_per_second;std::unordered_map<std::uint64_t,int> relations;unsigned ticks_into_second{};};
class CivilizationSystem {public:static void tick(std::vector<Civilization>&,unsigned tick_rate=15);};
}
