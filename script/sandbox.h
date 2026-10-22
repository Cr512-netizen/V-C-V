#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include <sol/sol.hpp>
namespace nuc::script {
class Sandbox {
public:
 explicit Sandbox(sol::state&,std::uint64_t instruction_limit=100000,std::chrono::milliseconds timeout=std::chrono::milliseconds(50));
 bool run_file(const std::string&,std::string* error=nullptr);
 bool call(sol::protected_function,const std::string& payload,std::string* error=nullptr);
 std::uint64_t instruction_limit() const noexcept { return instruction_limit_; }
private:
 static void instruction_hook(lua_State*,lua_Debug*);
 void begin(); void end() noexcept;
 sol::state& state_; std::uint64_t instruction_limit_; std::chrono::milliseconds timeout_;
 std::uint64_t instructions_{}; std::chrono::steady_clock::time_point deadline_{}; bool active_{};
};
}
