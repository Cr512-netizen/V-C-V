#include "render/gl/gl_api.h"

namespace nuc::render::gl {
namespace {
template<class T> T proc(const char* name) noexcept {
    return reinterpret_cast<T>(glfwGetProcAddress(name));
}
Api global_api;
}
Api& api() noexcept { return global_api; }

bool Api::load() noexcept {
    gen_vertex_arrays = proc<GenObjects>("glGenVertexArrays");
    bind_vertex_array = proc<BindVertexArray>("glBindVertexArray");
    delete_vertex_arrays = proc<DeleteObjects>("glDeleteVertexArrays");
    gen_buffers = proc<GenObjects>("glGenBuffers");
    bind_buffer = proc<BindObject>("glBindBuffer");
    delete_buffers = proc<DeleteObjects>("glDeleteBuffers");
    buffer_data = proc<BufferData>("glBufferData");
    vertex_attrib_pointer = proc<VertexAttribPointer>("glVertexAttribPointer");
    enable_vertex_attrib_array = proc<EnableVertexAttribArray>("glEnableVertexAttribArray");
    create_shader = proc<CreateShader>("glCreateShader");
    shader_source = proc<ShaderSource>("glShaderSource");
    compile_shader = proc<CompileShader>("glCompileShader");
    get_shader_iv = proc<GetObjectiv>("glGetShaderiv");
    get_shader_info_log = proc<GetInfoLog>("glGetShaderInfoLog");
    delete_shader = proc<DeleteObject>("glDeleteShader");
    create_program = proc<CreateProgram>("glCreateProgram");
    attach_shader = proc<AttachShader>("glAttachShader");
    link_program = proc<LinkProgram>("glLinkProgram");
    get_program_iv = proc<GetObjectiv>("glGetProgramiv");
    get_program_info_log = proc<GetInfoLog>("glGetProgramInfoLog");
    delete_program = proc<DeleteObject>("glDeleteProgram");
    use_program = proc<UseProgram>("glUseProgram");
    get_uniform_location = proc<GetUniformLocation>("glGetUniformLocation");
    uniform_matrix4fv = proc<UniformMatrix4fv>("glUniformMatrix4fv");
    uniform3f = proc<Uniform3f>("glUniform3f");
    uniform1i = proc<Uniform1i>("glUniform1i");
    active_texture = proc<ActiveTexture>("glActiveTexture");
    gen_textures = proc<GenObjects>("glGenTextures");
    delete_textures = proc<DeleteObjects>("glDeleteTextures");
    bind_texture = proc<BindObject>("glBindTexture");
    tex_image_3d = proc<TexImage3D>("glTexImage3D");
    tex_parameteri = proc<TexParameteri>("glTexParameteri");
    pixel_storei = proc<PixelStorei>("glPixelStorei");
    generate_mipmap = proc<GenerateMipmap>("glGenerateMipmap");
    return gen_vertex_arrays && bind_vertex_array && delete_vertex_arrays &&
        gen_buffers && bind_buffer && delete_buffers && buffer_data &&
        vertex_attrib_pointer && enable_vertex_attrib_array && create_shader &&
        shader_source && compile_shader && get_shader_iv && get_shader_info_log &&
        delete_shader && create_program && attach_shader && link_program &&
        get_program_iv && get_program_info_log && delete_program && use_program &&
        get_uniform_location && uniform_matrix4fv && uniform3f;
}
}
