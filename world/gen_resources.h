#pragma once
#include "world/chunk.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace nuc::world {
struct ResourceRule {std::uint32_t id{},source_material{};double chance{};std::uint32_t vein_size{1};int min_y{},max_y{31};};
struct BiomeResourceRule {std::string id;int region_chunks{4};std::vector<ResourceRule> resources;};
class ResourceGenerator {public:void load(const std::filesystem::path&);void apply(Chunk&,std::uint64_t seed) const;const std::vector<BiomeResourceRule>& biomes() const noexcept{return biomes_;}private:std::vector<BiomeResourceRule> biomes_;};
}
