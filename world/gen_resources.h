#pragma once
#include "world/biome.h"
#include "world/chunk.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nuc::world {
struct ResourceRule {
    std::uint32_t id{};               // ore material
    std::uint32_t source_material{};  // material the vein replaces
    double chance{};                  // density knob, read as attempts per chunk
    std::uint32_t vein_size{1};       // voxels per vein, including the seed voxel
    int min_y{};
    int max_y{31};
};

struct BiomeResourceRule {
    std::string id;                   // matches biome_name()
    int region_chunks{4};             // used only by the legacy region-based apply()
    std::vector<ResourceRule> resources;
};

// Turns a shaped chunk into an ore-bearing one. Veins are chunk-local random
// walks, so they never straddle a border and never need neighbour data.
class ResourceGenerator final {
public:
    void load(const std::filesystem::path& path);
    void apply(Chunk& chunk, std::uint64_t seed, Biome biome) const;
    void apply(Chunk& chunk, std::uint64_t seed) const;  // legacy: biome from a region hash
    [[nodiscard]] const std::vector<BiomeResourceRule>& biomes() const noexcept { return biomes_; }
    [[nodiscard]] const BiomeResourceRule* rule_for(Biome biome) const noexcept;
private:
    std::vector<BiomeResourceRule> biomes_;
};
}
