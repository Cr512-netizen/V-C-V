#include "script/lua_state.h"
#include "core/log.h"
namespace nuc {
LuaState::LuaState() {
 lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
 lua_["log"] = lua_.create_table();
 lua_["log"]["info"] = [](const std::string& s) { log::info(s, "lua"); };
 lua_["log"]["warn"] = [](const std::string& s) { log::warn(s, "lua"); };
 lua_["log"]["error"] = [](const std::string& s) { log::error(s, "lua"); };
}
bool LuaState::boot(const std::string& p) {
 auto r = lua_.safe_script_file(p, sol::script_pass_on_error);
 if (!r.valid()) {
  sol::error e = r;
  log::error(e.what(), "lua");
  return false;
 }
 return true;
}
}
