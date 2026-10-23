#pragma once
#include "render/texture_atlas.h"
#include <cstdint>

namespace nuc::render::gl {
// Owns a GL_TEXTURE_2D_ARRAY with one 32x32 layer per material. Move-only, and
// every call requires the GL context to be current.
class TextureArray final {
public:
    TextureArray() = default;
    ~TextureArray();
    TextureArray(TextureArray&& other) noexcept;
    TextureArray& operator=(TextureArray&& other) noexcept;
    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    bool upload(const ::nuc::render::TextureAtlas& atlas);
    void bind(std::uint32_t unit) const noexcept;
    void reset() noexcept;
    [[nodiscard]] bool ready() const noexcept { return texture_ != 0; }
    [[nodiscard]] int layers() const noexcept { return layers_; }

private:
    std::uint32_t texture_{};
    int layers_{};
};
} // namespace nuc::render::gl
