#include <catch2/catch_test_macros.hpp>
#include "script/mod_loader.h"
#include "script/hot_reload.h"
#include "sim/machine.h"
#include "sim/recipes.h"
#include "sim/energy.h"
#include "core/ecs/view.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
namespace {
struct TempMods {
 std::filesystem::path root=std::filesystem::temp_directory_path()/"nuc_mod_loader_tests";
 TempMods(){std::filesystem::remove_all(root);std::filesystem::create_directories(root);}
 ~TempMods(){std::error_code ec;std::filesystem::remove_all(root,ec);}
 void mod(const std::string& folder,const std::string& manifest,const std::string& script) {
  auto dir=root/folder;std::filesystem::create_directories(dir);
  std::ofstream(dir/"mod.json")<<manifest; std::ofstream(dir/"init.lua")<<script;
 }
};
}
TEST_CASE("mods load in dependency order and share safe voxel API") {
 TempMods tmp;
 tmp.mod("z-child",R"({"id":"child","version":"1","api_version":"1","dependencies":["base"]})",
  R"(assert(registry.get("test.value") == "ready"); assert(world.get_voxel(4,5,6)==7))");
 tmp.mod("a-base",R"({"id":"base","version":"1","api_version":"1","dependencies":[]})",
  R"(registry.register("test.value","ready"); world.set_voxel(4,5,6,7))");
 nuc::ecs::World ecs; nuc::script::ModLoader loader(ecs);
 auto errors=loader.discover_and_load(tmp.root);
 REQUIRE(errors.empty()); REQUIRE(loader.loaded_mods().size()==2);
 REQUIRE(loader.loaded_mods()[0].id=="base"); REQUIRE(loader.loaded_mods()[1].id=="child");
 REQUIRE(loader.shared_state()->voxels.at(std::tuple{4,5,6})==7);
}
TEST_CASE("Lua infinite loops are interrupted by sandbox budget") {
 TempMods tmp; tmp.mod("loop",R"({"id":"loop","version":"1","api_version":"1"})","while true do end");
 nuc::ecs::World ecs; nuc::script::ModLoader loader(ecs,5000,std::chrono::milliseconds(100));
 auto errors=loader.discover_and_load(tmp.root);
 REQUIRE(errors.size()==1); REQUIRE(errors[0].find("instruction budget")!=std::string::npos);
}
TEST_CASE("mod loader rejects dependency cycles and missing dependencies") {
 TempMods tmp;
 tmp.mod("a",R"({"id":"a","version":"1","dependencies":["b"]})","return");
 tmp.mod("b",R"({"id":"b","version":"1","dependencies":["a"]})","return");
 nuc::ecs::World ecs; nuc::script::ModLoader loader(ecs);
 REQUIRE_THROWS(loader.discover_and_load(tmp.root));
}

TEST_CASE("hot reload preserves mod.state") {
 TempMods tmp;
 tmp.mod("state",R"({"id":"state","version":"1","api_version":"1"})",
  R"(mod.state.count = (mod.state.count or 0) + 1; world.set_voxel(9,0,0,mod.state.count))");
 nuc::ecs::World ecs; nuc::script::ModLoader loader(ecs);
 REQUIRE(loader.discover_and_load(tmp.root).empty());
 REQUIRE(loader.shared_state()->voxels.at(std::tuple{9,0,0})==1);
 nuc::script::HotReload reload; std::string error;
 const bool reloaded=reload.reload(loader,"state",&error);
 INFO("hot reload error: " << error);
 REQUIRE(reloaded);
 REQUIRE(error.empty());
 REQUIRE(loader.shared_state()->voxels.at(std::tuple{9,0,0})==2);
}

TEST_CASE("Lua machine API creates and feeds a recipe-driven machine") {
 TempMods tmp;
 tmp.mod("factory",R"({"id":"factory","version":"1","api_version":"1"})",
  R"(local m = machine.create("crush")
    assert(m ~= 0)
    assert(machine.insert_input(m, 1, 4) == 4)
    mod.state.machine = m)");
 nuc::ecs::World ecs;
 nuc::sim::RecipeBook recipes;
 recipes.add({"crush",{{1,2}},{{2,1}},1});
 nuc::script::ModLoader loader(ecs);
 loader.shared_state()->recipes=&recipes;
 REQUIRE(loader.discover_and_load(tmp.root).empty());
 REQUIRE(ecs.entity_count()==1);
 nuc::sim::MachineSystem::tick(ecs,recipes,0);
 nuc::sim::MachineSystem::tick(ecs,recipes,0);
 std::uint32_t input_count=99,output_count=99;
 nuc::ecs::view<nuc::sim::MachineInput,nuc::sim::MachineOutput>(ecs).each(
  [&](nuc::ecs::Entity,nuc::sim::MachineInput& input,nuc::sim::MachineOutput& output) {
   input_count=input.inventory.count(1); output_count=output.inventory.count(2);
  });
 REQUIRE(input_count==0);
 REQUIRE(output_count==2);
}

TEST_CASE("Lua energy API registers nodes into the live simulation network") {
 TempMods tmp;
 tmp.mod("power",R"({"id":"power","version":"1","api_version":"1"})",
  R"(assert(energy.add_producer(1, 10))
    assert(energy.add_consumer(2, 7, 5))
    assert(energy.add_storage(3, 20, 0))
    assert(not energy.add_producer(1, 99)))");
 nuc::ecs::World ecs;
 nuc::sim::EnergyNetwork network;
 nuc::script::ModLoader loader(ecs);
 loader.shared_state()->energy_network=&network;
 REQUIRE(loader.discover_and_load(tmp.root).empty());
 network.tick();
 REQUIRE(network.find(2)->allocated==7);
 REQUIRE(network.find(3)->stored==3);
}
