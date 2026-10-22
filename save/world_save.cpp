#include "save/world_save.h"
#include "core/sim/state_hash.h"
#include "core/ecs/view.h"
#include "sim/npc.h"
#include "sim/machine.h"
#include "physics/body.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>
#include <stdexcept>
namespace nuc::save {
namespace {using Json=nlohmann::json;
Json body_json(const physics::RigidBody& b){return Json{{"position",{b.position.x,b.position.y,b.position.z}},{"previous_position",{b.previous_position.x,b.previous_position.y,b.previous_position.z}},{"velocity",{b.velocity.x,b.velocity.y,b.velocity.z}},{"half_extents",{b.half_extents.x,b.half_extents.y,b.half_extents.z}},{"inverse_mass",b.inverse_mass},{"restitution",b.restitution},{"friction",b.friction},{"sleep_time",b.sleep_time},{"awake",b.awake},{"material",b.material},{"owner_chunk_x",b.owner_chunk_x},{"owner_chunk_z",b.owner_chunk_z}};}
glm::vec3 read_vec3(const Json& j,const char* key){const auto& a=j.at(key);if(!a.is_array()||a.size()!=3)throw std::runtime_error(std::string("invalid physics body vector: ")+key);return {a[0].get<float>(),a[1].get<float>(),a[2].get<float>()};}
physics::RigidBody parse_body(const Json& j){const auto p=read_vec3(j,"position"),ext=read_vec3(j,"half_extents");const float inv=j.at("inverse_mass").get<float>();if(!std::isfinite(inv)||inv<0.0F)throw std::runtime_error("invalid saved inverse mass");auto b=physics::make_body(p,ext,inv>0.0F?1.0F/inv:0.0F,j.at("material").get<std::uint32_t>());b.inverse_mass=inv;b.previous_position=read_vec3(j,"previous_position");b.velocity=read_vec3(j,"velocity");b.restitution=std::clamp(j.at("restitution").get<float>(),0.0F,1.0F);b.friction=std::max(0.0F,j.at("friction").get<float>());b.sleep_time=j.at("sleep_time").get<float>();if(!std::isfinite(b.sleep_time)||b.sleep_time<0.0F||!std::isfinite(b.velocity.x)||!std::isfinite(b.velocity.y)||!std::isfinite(b.velocity.z)||!std::isfinite(b.previous_position.x)||!std::isfinite(b.previous_position.y)||!std::isfinite(b.previous_position.z))throw std::runtime_error("invalid saved physics body state");b.awake=j.at("awake").get<bool>();b.owner_chunk_x=j.at("owner_chunk_x").get<int>();b.owner_chunk_z=j.at("owner_chunk_z").get<int>();b.force=glm::vec3(0.0F);return b;}
Json chunk_json(const SavedChunk& c){Json delta=Json::array();for(const auto& d:c.delta)delta.push_back({d.index,d.material});return Json{{"x",c.x},{"z",c.z},{"delta",delta}};}
Json entity_json(const SavedEntity& e){Json inv=Json::array();for(const auto& s:e.inventory)inv.push_back({{"item",s.item},{"count",s.count},{"max_stack",s.max_stack}});return Json{{"id",e.id},{"kind",e.kind},{"x",e.x},{"y",e.y},{"z",e.z},{"inventory",inv},{"recipe",e.recipe},{"timer",e.timer},{"extra",e.extra}};}
Json civ_json(const SavedCivilization& c){Json resources=Json::object(),production=Json::object(),relations=Json::object();for(auto [id,n]:c.resources)resources[std::to_string(id)]=n;for(auto [id,n]:c.production_per_second)production[std::to_string(id)]=n;for(auto [id,value]:c.relations)relations[std::to_string(id)]=value;return Json{{"id",c.id},{"population",c.population},{"resources",resources},{"production_per_second",production},{"relations",relations},{"ticks_into_second",c.ticks_into_second}};}
SavedVoxelEdit parse_edit(const Json& j){SavedVoxelEdit e{j.at("x").get<int>(),j.at("y").get<int>(),j.at("z").get<int>(),j.at("material").get<std::uint32_t>()};if(e.y<0||e.y>=static_cast<int>(world::Chunk::extent))throw std::runtime_error("voxel edit y is outside world bounds");return e;}
SavedChunk parse_chunk(const Json& j){SavedChunk c;c.x=j.at("x").get<std::int32_t>();c.z=j.at("z").get<std::int32_t>();if(!j.at("delta").is_array())throw std::runtime_error("chunk delta must be array");std::set<std::uint32_t> seen;for(const auto& pair:j.at("delta")){if(!pair.is_array()||pair.size()!=2)throw std::runtime_error("invalid voxel delta");VoxelDelta d{pair[0].get<std::uint32_t>(),pair[1].get<std::uint32_t>()};if(d.index>=world::Chunk::voxel_count||d.material==0||!seen.insert(d.index).second)throw std::runtime_error("invalid or duplicate voxel delta");c.delta.push_back(d);}std::sort(c.delta.begin(),c.delta.end(),[](auto a,auto b){return a.index<b.index;});return c;}
void parse_map(const Json& j,std::map<sim::ItemId,std::int64_t>& out){if(!j.is_object())throw std::runtime_error("civilization resource map must be object");for(auto it=j.begin();it!=j.end();++it){auto id=static_cast<sim::ItemId>(std::stoul(it.key()));auto amount=it.value().get<std::int64_t>();out[id]=amount;}}
}
std::uint64_t WorldSaveData::hash() const {sim::StateHash h;h.integer(format_version);h.integer(seed);auto sorted=chunks;std::sort(sorted.begin(),sorted.end(),[](const auto&a,const auto&b){return a.x==b.x?a.z<b.z:a.x<b.x;});for(const auto& c:sorted){h.integer(c.x);h.integer(c.z);auto delta=c.delta;std::sort(delta.begin(),delta.end(),[](auto a,auto b){return a.index<b.index;});for(const auto& d:delta){h.integer(d.index);h.integer(d.material);}}auto edits=voxel_edits;std::sort(edits.begin(),edits.end(),[](const auto&a,const auto&b){if(a.x!=b.x)return a.x<b.x;if(a.y!=b.y)return a.y<b.y;return a.z<b.z;});for(const auto&e:edits){h.integer(e.x);h.integer(e.y);h.integer(e.z);h.integer(e.material);}auto ents=entities;std::sort(ents.begin(),ents.end(),[](const auto&a,const auto&b){return a.id<b.id;});for(const auto& e:ents){h.integer(e.id);h.text(e.kind);h.integer(e.x);h.integer(e.y);h.integer(e.z);h.text(e.recipe);h.integer(e.timer);h.text(e.extra.dump());for(const auto& s:e.inventory){h.integer(s.item);h.integer(s.count);h.integer(s.max_stack);}}auto civs=civilizations;std::sort(civs.begin(),civs.end(),[](const auto&a,const auto&b){return a.id<b.id;});for(const auto& c:civs){h.integer(c.id);h.integer(c.population);for(auto [id,n]:c.resources){h.integer(id);h.integer(n);}for(auto [id,n]:c.production_per_second){h.integer(id);h.integer(n);}h.integer(c.ticks_into_second);for(auto [id,relation]:c.relations){h.integer(id);h.integer(relation);}}for(const auto& [id,state]:mod_state){h.text(id);h.text(state);}return h.value();}
std::vector<SavedChunk> WorldSave::encode_chunks(const std::vector<world::Chunk>& chunks){std::vector<SavedChunk> out;std::set<std::pair<int,int>> coords;for(const auto& c:chunks){if(!coords.emplace(c.chunk_x(),c.chunk_z()).second)throw std::runtime_error("duplicate chunk coordinates");SavedChunk d;d.x=c.chunk_x();d.z=c.chunk_z();for(std::uint32_t i=0;i<c.voxels().size();++i)if(c.voxels()[i]!=0)d.delta.push_back({i,c.voxels()[i]});out.push_back(std::move(d));}return out;}
std::vector<world::Chunk> WorldSave::decode_chunks(const std::vector<SavedChunk>& chunks){std::vector<world::Chunk> out;std::set<std::pair<int,int>> coords;for(const auto& c:chunks){if(!coords.emplace(c.x,c.z).second)throw std::runtime_error("duplicate chunk coordinates");world::Chunk chunk(c.x,c.z);std::set<std::uint32_t> seen;for(const auto& d:c.delta){if(d.index>=world::Chunk::voxel_count||d.material==0||!seen.insert(d.index).second)throw std::runtime_error("invalid voxel delta");const auto x=d.index%world::Chunk::extent;const auto y=(d.index/world::Chunk::extent)%world::Chunk::extent;const auto z=d.index/(world::Chunk::extent*world::Chunk::extent);chunk.set(x,y,z,d.material);}out.push_back(std::move(chunk));}return out;}
void WorldSave::save(const std::filesystem::path& path,const WorldSaveData& data){
 if(data.format_version!=1&&data.format_version!=2)throw std::runtime_error("unsupported world save version");
 Json chunks=Json::array(),edits=Json::array(),entities=Json::array(),civs=Json::array(),mods=Json::object();
 auto sorted=data.chunks;std::sort(sorted.begin(),sorted.end(),[](const auto&a,const auto&b){return a.x==b.x?a.z<b.z:a.x<b.x;});
 for(const auto& c:sorted)chunks.push_back(chunk_json(c));
 auto se=data.voxel_edits;std::sort(se.begin(),se.end(),[](const auto&a,const auto&b){if(a.x!=b.x)return a.x<b.x;if(a.y!=b.y)return a.y<b.y;return a.z<b.z;});
 for(std::size_t i=0;i<se.size();++i){const auto&e=se[i];if(e.y<0||e.y>=static_cast<int>(world::Chunk::extent))throw std::runtime_error("voxel edit y is outside world bounds");if(i&&e.x==se[i-1].x&&e.y==se[i-1].y&&e.z==se[i-1].z)throw std::runtime_error("duplicate voxel edit coordinate");edits.push_back({{"x",e.x},{"y",e.y},{"z",e.z},{"material",e.material}});}
 for(const auto& e:data.entities)entities.push_back(entity_json(e));
 for(const auto& c:data.civilizations)civs.push_back(civ_json(c));
 for(const auto& [id,state]:data.mod_state)mods[id]=state;
 Json root{{"version",data.format_version},{"seed",data.seed},{"chunks",chunks},{"entities",entities},{"civilizations",civs},{"mod_state",mods}};
 if(data.format_version==2)root["voxel_edits"]=edits;
 if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());
 auto tmp=path;tmp += ".tmp";
 {std::ofstream out(tmp,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("cannot create world save temp file");out<<root.dump();out.flush();if(!out)throw std::runtime_error("failed writing world save");}
 std::error_code ec;std::filesystem::rename(tmp,path,ec);
 if(ec){auto backup=path;backup += ".bak";std::error_code ignored;std::filesystem::remove(backup,ignored);ec.clear();std::filesystem::rename(path,backup,ec);if(ec)throw std::runtime_error("cannot preserve previous world save: "+ec.message());ec.clear();std::filesystem::rename(tmp,path,ec);if(ec){std::error_code restore;std::filesystem::rename(backup,path,restore);throw std::runtime_error("cannot replace world save: "+ec.message());}std::filesystem::remove(backup,ignored);}
}
WorldSaveData WorldSave::load(const std::filesystem::path& path){
 std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("cannot open world save");
 Json root;try{in>>root;}catch(const Json::exception& e){throw std::runtime_error("invalid world save: "+std::string(e.what()));}
 const auto version=root.value("version",0u);if(version!=1&&version!=2)throw std::runtime_error("unsupported world save version");
 WorldSaveData data;data.format_version=version;data.seed=root.at("seed").get<std::uint64_t>();
 if(root.contains("chunks"))for(const auto& j:root.at("chunks"))data.chunks.push_back(parse_chunk(j));
 if(version==2){if(!root.contains("voxel_edits")||!root["voxel_edits"].is_array())throw std::runtime_error("voxel_edits must be an array");std::set<std::tuple<int,int,int>> seen;for(const auto& j:root["voxel_edits"]){auto e=parse_edit(j);if(!seen.emplace(e.x,e.y,e.z).second)throw std::runtime_error("duplicate voxel edit coordinate");data.voxel_edits.push_back(e);}}
 for(const auto& j:root.at("entities")){SavedEntity e;e.id=j.at("id").get<std::uint64_t>();e.kind=j.at("kind").get<std::string>();e.x=j.at("x").get<int>();e.y=j.at("y").get<int>();e.z=j.at("z").get<int>();e.recipe=j.at("recipe").get<std::string>();e.timer=j.at("timer").get<std::uint32_t>();e.extra=j.value("extra",Json::object());for(const auto& s:j.at("inventory"))e.inventory.push_back({s.at("item").get<sim::ItemId>(),s.at("count").get<std::uint32_t>(),s.at("max_stack").get<std::uint32_t>()});data.entities.push_back(std::move(e));}for(const auto& j:root.at("civilizations")){SavedCivilization c;c.id=j.at("id").get<std::uint64_t>();c.population=j.at("population").get<std::uint64_t>();parse_map(j.at("resources"),c.resources);parse_map(j.at("production_per_second"),c.production_per_second);c.ticks_into_second=j.value("ticks_into_second",0u);if(j.contains("relations")){if(!j["relations"].is_object())throw std::runtime_error("civilization relations must be object");for(auto it=j["relations"].begin();it!=j["relations"].end();++it)c.relations[std::stoull(it.key())]=it.value().get<int>();}data.civilizations.push_back(std::move(c));}const auto& mods=root.at("mod_state");if(!mods.is_object())throw std::runtime_error("mod_state must be an object");for(auto it=mods.begin();it!=mods.end();++it)data.mod_state[it.key()]=it.value().get<std::string>();return data;}

std::vector<SavedEntity> WorldSave::capture_entities(ecs::World& ecs){
 std::vector<SavedEntity> out;
 auto save_inventory=[](const sim::Inventory& inv){std::vector<sim::ItemStack> stacks;for(std::size_t i=0;i<inv.size();++i){const auto& s=inv.slot(i);if(!s.empty())stacks.push_back(s);}return stacks;};
 ecs::view<sim::NpcTag,sim::NpcGoal,sim::NpcPosition,sim::NpcInventory,sim::NpcLod>(ecs).each([&](ecs::Entity id,sim::NpcTag&,sim::NpcGoal& g,sim::NpcPosition& p,sim::NpcInventory& inv,sim::NpcLod& lod){
  SavedEntity e;e.id=id;e.kind="npc";e.x=p.x;e.y=p.y;e.z=p.z;e.inventory=save_inventory(inv.inventory);e.timer=g.remaining_ticks;
  e.extra={{"state",static_cast<int>(g.state)},{"target_x",g.target_x},{"target_y",g.target_y},{"target_z",g.target_z},{"work_item",g.work_item},{"work_count",g.work_count},{"work_ticks",g.work_ticks},{"completed",g.completed},{"tier",static_cast<int>(lod.tier)}};
  out.push_back(std::move(e));
 });
 ecs::view<sim::MachineInput,sim::MachineOutput,sim::MachineRecipe,sim::MachineTimer>(ecs).each([&](ecs::Entity id,sim::MachineInput& in,sim::MachineOutput& output,sim::MachineRecipe& recipe,sim::MachineTimer& timer){
  SavedEntity e;e.id=id;e.kind="machine";e.recipe=recipe.id;e.timer=timer.remaining_ticks;e.inventory=save_inventory(in.inventory);Json out_inv=Json::array();for(const auto& s:save_inventory(output.inventory))out_inv.push_back({{"item",s.item},{"count",s.count},{"max_stack",s.max_stack}});e.extra={{"pending_recipe",timer.pending_recipe},{"output_inventory",out_inv}};out.push_back(std::move(e));
 });
 ecs::view<physics::RigidBody>(ecs).each([&](ecs::Entity id,physics::RigidBody& body){
  auto it=std::find_if(out.begin(),out.end(),[&](const auto& saved){return saved.id==id;});
  if(it==out.end()){SavedEntity e;e.id=id;e.kind="physics_body";e.extra=Json::object();out.push_back(std::move(e));it=std::prev(out.end());}
  it->extra["physics_body"]=body_json(body);
  it->x=static_cast<int>(std::floor(body.position.x));it->y=static_cast<int>(std::floor(body.position.y));it->z=static_cast<int>(std::floor(body.position.z));
 });
 std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.id<b.id;});return out;
}
void WorldSave::restore_entities(ecs::World& ecs,const std::vector<SavedEntity>& entities){
 auto sorted=entities;std::sort(sorted.begin(),sorted.end(),[](const auto&a,const auto&b){return a.id<b.id;});
 auto restore_inventory=[](sim::Inventory& inv,const std::vector<sim::ItemStack>& stacks){for(const auto& s:stacks)if(inv.insert(s)!=s.count)throw std::runtime_error("saved inventory exceeds capacity");};
 for(const auto& e:sorted){
  const auto id=ecs.create_at(e.id);
  if(e.kind=="npc"){
   ecs.emplace<sim::NpcTag>(id);ecs.emplace<sim::NpcGoal>(id);ecs.emplace<sim::NpcPosition>(id,sim::NpcPosition{e.x,e.y,e.z});ecs.emplace<sim::NpcInventory>(id);ecs.emplace<sim::NpcLod>(id);
   auto& g=ecs.get<sim::NpcGoal>(id);g.state=static_cast<sim::NpcState>(e.extra.value("state",0));g.target_x=e.extra.value("target_x",e.x);g.target_y=e.extra.value("target_y",e.y);g.target_z=e.extra.value("target_z",e.z);g.work_item=e.extra.value("work_item",1u);g.work_count=e.extra.value("work_count",1u);g.work_ticks=e.extra.value("work_ticks",1u);g.remaining_ticks=e.timer;g.completed=e.extra.value("completed",false);
   const int tier=e.extra.value("tier",0);if(tier<0||tier>2)throw std::runtime_error("invalid saved NPC tier");ecs.get<sim::NpcLod>(id).tier=static_cast<sim::NpcTier>(tier);restore_inventory(ecs.get<sim::NpcInventory>(id).inventory,e.inventory);
  }else if(e.kind=="machine"){
   auto& in=ecs.emplace<sim::MachineInput>(id);restore_inventory(in.inventory,e.inventory);auto& output=ecs.emplace<sim::MachineOutput>(id);
   if(e.extra.contains("output_inventory"))for(const auto& s:e.extra["output_inventory"])if(output.inventory.insert({s.at("item").get<sim::ItemId>(),s.at("count").get<std::uint32_t>(),s.at("max_stack").get<std::uint32_t>()})!=s.at("count").get<std::uint32_t>())throw std::runtime_error("saved machine output exceeds capacity");
   ecs.emplace<sim::MachineRecipe>(id,sim::MachineRecipe{e.recipe});ecs.emplace<sim::MachineTimer>(id,sim::MachineTimer{e.timer,e.extra.value("pending_recipe",std::string{})});
  }
  if(e.extra.contains("physics_body"))ecs.emplace<physics::RigidBody>(id,parse_body(e.extra.at("physics_body")));
 }
}


}
