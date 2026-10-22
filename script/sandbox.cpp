#include "script/sandbox.h"
#include <lua.hpp>
#include <stdexcept>
namespace nuc::script {
Sandbox::Sandbox(sol::state& state,std::uint64_t limit,std::chrono::milliseconds timeout)
 :state_(state),instruction_limit_(limit),timeout_(timeout) {
 if(instruction_limit_==0||timeout_.count()<=0) throw std::invalid_argument("sandbox limits must be positive");
}
void Sandbox::begin() {
 if(active_) return;
 active_=true; instructions_=0; deadline_=std::chrono::steady_clock::now()+timeout_;
 auto* L=state_.lua_state(); *static_cast<Sandbox**>(lua_getextraspace(L))=this;
 lua_sethook(L,&Sandbox::instruction_hook,LUA_MASKCOUNT,1000);
}
void Sandbox::end() noexcept {
 if(!active_) return;
 auto* L=state_.lua_state(); lua_sethook(L,nullptr,0,0);
 *static_cast<Sandbox**>(lua_getextraspace(L))=nullptr; active_=false;
}
void Sandbox::instruction_hook(lua_State* L,lua_Debug*) {
 auto* self=*static_cast<Sandbox**>(lua_getextraspace(L)); if(!self) return;
 self->instructions_+=1000;
 if(self->instructions_>self->instruction_limit_) luaL_error(L,"mod exceeded Lua instruction budget");
 if(std::chrono::steady_clock::now()>self->deadline_) luaL_error(L,"mod hook timed out");
}
bool Sandbox::run_file(const std::string& path,std::string* error) {
 const bool nested=active_; if(!nested) begin();
 auto result=state_.safe_script_file(path,sol::script_pass_on_error);
 if(!nested) end();
 if(result.valid()) return true;
 sol::error e=result; if(error) *error=e.what(); return false;
}
bool Sandbox::call(sol::protected_function function,const std::string& payload,std::string* error) {
 const bool nested=active_; if(!nested) begin();
 auto result=function(payload);
 if(!nested) end();
 if(result.valid()) return true;
 sol::error e=result; if(error) *error=e.what(); return false;
}
}
