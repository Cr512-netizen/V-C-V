#include "script/api_core.h"
#include "script/sandbox.h"
#include "sim/npc.h"
#include "sim/machine.h"
#include "sim/recipes.h"
#include "sim/energy.h"
#include "core/log.h"
#include <stdexcept>
#include <utility>
namespace nuc::script {
ApiCore::ApiCore(std::string id,std::shared_ptr<ApiSharedState> shared,ecs::World& ecs,Sandbox* sandbox)
 :mod_id_(std::move(id)),shared_(std::move(shared)),ecs_(ecs),sandbox_(sandbox) {}
void ApiCore::bind(sol::state& lua) {
 sol::table log_api=lua.create_table();
 log_api.set_function("info",[](const std::string& s){nuc::log::info(s,"mod");});
 log_api.set_function("warn",[](const std::string& s){nuc::log::warn(s,"mod");});
 log_api.set_function("error",[](const std::string& s){nuc::log::error(s,"mod");});
 lua["log"]=log_api;
 sol::table config=lua.create_table();
 config.set_function("get",[this](const std::string& key,sol::optional<std::string> fallback){
  auto it=config_.find(key); return it==config_.end()?fallback.value_or(std::string{}):it->second;
 });
 config.set_function("set",[this](const std::string& key,const std::string& value){
  if(key.empty()) throw std::invalid_argument("config key cannot be empty"); config_[key]=value;
 });
 lua["config"]=config;
 sol::table registry=lua.create_table();
 registry.set_function("register",[this](const std::string& key,const std::string& value){
  if(key.empty()) throw std::invalid_argument("registry key cannot be empty");
  auto [it,inserted]=shared_->registry.emplace(key,value);
  if(!inserted&&it->second!=value) throw std::runtime_error("registry key already registered: "+key);
 });
 registry.set_function("get",[this](const std::string& key)->sol::optional<std::string>{
  auto it=shared_->registry.find(key); if(it==shared_->registry.end()) return sol::nullopt; return it->second;
 });
 lua["registry"]=registry;
 sol::table events=lua.create_table();
 events.set_function("on",[this](const std::string& name,sol::protected_function fn){
  if(name.empty()||!fn.valid()) throw std::invalid_argument("events.on requires a name and function");
  shared_->listeners[name].push_back({mod_id_,sandbox_,std::move(fn)});
 });
 events.set_function("emit",[this](const std::string& name,const std::string& payload){
  auto it=shared_->listeners.find(name); if(it==shared_->listeners.end()) return;
  for(auto& listener:it->second) {
   std::string error;
   const bool ok=listener.sandbox ? listener.sandbox->call(listener.callback,payload,&error) : false;
   if(!ok) nuc::log::error("event '"+name+"' listener failed: "+error,"mod");
  }
 });
 lua["events"]=events;
 sol::table world=lua.create_table();
 world.set_function("get_voxel",[this](int x,int y,int z){
  if(shared_->voxel_reader) return shared_->voxel_reader(x,y,z);
  auto it=shared_->voxels.find({x,y,z}); return it==shared_->voxels.end()?0u:it->second;
 });
 world.set_function("set_voxel",[this](int x,int y,int z,std::uint32_t material){
  if(shared_->voxel_writer) shared_->voxel_writer(x,y,z,material);
  else if(material==0) shared_->voxels.erase({x,y,z}); else shared_->voxels[{x,y,z}]=material;
  if(shared_->voxel_changed) shared_->voxel_changed(x,y,z,material);
 });
 lua["world"]=world;
 sol::table ecs_api=lua.create_table();
 ecs_api.set_function("create",[this](){return ecs_.create();});
 ecs_api.set_function("destroy",[this](ecs::Entity e){if(ecs_.alive(e)) ecs_.destroy(e);});
 ecs_api.set_function("alive",[this](ecs::Entity e){return ecs_.alive(e);});
 ecs_api.set_function("entity_count",[this](){return ecs_.entity_count();});
 lua["ecs"]=ecs_api;
 sol::table npc_api=lua.create_table();
 npc_api.set_function("spawn",[this](int x,int y,int z){
  const auto entity=sim::NpcSystem::spawn(ecs_,x,y,z);
  if(shared_->entity_spawned) shared_->entity_spawned(entity);
  return entity;
 });
 npc_api.set_function("move_to",[this](ecs::Entity e,int x,int y,int z){
  if(!ecs_.alive(e)||!ecs_.has<sim::NpcGoal>(e)||!ecs_.has<sim::NpcPosition>(e))return false;
  auto& goal=ecs_.get<sim::NpcGoal>(e);goal.target_x=x;goal.target_y=y;goal.target_z=z;goal.completed=false;goal.state=sim::NpcState::GoTo;return true;
 });
 npc_api.set_function("mine",[this](ecs::Entity e,std::uint32_t item,std::uint32_t count,std::uint32_t ticks){
  if(!ecs_.alive(e)||!ecs_.has<sim::NpcGoal>(e)||item==0||count==0||ticks==0)return false;
  auto& goal=ecs_.get<sim::NpcGoal>(e);goal.work_item=item;goal.work_count=count;goal.work_ticks=ticks;goal.completed=false;return true;
 });
 npc_api.set_function("place",[this](int x,int y,int z,std::uint32_t material){
  if(shared_->voxel_writer) shared_->voxel_writer(x,y,z,material);
  else if(material==0) shared_->voxels.erase({x,y,z}); else shared_->voxels[{x,y,z}]=material;
  if(shared_->voxel_changed) shared_->voxel_changed(x,y,z,material);
  return true;
 });
 npc_api.set_function("trade",[this](ecs::Entity from,ecs::Entity to,std::uint32_t item,std::uint32_t count){
  if(!ecs_.alive(from)||!ecs_.alive(to)||from==to||item==0||count==0||!ecs_.has<sim::NpcInventory>(from)||!ecs_.has<sim::NpcInventory>(to))return false;
  auto& source=ecs_.get<sim::NpcInventory>(from).inventory;auto& target=ecs_.get<sim::NpcInventory>(to).inventory;
  if(source.count(item)<count||!target.can_insert({item,count,64}))return false;
  if(source.remove(item,count)!=count)return false;return target.insert({item,count,64})==count;
 });
 lua["npc"]=npc_api;
 sol::table machine_api=lua.create_table();
 machine_api.set_function("create",[this](const std::string& recipe_id)->ecs::Entity {
  if(!shared_->recipes||!shared_->recipes->find(recipe_id)) return ecs::null_entity;
  const auto entity=ecs_.create();
  try {
   ecs_.emplace<sim::MachineInput>(entity);
   ecs_.emplace<sim::MachineOutput>(entity);
   ecs_.emplace<sim::MachineRecipe>(entity,sim::MachineRecipe{recipe_id});
   ecs_.emplace<sim::MachineTimer>(entity);
  } catch(...) { ecs_.destroy(entity); throw; }
  if(shared_->entity_spawned) shared_->entity_spawned(entity);
  return entity;
 });
 machine_api.set_function("set_recipe",[this](ecs::Entity entity,const std::string& recipe_id) {
  if(!ecs_.alive(entity)||!ecs_.has<sim::MachineRecipe>(entity)||!ecs_.has<sim::MachineTimer>(entity)||!shared_->recipes||!shared_->recipes->find(recipe_id)) return false;
  ecs_.get<sim::MachineRecipe>(entity).id=recipe_id;
  ecs_.get<sim::MachineTimer>(entity).pending_recipe.clear();
  ecs_.get<sim::MachineTimer>(entity).remaining_ticks=0;
  return true;
 });
 machine_api.set_function("insert_input",[this](ecs::Entity entity,sim::ItemId item,std::uint32_t count)->std::uint32_t {
  if(!ecs_.alive(entity)||!ecs_.has<sim::MachineInput>(entity)||item==0||count==0) return 0;
  return ecs_.get<sim::MachineInput>(entity).inventory.insert({item,count,64});
 });
 machine_api.set_function("input_count",[this](ecs::Entity entity,sim::ItemId item)->std::uint32_t {
  if(!ecs_.alive(entity)||!ecs_.has<sim::MachineInput>(entity)) return 0;
  return ecs_.get<sim::MachineInput>(entity).inventory.count(item);
 });
 machine_api.set_function("output_count",[this](ecs::Entity entity,sim::ItemId item)->std::uint32_t {
  if(!ecs_.alive(entity)||!ecs_.has<sim::MachineOutput>(entity)) return 0;
  return ecs_.get<sim::MachineOutput>(entity).inventory.count(item);
 });
 machine_api.set_function("take_output",[this](ecs::Entity entity,sim::ItemId item,std::uint32_t count)->std::uint32_t {
  if(!ecs_.alive(entity)||!ecs_.has<sim::MachineOutput>(entity)||item==0||count==0) return 0;
  return ecs_.get<sim::MachineOutput>(entity).inventory.remove(item,count);
 });
 lua["machine"]=machine_api;
 sol::table energy_api=lua.create_table();
 energy_api.set_function("has",[this](std::uint32_t id) {
  return shared_->energy_network&&shared_->energy_network->find(id)!=nullptr;
 });
 energy_api.set_function("add_producer",[this](std::uint32_t id,std::int64_t production) {
  auto* net=shared_->energy_network;
  if(!net||id==0||production<0||net->find(id)) return false;
  net->add({id,sim::EnergyRole::Producer,production});
  return true;
 });
 energy_api.set_function("add_consumer",[this](std::uint32_t id,std::int64_t demand,int priority) {
  auto* net=shared_->energy_network;
  if(!net||id==0||demand<0||net->find(id)) return false;
  net->add({id,sim::EnergyRole::Consumer,0,demand,0,0,0,priority});
  return true;
 });
 energy_api.set_function("add_storage",[this](std::uint32_t id,std::int64_t capacity,std::int64_t stored) {
  auto* net=shared_->energy_network;
  if(!net||id==0||capacity<0||stored<0||stored>capacity||net->find(id)) return false;
  net->add({id,sim::EnergyRole::Storage,0,0,capacity,stored});
  return true;
 });
 energy_api.set_function("allocated",[this](std::uint32_t id)->std::int64_t {
  if(!shared_->energy_network) return 0;
  const auto* node=shared_->energy_network->find(id); return node?node->allocated:0;
 });
 energy_api.set_function("stored",[this](std::uint32_t id)->std::int64_t {
  if(!shared_->energy_network) return 0;
  const auto* node=shared_->energy_network->find(id); return node?node->stored:0;
 });
 energy_api.set_function("produced_last_tick",[this]()->std::int64_t {
  return shared_->energy_network?shared_->energy_network->produced_last_tick():0;
 });
 energy_api.set_function("consumed_last_tick",[this]()->std::int64_t {
  return shared_->energy_network?shared_->energy_network->consumed_last_tick():0;
 });
 lua["energy"]=energy_api;
}
}


