#include "render/gl/texture_upload.h"
#include "render/gl/gl_api.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <utility>

namespace nuc::render::gl {
namespace {
constexpr GLenum texture_2d_array = 0x8C1A;
constexpr GLenum min_filter = 0x2801;
constexpr GLenum mag_filter = 0x2800;
constexpr GLenum wrap_s = 0x2802;
constexpr GLenum wrap_t = 0x2803;
constexpr GLenum nearest = 0x2600;
constexpr GLenum nearest_mipmap_nearest = 0x2700;
constexpr GLenum repeat = 0x2901;
constexpr GLenum rgba8 = 0x8058;
constexpr GLenum unpack_alignment = 0x0CF5;
constexpr GLenum texture0 = 0x84C0;
} // namespace

TextureArray::~TextureArray() { reset(); }

TextureArray::TextureArray(TextureArray&& other) noexcept
    : texture_(std::exchange(other.texture_, 0)), layers_(std::exchange(other.layers_, 0)) {}

TextureArray& TextureArray::operator=(TextureArray&& other) noexcept {
    if (this != &other) {
        reset();
        texture_ = std::exchange(other.texture_, 0);
        layers_ = std::exchange(other.layers_, 0);
    }
    return *this;
}

bool TextureArray::upload(const ::nuc::render::TextureAtlas& atlas) {
    auto& g = api();
    if (atlas.layers <= 0 || atlas.rgba.size() != static_cast<std::size_t>(atlas.layers) * tile_rgba_bytes) {
        std::cerr << "[OpenGL] texture atlas is empty or malformed\n";
        return false;
    }
    if (!g.textures_available()) {
        std::cerr << "[OpenGL] texture array entry points unavailable, keeping flat colours\n";
        return false;
    }
    GLuint handle = 0;
    g.gen_textures(1, &handle);
    if (handle == 0) return false;

    g.active_texture(texture0);
    g.bind_texture(texture_2d_array, handle);
    g.pixel_storei(unpack_alignment, 1);
    g.tex_parameteri(texture_2d_array, min_filter, nearest_mipmap_nearest);
    g.tex_parameteri(texture_2d_array, mag_filter, nearest);
    g.tex_parameteri(texture_2d_array, wrap_s, repeat);
    g.tex_parameteri(texture_2d_array, wrap_t, repeat);
    g.tex_image_3d(texture_2d_array, 0, rgba8, tile_extent, tile_extent, atlas.layers, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, atlas.rgba.data());
    g.generate_mipmap(texture_2d_array);
    g.bind_texture(texture_2d_array, 0);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "[OpenGL] texture array upload error: " << error << '\n';
        g.delete_textures(1, &handle);
        return false;
    }
    reset();
    texture_ = handle;
    layers_ = atlas.layers;
    return true;
}

void TextureArray::bind(std::uint32_t unit) const noexcept {
    if (!ready()) return;
    auto& g = api();
    g.active_texture(texture0 + unit);
    g.bind_texture(texture_2d_array, texture_);
}

void TextureArray::reset() noexcept {
    if (texture_ != 0 && api().delete_textures) api().delete_textures(1, &texture_);
    texture_ = 0;
    layers_ = 0;
}
} // namespace nuc::render::gl
