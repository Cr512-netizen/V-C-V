#pragma once
#include "render/chunk_mesh.h"
#include <cstddef>
#include <cstdint>

namespace nuc::render::gl {
// Owns one VAO/VBO/EBO. Call upload/draw/reset only while the GL context is current.
class MeshUpload final {
public:
    MeshUpload() = default;
    ~MeshUpload();
    MeshUpload(const MeshUpload&) = delete;
    MeshUpload& operator=(const MeshUpload&) = delete;
    MeshUpload(MeshUpload&& other) noexcept;
    MeshUpload& operator=(MeshUpload&& other) noexcept;
    bool upload(const ::nuc::render::ChunkMesh& mesh);
    void draw() const;
    void reset() noexcept;
    [[nodiscard]] bool ready() const noexcept { return vao_ != 0 && index_count_ != 0; }
    [[nodiscard]] std::size_t index_count() const noexcept { return index_count_; }
private:
    std::uint32_t vao_{}, vbo_{}, ebo_{};
    std::size_t index_count_{};
};
}
