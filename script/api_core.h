#pragma once
#include "core/ecs/world.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <sol/sol.hpp>
namespace nuc::sim { class RecipeBook; class EnergyNetwork; }
namespace nuc::script {
class Sandbox;
struct ApiSharedState {
 std::map<std::tuple<int,int,int>,std::uint32_t> voxels;
 std::function<std::uint32_t(int,int,int)> voxel_reader;
 std::function<void(int,int,int,std::uint32_t)> voxel_writer;
 std::function<void(int,int,int,std::uint32_t)> voxel_changed;
 std::function<void(ecs::Entity)> entity_spawned;
 sim::RecipeBook* recipes{};
 sim::EnergyNetwork* energy_network{};
 std::unordered_map<std::string,std::string> registry;
 std::unordered_map<std::string,std::string> serialized_mod_state;
 struct Listener { std::string owner; Sandbox* sandbox{}; sol::protected_function callback; };
 std::unordered_map<std::string,std::vector<Listener>> listeners;
};
class ApiCore {
public:
 ApiCore(std::string,std::shared_ptr<ApiSharedState>,ecs::World&,Sandbox*);
 void bind(sol::state&);
private:
 std::string mod_id_; std::shared_ptr<ApiSharedState> shared_; ecs::World& ecs_; Sandbox* sandbox_;
 std::unordered_map<std::string,std::string> config_;
};
}
