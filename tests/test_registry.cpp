#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "world/registry.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace {
struct Fixture {
    std::filesystem::path root = std::filesystem::temp_directory_path() / "nuc_registry_tests";
    Fixture() { std::filesystem::create_directories(root); }
    std::filesystem::path write(const std::string& name, const std::string& text) {
        auto p = root / name; std::ofstream out(p); out << text; return p;
    }
    ~Fixture() { std::error_code ec; std::filesystem::remove_all(root, ec); }
};
}
TEST_CASE("registry loads shipped material and block data") {
    nuc::world::Registry registry;
    registry.load(std::filesystem::path(NUC_SOURCE_DIR) / "data/materials.json",
                  std::filesystem::path(NUC_SOURCE_DIR) / "data/blocks.json");
    REQUIRE(registry.materials().size() == 7);
    REQUIRE(registry.blocks().size() == 7);
    REQUIRE(registry.material_by_id(1)->name == "stone");
    REQUIRE(registry.material_by_name("water")->id == 4);
    REQUIRE(registry.material_by_name("copper_ore")->id == 10);
    REQUIRE(registry.material_by_name("iron_ore")->id == 11);
    REQUIRE(registry.block_by_name("grass")->at(7, 7, 7) == 3);
}
TEST_CASE("registry rejects duplicate material ids") {
    Fixture f;
    auto m = f.write("materials.json", R"([{"id":1,"name":"stone"},{"id":1,"name":"soil"}])");
    auto b = f.write("blocks.json", "[]");
    nuc::world::Registry registry;
    REQUIRE_THROWS_WITH(registry.load(m, b), Catch::Matchers::ContainsSubstring("duplicate material id"));
}
TEST_CASE("registry rejects blocks with missing material references") {
    Fixture f;
    auto m = f.write("materials.json", R"([{"id":1,"name":"stone"}])");
    auto b = f.write("blocks.json", R"([{"id":1,"name":"void","material_id":99}])");
    nuc::world::Registry registry;
    REQUIRE_THROWS_WITH(registry.load(m, b), Catch::Matchers::ContainsSubstring("nonexistent material id"));
}
