#include "world/gen_resources.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace nuc::world {
namespace {
using Json = nlohmann::json;

constexpr std::uint64_t golden = 0x9e3779b97f4a7c15ULL;

std::uint64_t mix(std::uint64_t value) {
    value += golden;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

// Small deterministic stream: every vein draws from one of these.
class Rng {
public:
    explicit Rng(std::uint64_t seed) : state_(seed) {}
    [[nodiscard]] std::uint64_t next() {
        state_ += golden;
        return mix(state_);
    }
    [[nodiscard]] int range(int low, int high) {
        const auto span = static_cast<std::uint64_t>(high - low + 1);
        return low + static_cast<int>(next() % span);
    }
private:
    std::uint64_t state_;
};

int floor_div(int value, int divisor) {
    const int quotient = value / divisor;
    return value % divisor < 0 ? quotient - 1 : quotient;
}

std::uint32_t require_material(const Json& node, const char* key, const char* what) {
    if (!node.contains(key) || !node[key].is_number_unsigned())
        throw std::runtime_error(std::string("resource rule needs an unsigned '") + key + "' (" + what + ")");
    return node[key].get<std::uint32_t>();
}

ResourceRule parse_rule(const Json& node) {
    if (!node.is_object()) throw std::runtime_error("resource rule must be an object");
    ResourceRule rule;
    rule.id = require_material(node, "id", "ore material");
    rule.source_material = require_material(node, "source_material", "host material");
    if (!node.contains("chance") || !node["chance"].is_number())
        throw std::runtime_error("resource rule needs a numeric 'chance'");
    rule.chance = node["chance"].get<double>();
    rule.vein_size = require_material(node, "vein_size", "voxels per vein");
    if (!node.contains("min_y") || !node["min_y"].is_number_integer() || !node.contains("max_y") ||
        !node["max_y"].is_number_integer())
        throw std::runtime_error("resource rule needs integer 'min_y' and 'max_y'");
    rule.min_y = node["min_y"].get<int>();
    rule.max_y = node["max_y"].get<int>();
    const bool valid = rule.id != 0 && rule.source_material != 0 && std::isfinite(rule.chance) &&
                       rule.chance >= 0.0 && rule.chance <= 1.0 && rule.vein_size >= 1 && rule.min_y >= 0 &&
                       rule.max_y < static_cast<int>(Chunk::extent) && rule.min_y <= rule.max_y;
    if (!valid) throw std::runtime_error("resource rule values are out of range");
    return rule;
}

BiomeResourceRule parse_biome(const Json& node) {
    if (!node.is_object() || !node.contains("id") || !node["id"].is_string())
        throw std::runtime_error("biome rule needs a string 'id'");
    BiomeResourceRule biome;
    biome.id = node["id"].get<std::string>();
    if (biome.id.empty()) throw std::runtime_error("biome id must not be empty");
    if (node.contains("region_chunks") && node["region_chunks"].is_number_integer())
        biome.region_chunks = std::max(1, node["region_chunks"].get<int>());
    if (!node.contains("resources") || !node["resources"].is_array())
        throw std::runtime_error("biome rule needs a 'resources' array");
    for (const auto& entry : node["resources"]) biome.resources.push_back(parse_rule(entry));
    return biome;
}
}

void ResourceGenerator::load(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open biome resource rules: " + path.string());
    Json root;
    try {
        in >> root;
    } catch (const Json::exception& error) {
        throw std::runtime_error("invalid biome JSON: " + std::string(error.what()));
    }
    if (!root.is_array() || root.empty()) throw std::runtime_error("biomes root must be a non-empty array");
    std::vector<BiomeResourceRule> parsed;
    parsed.reserve(root.size());
    for (const auto& entry : root) parsed.push_back(parse_biome(entry));
    biomes_ = std::move(parsed);
}

const BiomeResourceRule* ResourceGenerator::rule_for(Biome biome) const noexcept {
    const std::string_view wanted = biome_name(biome);
    const auto match = std::find_if(biomes_.begin(), biomes_.end(),
                                    [wanted](const BiomeResourceRule& rule) { return rule.id == wanted; });
    return match == biomes_.end() ? nullptr : &*match;
}

void ResourceGenerator::apply(Chunk& chunk, std::uint64_t seed, Biome biome) const {
    const BiomeResourceRule* rules = rule_for(biome);
    if (rules == nullptr) return;
    const int extent = static_cast<int>(Chunk::extent);
    const int chunk_x = chunk.chunk_x();
    const int chunk_z = chunk.chunk_z();
    Rng rng(mix(seed) ^ mix(static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_x))) ^
            mix(static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_z))));

    for (const auto& rule : rules->resources) {
        const int attempts = std::clamp(static_cast<int>(std::lround(rule.chance * 64.0)), 1, 8);
        for (int attempt = 0; attempt < attempts; ++attempt) {
            const int y = rng.range(rule.min_y, rule.max_y);
            const int x = rng.range(0, extent - 1);
            const int z = rng.range(0, extent - 1);
            if (chunk.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y), static_cast<std::size_t>(z)) !=
                rule.source_material)
                continue;
            chunk.set(static_cast<std::size_t>(x), static_cast<std::size_t>(y), static_cast<std::size_t>(z), rule.id);
            int placed = 1;
            int cursor_x = x;
            int cursor_y = y;
            int cursor_z = z;
            while (placed < static_cast<int>(rule.vein_size)) {
                const int direction = rng.range(0, 5);
                const int step_x = direction == 0 ? 1 : (direction == 1 ? -1 : 0);
                const int step_y = direction == 2 ? 1 : (direction == 3 ? -1 : 0);
                const int step_z = direction == 4 ? 1 : (direction == 5 ? -1 : 0);
                const int next_x = cursor_x + step_x;
                const int next_y = cursor_y + step_y;
                const int next_z = cursor_z + step_z;
                const bool inside = next_x >= 0 && next_x < extent && next_z >= 0 && next_z < extent &&
                                    next_y >= rule.min_y && next_y <= rule.max_y;
                if (!inside) continue;
                if (chunk.at(static_cast<std::size_t>(next_x), static_cast<std::size_t>(next_y),
                             static_cast<std::size_t>(next_z)) != rule.source_material)
                    continue;
                chunk.set(static_cast<std::size_t>(next_x), static_cast<std::size_t>(next_y),
                          static_cast<std::size_t>(next_z), rule.id);
                cursor_x = next_x;
                cursor_y = next_y;
                cursor_z = next_z;
                ++placed;
            }
        }
    }
}

void ResourceGenerator::apply(Chunk& chunk, std::uint64_t seed) const {
    if (biomes_.empty()) return;
    const int region = biomes_.front().region_chunks;
    const auto rx = floor_div(chunk.chunk_x(), region);
    const auto rz = floor_div(chunk.chunk_z(), region);
    const auto index = static_cast<std::size_t>(
        mix(seed ^ mix(static_cast<std::uint32_t>(rx)) ^ mix(static_cast<std::uint32_t>(rz))) % biomes_.size());
    apply(chunk, seed, biome_from_name(biomes_[index].id));
}
}
