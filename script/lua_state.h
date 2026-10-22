#pragma once
#include <sol/sol.hpp>
#include <string>
namespace nuc {class LuaState{sol::state lua_;public:LuaState();bool boot(const std::string&);sol::state& state(){return lua_;}};}
