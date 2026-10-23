#pragma once
#include <GLFW/glfw3.h>
#include <cstddef>

namespace nuc::render::gl {
// Minimal OpenGL 3.3 entry-point table. Functions are resolved through GLFW so
// Windows does not depend on direct exports of post-1.1 OpenGL symbols.
struct Api {
    using GenObjects = void (APIENTRY*)(GLsizei, GLuint*);
    using BindObject = void (APIENTRY*)(GLenum, GLuint);
    using BindVertexArray = void (APIENTRY*)(GLuint);
    using DeleteObjects = void (APIENTRY*)(GLsizei, const GLuint*);
    using BufferData = void (APIENTRY*)(GLenum, std::ptrdiff_t, const void*, GLenum);
    using VertexAttribPointer = void (APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
    using EnableVertexAttribArray = void (APIENTRY*)(GLuint);
    using CreateShader = GLuint (APIENTRY*)(GLenum);
    using ShaderSource = void (APIENTRY*)(GLuint, GLsizei, const char* const*, const GLint*);
    using CompileShader = void (APIENTRY*)(GLuint);
    using GetObjectiv = void (APIENTRY*)(GLuint, GLenum, GLint*);
    using GetInfoLog = void (APIENTRY*)(GLuint, GLsizei, GLsizei*, char*);
    using DeleteObject = void (APIENTRY*)(GLuint);
    using CreateProgram = GLuint (APIENTRY*)();
    using AttachShader = void (APIENTRY*)(GLuint, GLuint);
    using LinkProgram = void (APIENTRY*)(GLuint);
    using UseProgram = void (APIENTRY*)(GLuint);
    using GetUniformLocation = GLint (APIENTRY*)(GLuint, const char*);
    using UniformMatrix4fv = void (APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat*);
    using Uniform3f = void (APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
    using Uniform1i = void (APIENTRY*)(GLint, GLint);
    using ActiveTexture = void (APIENTRY*)(GLenum);
    using TexImage3D = void (APIENTRY*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum,
                                        const void*);
    using TexParameteri = void (APIENTRY*)(GLenum, GLenum, GLint);
    using PixelStorei = void (APIENTRY*)(GLenum, GLint);
    using GenerateMipmap = void (APIENTRY*)(GLenum);
    GenObjects gen_vertex_arrays{};
    BindVertexArray bind_vertex_array{};
    DeleteObjects delete_vertex_arrays{};
    GenObjects gen_buffers{};
    BindObject bind_buffer{};
    DeleteObjects delete_buffers{};
    BufferData buffer_data{};
    VertexAttribPointer vertex_attrib_pointer{};
    EnableVertexAttribArray enable_vertex_attrib_array{};
    CreateShader create_shader{};
    ShaderSource shader_source{};
    CompileShader compile_shader{};
    GetObjectiv get_shader_iv{};
    GetInfoLog get_shader_info_log{};
    DeleteObject delete_shader{};
    CreateProgram create_program{};
    AttachShader attach_shader{};
    LinkProgram link_program{};
    GetObjectiv get_program_iv{};
    GetInfoLog get_program_info_log{};
    DeleteObject delete_program{};
    UseProgram use_program{};
    GetUniformLocation get_uniform_location{};
    UniformMatrix4fv uniform_matrix4fv{};
    Uniform3f uniform3f{};
    Uniform1i uniform1i{};
    ActiveTexture active_texture{};
    GenObjects gen_textures{};
    DeleteObjects delete_textures{};
    BindObject bind_texture{};
    TexImage3D tex_image_3d{};
    TexParameteri tex_parameteri{};
    PixelStorei pixel_storei{};
    GenerateMipmap generate_mipmap{};
    bool load() noexcept;
    // Texture entry points are optional: a driver that lacks them leaves the
    // renderer on the flat-colour fallback instead of failing initialization.
    [[nodiscard]] bool textures_available() const noexcept {
        return gen_textures && delete_textures && bind_texture && tex_image_3d && tex_parameteri &&
               pixel_storei && generate_mipmap && active_texture && uniform1i;
    }
};
Api& api() noexcept;
} // namespace nuc::render::gl
