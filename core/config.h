#pragma once
#include <string>
namespace nuc {
struct Config {
 int width = 1280, height = 720, tick_rate = 15;
 bool vsync = true;
 unsigned threads = 0;
 std::string renderer = "gl_mesh";
 float clear_r = .035F, clear_g = .045F, clear_b = .065F;
 static Config load(const std::string&);
};
}
