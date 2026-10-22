#include "world/registry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace nuc::world {
namespace {
using Json = nlohmann::json;
Json read_json(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open registry file: " + path.string());
    Json value;
    try { file >> value; }
    catch (const Json::exception& e) {
        throw std::runtime_error("Invalid JSON in " + path.string() + ": " + e.what());
    }
    if (!value.is_array()) throw std::runtime_error("Registry root must be an array: " + path.string());
    return value;
}
std::uint32_t required_id(const Json& item, const std::string& where) {
    if (!item.contains("id") || (!item["id"].is_number_unsigned() && !item["id"].is_number_integer()))
        throw std::runtime_error(where + ": missing integer id");
    const auto raw = item["id"].get<std::int64_t>();
    if (raw < 0 || static_cast<std::uint64_t>(raw) > std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error(where + ": id is outside uint32 range");
    return static_cast<std::uint32_t>(raw);
}
std::string required_string(const Json& item, const char* key, const std::string& where) {
    if (!item.contains(key) || !item[key].is_string() || item[key].get<std::string>().empty())
        throw std::runtime_error(where + ": '" + key + "' must be a non-empty string");
    return item[key].get<std::string>();
}
std::uint32_t parse_flags(const Json& item, const std::string& where) {
    if (!item.contains("flags")) return MaterialSolid;
    if (!item["flags"].is_array()) throw std::runtime_error(where + ": 'flags' must be an array");
    std::uint32_t flags = 0;
    for (const auto& value : item["flags"]) {
        if (!value.is_string()) throw std::runtime_error(where + ": flag names must be strings");
        const auto name = value.get<std::string>();
        if (name == "solid") flags |= MaterialSolid;
        else if (name == "transparent") flags |= MaterialTransparent;
        else if (name == "emissive") flags |= MaterialEmissive;
        else throw std::runtime_error(where + ": unknown material flag '" + name + "'");
    }
    return flags;
}
}
std::uint32_t BlockDefinition::at(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 || x >= extent || y >= extent || z >= extent)
        throw std::out_of_range("Block voxel coordinates must be in [0, 7]");
    return voxels[static_cast<std::size_t>(x + extent * (y + extent * z))];
}
void Registry::load(const std::filesystem::path& materials_path,
                    const std::filesystem::path& blocks_path) {
    const Json material_json = read_json(materials_path);
    const Json block_json = read_json(blocks_path);
    std::vector<Material> materials;
    std::vector<BlockDefinition> blocks;
    std::unordered_map<std::uint32_t, std::size_t> material_ids, block_ids;
    std::unordered_map<std::string, std::size_t> material_names, block_names;

    for (std::size_t i = 0; i < material_json.size(); ++i) {
        const auto& item = material_json[i];
        const auto where = materials_path.string() + "[" + std::to_string(i) + "]";
        if (!item.is_object()) throw std::runtime_error(where + ": expected object");
        Material material;
        material.id = required_id(item, where);
        material.name = required_string(item, "name", where);
        if (!material_ids.emplace(material.id, materials.size()).second)
            throw std::runtime_error(where + ": duplicate material id " + std::to_string(material.id));
        if (!material_names.emplace(material.name, materials.size()).second)
            throw std::runtime_error(where + ": duplicate material name '" + material.name + "'");
        if (item.contains("color")) {
            const auto& color = item["color"];
            if (!color.is_array() || (color.size() != 3 && color.size() != 4))
                throw std::runtime_error(where + ": color must have 3 or 4 components");
            for (std::size_t c = 0; c < color.size(); ++c) {
                if (!color[c].is_number()) throw std::runtime_error(where + ": color components must be numeric");
                const float component = color[c].get<float>();
                if (component < 0.F || component > 1.F) throw std::runtime_error(where + ": color components must be in [0, 1]");
                material.color[c] = component;
            }
            if (color.size() == 3) material.color[3] = 1.F;
        }
        material.flags = parse_flags(item, where);
        if (item.contains("texture_path")) {
            if (!item["texture_path"].is_string()) throw std::runtime_error(where + ": texture_path must be a string");
            material.texture_path = item["texture_path"].get<std::string>();
        }
        materials.push_back(std::move(material));
    }

    for (std::size_t i = 0; i < block_json.size(); ++i) {
        const auto& item = block_json[i];
        const auto where = blocks_path.string() + "[" + std::to_string(i) + "]";
        if (!item.is_object()) throw std::runtime_error(where + ": expected object");
        BlockDefinition block;
        block.id = required_id(item, where);
        block.name = required_string(item, "name", where);
        if (!block_ids.emplace(block.id, blocks.size()).second)
            throw std::runtime_error(where + ": duplicate block id " + std::to_string(block.id));
        if (!block_names.emplace(block.name, blocks.size()).second)
            throw std::runtime_error(where + ": duplicate block name '" + block.name + "'");
        if (!item.contains("material_id")) throw std::runtime_error(where + ": missing material_id");
        const auto raw_material = item["material_id"].get<std::int64_t>();
        if (raw_material < 0 || static_cast<std::uint64_t>(raw_material) > std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error(where + ": invalid material_id");
        const auto material_id = static_cast<std::uint32_t>(raw_material);
        if (!material_ids.contains(material_id))
            throw std::runtime_error(where + ": references nonexistent material id " + std::to_string(material_id));
        block.voxels.fill(material_id);
        if (item.contains("voxels")) {
            const auto& voxels = item["voxels"];
            if (!voxels.is_array() || voxels.size() != BlockDefinition::voxel_count)
                throw std::runtime_error(where + ": voxels must contain exactly 512 material ids");
            for (std::size_t v = 0; v < voxels.size(); ++v) {
                if (!voxels[v].is_number_integer()) throw std::runtime_error(where + ": voxel material ids must be integers");
                const auto raw = voxels[v].get<std::int64_t>();
                if (raw < 0 || static_cast<std::uint64_t>(raw) > std::numeric_limits<std::uint32_t>::max() ||
                    !material_ids.contains(static_cast<std::uint32_t>(raw)))
                    throw std::runtime_error(where + ": voxel references nonexistent material id");
                block.voxels[v] = static_cast<std::uint32_t>(raw);
            }
        }
        blocks.push_back(std::move(block));
    }
    materials_ = std::move(materials); blocks_ = std::move(blocks);
    material_ids_ = std::move(material_ids); material_names_ = std::move(material_names);
    block_ids_ = std::move(block_ids); block_names_ = std::move(block_names);
}
const Material* Registry::material_by_id(std::uint32_t id) const noexcept { auto it=material_ids_.find(id); return it==material_ids_.end()?nullptr:&materials_[it->second]; }
const Material* Registry::material_by_name(std::string_view name) const noexcept { auto it=material_names_.find(std::string(name)); return it==material_names_.end()?nullptr:&materials_[it->second]; }
const BlockDefinition* Registry::block_by_id(std::uint32_t id) const noexcept { auto it=block_ids_.find(id); return it==block_ids_.end()?nullptr:&blocks_[it->second]; }
const BlockDefinition* Registry::block_by_name(std::string_view name) const noexcept { auto it=block_names_.find(std::string(name)); return it==block_names_.end()?nullptr:&blocks_[it->second]; }
} // namespace nuc::world
