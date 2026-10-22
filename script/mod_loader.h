#pragma once
#include "core/ecs/world.h"
#include "script/api_core.h"
#include "script/sandbox.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <sol/sol.hpp>
namespace nuc::script {
struct ModInfo { std::string id; std::string version; std::string api_version{"1"}; std::vector<std::string> dependencies; std::filesystem::path directory; };
class ModLoader {
public:
 explicit ModLoader(ecs::World&,std::uint64_t instruction_limit=100000,std::chrono::milliseconds timeout=std::chrono::milliseconds(50));
 ~ModLoader(); ModLoader(const ModLoader&)=delete; ModLoader& operator=(const ModLoader&)=delete;
 std::vector<std::string> discover_and_load(const std::filesystem::path&);
 bool reload(const std::string& mod_id,std::string& error);
 void emit_event(const std::string& name,const std::string& payload);
 std::map<std::string,std::string> capture_mod_states();
 void restore_mod_states(const std::map<std::string,std::string>& states);
 const std::vector<ModInfo>& loaded_mods() const noexcept { return loaded_info_; }
 bool loaded(const std::string&) const noexcept;
 std::shared_ptr<ApiSharedState> shared_state() const noexcept { return shared_; }
private:
 struct Instance { ModInfo info; std::unique_ptr<sol::state> lua; std::unique_ptr<Sandbox> sandbox; std::unique_ptr<ApiCore> api; };
 std::vector<ModInfo> discover(const std::filesystem::path&) const;
 std::vector<ModInfo> order(std::vector<ModInfo>) const;
 bool load_one(const ModInfo&,std::string&); void remove_listeners(const std::string&) noexcept;
 ecs::World& ecs_; std::uint64_t instruction_limit_; std::chrono::milliseconds timeout_;
 std::shared_ptr<ApiSharedState> shared_; std::vector<Instance> instances_; std::vector<ModInfo> loaded_info_;
};
}
