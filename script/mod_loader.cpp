#include "script/mod_loader.h"
#include "core/log.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <lua.hpp>
#include <fstream>
#include <functional>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <unordered_map>
namespace nuc::script {
namespace {
using Json=nlohmann::json;
Json read_json(const std::filesystem::path& path) {
 std::ifstream in(path); if(!in) throw std::runtime_error("cannot open "+path.string());
 Json j; try { in>>j; } catch(const Json::exception& e) { throw std::runtime_error(path.string()+": "+e.what()); }
 if(!j.is_object()) throw std::runtime_error(path.string()+": expected JSON object"); return j;
}
std::string required_string(const Json& j,const char* key,const std::filesystem::path& path) {
 if(!j.contains(key)||!j[key].is_string()||j[key].get<std::string>().empty())
  throw std::runtime_error(path.string()+": missing non-empty '"+key+"'");
 return j[key].get<std::string>();
}
Json lua_to_json(lua_State* L,int index,int depth=0) {
 if(depth>32) throw std::runtime_error("mod.state nesting exceeds 32 levels");
 index=lua_absindex(L,index);
 switch(lua_type(L,index)) {
  case LUA_TNONE: case LUA_TNIL:return nullptr;
  case LUA_TBOOLEAN:return lua_toboolean(L,index)!=0;
  case LUA_TNUMBER:
   if(lua_isinteger(L,index)) return static_cast<std::int64_t>(lua_tointeger(L,index));
   else {
    const double number=static_cast<double>(lua_tonumber(L,index));
    if(!std::isfinite(number)) throw std::runtime_error("mod.state numbers must be finite");
    return number;
   }
  case LUA_TSTRING: {
   std::size_t length=0; const char* value=lua_tolstring(L,index,&length);
   return std::string(value,length);
  }
  case LUA_TTABLE: {
   Json result=Json::object();
   lua_pushnil(L);
   while(lua_next(L,index)!=0) {
    if(lua_type(L,-2)!=LUA_TSTRING) {
     lua_pop(L,2);
     throw std::runtime_error("mod.state table keys must be strings");
    }
    std::size_t length=0; const char* raw_key=lua_tolstring(L,-2,&length);
    const std::string key(raw_key,length);
    result[key]=lua_to_json(L,-1,depth+1);
    lua_pop(L,1);
   }
   return result;
  }
  default:throw std::runtime_error("mod.state supports only strings, numbers, booleans, nil, and tables");
 }
}
Json serialize_mod_state(sol::state& lua) {
 lua_State* L=lua.lua_state();
 const int top=lua_gettop(L);
 try {
  lua_getglobal(L,"mod");
  if(!lua_istable(L,-1)) throw std::runtime_error("Lua global 'mod' is not a table");
  lua_getfield(L,-1,"state");
  if(!lua_istable(L,-1)) throw std::runtime_error("mod.state is not a table");
  Json result=lua_to_json(L,-1);
  lua_settop(L,top);
  return result;
 } catch(...) {
  lua_settop(L,top);
  throw;
 }
}
void json_to_lua(sol::state& lua,sol::table& target,const Json& value,int depth=0) {
 if(depth>32) throw std::runtime_error("saved mod.state nesting exceeds 32 levels");
 if(!value.is_object()) return;
 for(auto it=value.begin();it!=value.end();++it) {
  if(it->is_object()) { sol::table child=lua.create_table(); json_to_lua(lua,child,*it,depth+1); target[it.key()]=child; }
  else if(it->is_string()) target[it.key()]=it->get<std::string>();
  else if(it->is_boolean()) target[it.key()]=it->get<bool>();
  else if(it->is_number_unsigned()) {
   const auto number=it->get<std::uint64_t>();
   if(number<=static_cast<std::uint64_t>(std::numeric_limits<lua_Integer>::max())) target[it.key()]=static_cast<lua_Integer>(number);
   else target[it.key()]=static_cast<lua_Number>(number);
  }
  else if(it->is_number_integer()) target[it.key()]=static_cast<lua_Integer>(it->get<std::int64_t>());
  else if(it->is_number_float()) target[it.key()]=it->get<double>();
  else if(it->is_null()) target[it.key()]=sol::nil;
 }
}
}
ModLoader::ModLoader(ecs::World& ecs,std::uint64_t limit,std::chrono::milliseconds timeout)
 :ecs_(ecs),instruction_limit_(limit),timeout_(timeout),shared_(std::make_shared<ApiSharedState>()) {}
ModLoader::~ModLoader() { if(shared_) shared_->listeners.clear(); instances_.clear(); }
bool ModLoader::loaded(const std::string& id) const noexcept {
 return std::any_of(loaded_info_.begin(),loaded_info_.end(),[&](const auto& m){return m.id==id;});
}
void ModLoader::remove_listeners(const std::string& owner) noexcept {
 for(auto it=shared_->listeners.begin();it!=shared_->listeners.end();) {
  auto& list=it->second;
  list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& l){return l.owner==owner;}),list.end());
  if(list.empty()) it=shared_->listeners.erase(it); else ++it;
 }
}
std::vector<ModInfo> ModLoader::discover(const std::filesystem::path& root) const {
 std::vector<ModInfo> mods;
 if(!std::filesystem::exists(root)) return mods;
 if(!std::filesystem::is_directory(root)) throw std::runtime_error("mods path is not a directory: "+root.string());
 for(const auto& entry:std::filesystem::directory_iterator(root)) {
  if(!entry.is_directory()) continue;
  const auto manifest=entry.path()/"mod.json"; if(!std::filesystem::exists(manifest)) continue;
  const auto j=read_json(manifest);
  ModInfo m; m.id=required_string(j,"id",manifest); m.version=required_string(j,"version",manifest);
  if(j.contains("api_version")) {
   if(!j["api_version"].is_string()) throw std::runtime_error(manifest.string()+": api_version must be a string");
   m.api_version=j["api_version"].get<std::string>();
  }
  if(m.api_version!="1") throw std::runtime_error("mod '"+m.id+"' requires unsupported API version "+m.api_version);
  if(j.contains("dependencies")) {
   if(!j["dependencies"].is_array()) throw std::runtime_error(manifest.string()+": dependencies must be an array");
   for(const auto& d:j["dependencies"]) {
    if(!d.is_string()||d.get<std::string>().empty()) throw std::runtime_error(manifest.string()+": dependency IDs must be strings");
    m.dependencies.push_back(d.get<std::string>());
   }
  }
  m.directory=entry.path(); mods.push_back(std::move(m));
 }
 std::sort(mods.begin(),mods.end(),[](const auto&a,const auto&b){return a.id<b.id;});
 std::set<std::string> ids;
 for(const auto& m:mods) if(!ids.insert(m.id).second) throw std::runtime_error("duplicate mod id: "+m.id);
 return mods;
}
std::vector<ModInfo> ModLoader::order(std::vector<ModInfo> mods) const {
 std::unordered_map<std::string,std::size_t> index;
 for(std::size_t i=0;i<mods.size();++i) index.emplace(mods[i].id,i);
 std::vector<int> state(mods.size()); std::vector<ModInfo> sorted;
 std::function<void(std::size_t)> visit=[&](std::size_t i) {
  if(state[i]==2) return;
  if(state[i]==1) throw std::runtime_error("cyclic mod dependency involving: "+mods[i].id);
  state[i]=1; auto deps=mods[i].dependencies; std::sort(deps.begin(),deps.end());
  for(const auto& dep:deps) {
   auto it=index.find(dep);
   if(it==index.end()) throw std::runtime_error("mod '"+mods[i].id+"' has missing dependency '"+dep+"'");
   visit(it->second);
  }
  state[i]=2; sorted.push_back(mods[i]);
 };
 for(std::size_t i=0;i<mods.size();++i) visit(i);
 return sorted;
}
bool ModLoader::load_one(const ModInfo& info,std::string& error) {
 auto instance=std::make_unique<Instance>(); instance->info=info;
 instance->lua=std::make_unique<sol::state>();
 instance->lua->open_libraries(sol::lib::base,sol::lib::math,sol::lib::table,sol::lib::string,sol::lib::utf8);
 (*instance->lua)["io"]=sol::nil; (*instance->lua)["os"]=sol::nil;
 (*instance->lua)["debug"]=sol::nil; (*instance->lua)["package"]=sol::nil;
 (*instance->lua)["dofile"]=sol::nil; (*instance->lua)["loadfile"]=sol::nil;
 instance->sandbox=std::make_unique<Sandbox>(*instance->lua,instruction_limit_,timeout_);
 instance->api=std::make_unique<ApiCore>(info.id,shared_,ecs_,instance->sandbox.get()); instance->api->bind(*instance->lua);
 sol::table mod_table=instance->lua->create_table(); sol::table state_table=instance->lua->create_table();
 auto saved=shared_->serialized_mod_state.find(info.id);
 if(saved!=shared_->serialized_mod_state.end()) json_to_lua(*instance->lua,state_table,Json::parse(saved->second));
 mod_table["state"]=state_table; (*instance->lua)["mod"]=mod_table;
 const auto entry=info.directory/"init.lua";
 if(!std::filesystem::exists(entry)) { error="missing entry point: "+entry.string(); return false; }
 if(!instance->sandbox->run_file(entry.string(),&error)) { if(error.empty()) error="failed to execute mod entry point: "+entry.string(); remove_listeners(info.id); return false; }
 for(const char* extra : {"events.lua","recipes.lua","energy.lua","npc_ai.lua","generator.lua"}) {
  const auto script=info.directory/extra;
  if(std::filesystem::exists(script)&&!instance->sandbox->run_file(script.string(),&error)) { if(error.empty()) error="failed to execute mod script: "+script.string(); remove_listeners(info.id); return false; }
 }
 instances_.push_back(std::move(*instance)); loaded_info_.push_back(info);
 log::info("Loaded Lua mod "+info.id+" "+info.version,"mods"); return true;
}
bool ModLoader::reload(const std::string& id,std::string& error) {
 auto it=std::find_if(instances_.begin(),instances_.end(),[&](const auto& i){return i.info.id==id;});
 if(it==instances_.end()) { error="mod is not loaded: "+id; return false; }
 try {
  shared_->serialized_mod_state[id]=serialize_mod_state(*it->lua).dump();
 } catch(const std::exception& e) { error=std::string("cannot preserve mod.state: ")+e.what(); return false; }
 const ModInfo info=it->info; const auto index=static_cast<std::size_t>(std::distance(instances_.begin(),it));
 remove_listeners(id); instances_.erase(instances_.begin()+static_cast<std::ptrdiff_t>(index));
 auto meta=std::find_if(loaded_info_.begin(),loaded_info_.end(),[&](const auto& m){return m.id==id;});
 if(meta!=loaded_info_.end()) loaded_info_.erase(meta);
 const bool reloaded=load_one(info,error);
 if(!reloaded&&error.empty()) error="failed to reload mod '"+id+"' without a diagnostic";
 return reloaded;
}
std::map<std::string,std::string> ModLoader::capture_mod_states() {
 for(auto& instance:instances_) {
  shared_->serialized_mod_state[instance.info.id]=serialize_mod_state(*instance.lua).dump();
 }
 return std::map<std::string,std::string>(shared_->serialized_mod_state.begin(),shared_->serialized_mod_state.end());
}
void ModLoader::restore_mod_states(const std::map<std::string,std::string>& states) {
 if(!instances_.empty()) throw std::logic_error("restore mod state before loading mods");
 shared_->serialized_mod_state.clear();
 shared_->serialized_mod_state.insert(states.begin(),states.end());
}
void ModLoader::emit_event(const std::string& name,const std::string& payload) {
 auto it=shared_->listeners.find(name); if(it==shared_->listeners.end()) return;
 for(auto& listener:it->second) {
  std::string error;
  if(!listener.sandbox||!listener.sandbox->call(listener.callback,payload,&error))
   log::error("event '"+name+"' listener failed: "+error,"mods");
 }
}
std::vector<std::string> ModLoader::discover_and_load(const std::filesystem::path& root) {
 auto mods=order(discover(root)); std::vector<std::string> errors;
 for(const auto& info:mods) {
  bool deps_ok=true; for(const auto& dep:info.dependencies) if(!loaded(dep)) deps_ok=false;
  if(!deps_ok) { errors.push_back(info.id+": dependency failed to load"); continue; }
  std::string error;
  if(!load_one(info,error)) { errors.push_back(info.id+": "+error); log::error(errors.back(),"mods"); }
 }
 return errors;
}
}
