#pragma once
#include "render/renderer.h"
#include "render/gl/mesh_upload.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace nuc {
struct BodyDraw { glm::vec3 position{}, half_extents{0.5F}; std::uint32_t material{1}; };
struct ChunkDraw {
    render::gl::MeshUpload* mesh{};
    float origin_x{}, origin_z{};
};
// OpenGL 3.3 core renderer; chunk meshes are non-owning and released by the app.
class GlRenderer final : public IRenderer {
public:
    ~GlRenderer() override { shutdown(); }
    bool init() override;
    void resize(int width, int height) override;
    void begin_frame() override;
    void end_frame() override;
    void shutdown() override;
    void set_world(const std::vector<ChunkDraw>* chunks) noexcept { chunks_=chunks; }
    void set_bodies(const std::vector<BodyDraw>* bodies,render::gl::MeshUpload* unit_cube) noexcept { bodies_=bodies; unit_cube_=unit_cube; }
    void set_view_projection(const glm::mat4& matrix) noexcept { view_projection_=matrix; }
private:
    const std::vector<ChunkDraw>* chunks_{};
    const std::vector<BodyDraw>* bodies_{};
    render::gl::MeshUpload* unit_cube_{};
    glm::mat4 view_projection_{1.0F};
    unsigned int program_{};
    int vp_location_{-1}, model_location_{-1}, tint_location_{-1};
    int width_{1280}, height_{720};
};
}
