#pragma once
#include "world/material.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nuc::world {
class Registry {
public:
    void load(const std::filesystem::path& materials_path,
              const std::filesystem::path& blocks_path);
    [[nodiscard]] const Material* material_by_id(std::uint32_t id) const noexcept;
    [[nodiscard]] const Material* material_by_name(std::string_view name) const noexcept;
    [[nodiscard]] const BlockDefinition* block_by_id(std::uint32_t id) const noexcept;
    [[nodiscard]] const BlockDefinition* block_by_name(std::string_view name) const noexcept;
    [[nodiscard]] const std::vector<Material>& materials() const noexcept { return materials_; }
    [[nodiscard]] const std::vector<BlockDefinition>& blocks() const noexcept { return blocks_; }
    [[nodiscard]] bool empty() const noexcept { return materials_.empty() && blocks_.empty(); }
private:
    std::vector<Material> materials_;
    std::vector<BlockDefinition> blocks_;
    std::unordered_map<std::uint32_t, std::size_t> material_ids_;
    std::unordered_map<std::string, std::size_t> material_names_;
    std::unordered_map<std::uint32_t, std::size_t> block_ids_;
    std::unordered_map<std::string, std::size_t> block_names_;
};
} // namespace nuc::world
