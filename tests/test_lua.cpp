#include <catch2/catch_test_macros.hpp>
#include "script/lua_state.h"
TEST_CASE("lua_can_return_number"){nuc::LuaState lua;auto r=lua.state().safe_script("function answer() return 42 end");REQUIRE(r.valid());REQUIRE(lua.state()["answer"]().get<int>()==42);}
