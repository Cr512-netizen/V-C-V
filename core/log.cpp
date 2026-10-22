#include "core/log.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
namespace nuc::log { namespace {std::mutex m;std::ofstream out;const char* label(Level l){switch(l){case Level::debug:return "DEBUG";case Level::info:return "INFO";case Level::warn:return "WARN";case Level::error:return "ERROR";}return "?";}}
void write(Level l,std::string_view c,std::string_view s,const char* src,int line){std::lock_guard lock(m);if(!out.is_open()){std::filesystem::create_directories("logs");out.open("logs/app.log",std::ios::app);}auto t=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());std::tm tm{};
#ifdef _WIN32
localtime_s(&tm,&t);
#else
localtime_r(&t,&tm);
#endif
std::ostringstream os;os<<std::put_time(&tm,"%F %T")<<" ["<<label(l)<<"] ["<<c<<"] "<<s;if(src)os<<" ("<<src<<":"<<line<<")";std::cout<<os.str()<<'\n';if(out)out<<os.str()<<'\n';}}
