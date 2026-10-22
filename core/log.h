#pragma once
#include <string_view>
namespace nuc::log { enum class Level{debug,info,warn,error}; void write(Level,std::string_view,std::string_view,const char* =nullptr,int=0); inline void info(std::string_view s,std::string_view c="app"){write(Level::info,c,s);} inline void warn(std::string_view s,std::string_view c="app"){write(Level::warn,c,s);} inline void error(std::string_view s,std::string_view c="app"){write(Level::error,c,s);} }
