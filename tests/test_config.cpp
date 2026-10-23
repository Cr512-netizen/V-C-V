#include <catch2/catch_test_macros.hpp>
#include "core/config.h"
#include "test_scratch.h"
#include <fstream>
#include <filesystem>

TEST_CASE("config defaults simulation to 15 ticks per second") {
 nuc::Config defaults;
 REQUIRE(defaults.tick_rate == 15);
}

TEST_CASE("config parses values") {
 auto p = nuc::test::scratch_file("nuc_config_test.toml");
 {
  std::ofstream f(p);
  f << "[window]\nwidth=900\nheight=600\nvsync=false\n[runtime]\ntick_rate=30\n";
 }
 auto c = nuc::Config::load(p.string());
 REQUIRE(c.width == 900);
 REQUIRE(c.height == 600);
 REQUIRE_FALSE(c.vsync);
 REQUIRE(c.tick_rate == 30);
 std::filesystem::remove(p);
}

TEST_CASE("config selects the visible GL mesh renderer") {
 auto p = nuc::test::scratch_file("nuc_config_renderer_test.toml");
 { std::ofstream f(p); f << "[render]\nrenderer = \"gl_mesh\"\n"; }
 const auto c = nuc::Config::load(p.string());
 REQUIRE(c.renderer == "gl_mesh");
 std::filesystem::remove(p);
}
