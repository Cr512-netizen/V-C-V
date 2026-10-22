#pragma once
#include "world/chunk_manager.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>
#include <utility>

namespace nuc {
// Small ray-driven edit controller. It contains no UI or platform polling.
enum class EditAction { break_block, place_block };
class EditMode final {
public:
    explicit EditMode(world::ChunkManager& world, std::uint32_t selected_material=3)
        : world_(world), selected_material_(selected_material?selected_material:3) {}
    void set_selected_material(std::uint32_t material) noexcept {
        if(material) selected_material_=material;
    }
    [[nodiscard]] std::uint32_t selected_material() const noexcept { return selected_material_; }
    using BreakCallback = std::function<void(world::VoxelCoord,std::uint32_t)>;
    void set_break_callback(BreakCallback callback) { on_break_ = std::move(callback); }
    bool apply(EditAction action, glm::vec3 origin, glm::vec3 direction, float reach);
private:
    world::ChunkManager& world_;
    std::uint32_t selected_material_;
    BreakCallback on_break_;
};
}
