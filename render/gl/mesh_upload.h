#pragma once
#include "render/chunk_mesh.h"
#include "render/texture_atlas.h"
#include <cstddef>
#include <cstdint>

namespace nuc::render::gl {
// Owns one VAO/VBO/EBO. Call upload/draw/reset only while the GL context is current.
// The atlas pointer is optional: without it every vertex uses layer 0, which the
// renderer only does when texture upload failed and flat colours are in use.
class MeshUpload final {
public:
    MeshUpload() = default;
    ~MeshUpload();
    MeshUpload(const MeshUpload&) = delete;
    MeshUpload& operator=(const MeshUpload&) = delete;
    MeshUpload(MeshUpload&& other) noexcept;
    MeshUpload& operator=(MeshUpload&& other) noexcept;
    void set_atlas(const ::nuc::render::TextureAtlas* atlas) noexcept { atlas_ = atlas; }
    bool upload(const ::nuc::render::ChunkMesh& mesh);
    void draw() const;
    void reset() noexcept;
    [[nodiscard]] bool ready() const noexcept { return vao_ != 0 && index_count_ != 0; }
    [[nodiscard]] std::size_t index_count() const noexcept { return index_count_; }
private:
    const ::nuc::render::TextureAtlas* atlas_{};
    std::uint32_t vao_{}, vbo_{}, ebo_{};
    std::size_t index_count_{};
};
} // namespace nuc::render::gl
