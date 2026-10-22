#include "render/gl/gl_renderer.h"
#include "render/gl/gl_api.h"
#include <GLFW/glfw3.h>
#include <array>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace nuc {
namespace {
std::array<float,3> tint_for(std::uint32_t id) {
    switch(id) {
    case 1:return {0.48F,0.49F,0.51F};
    case 2:return {0.43F,0.27F,0.15F};
    case 3:return {0.25F,0.62F,0.20F};
    case 4:return {0.16F,0.40F,0.78F};
    default:{const float v=0.35F+static_cast<float>((id*37U)%45U)/100.0F;return {v,v*0.82F,v*0.68F};}
    }
}
constexpr GLenum vertex_shader=0x8B31, fragment_shader=0x8B30;
constexpr GLenum compile_status=0x8B81, link_status=0x8B82;
constexpr GLenum info_log_length=0x8B84;
constexpr GLenum debug_output=0x92E0, debug_output_sync=0x8242;
using DebugCallback=void (APIENTRY*)(GLenum,GLenum,GLuint,GLenum,GLsizei,const char*,const void*);
using DebugMessageCallback=void (APIENTRY*)(DebugCallback,const void*);
void APIENTRY debug_message(GLenum,GLenum,GLuint,GLenum,GLsizei length,const char* message,const void*) {
    std::cerr << "[OpenGL] " << std::string(message,message+length) << '\n';
}
GLuint compile(GLenum type,const char* source) {
    auto& g=render::gl::api();
    const GLuint shader=g.create_shader(type);
    g.shader_source(shader,1,&source,nullptr); g.compile_shader(shader);
    GLint ok=0; g.get_shader_iv(shader,compile_status,&ok);
    if (!ok) {
        GLint length=0; g.get_shader_iv(shader,info_log_length,&length);
        std::string log(static_cast<std::size_t>(length>1?length:1),'\0');
        g.get_shader_info_log(shader,static_cast<GLsizei>(log.size()),nullptr,log.data());
        std::cerr << "[GLSL] compile failed: " << log << '\n';
        g.delete_shader(shader); return 0;
    }
    return shader;
}
}
bool GlRenderer::init() {
    auto& g=render::gl::api();
    if (!g.load()) { std::cerr << "[OpenGL] Required 3.3 entry points unavailable\n"; return false; }
    if (auto callback=reinterpret_cast<DebugMessageCallback>(glfwGetProcAddress("glDebugMessageCallback"))) {
        glEnable(debug_output); glEnable(debug_output_sync); callback(debug_message,nullptr);
    }
    constexpr const char* vs=R"(#version 330 core
layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec3 a_color;
uniform mat4 u_view_projection;
uniform mat4 u_model;
out vec3 v_color;
out vec3 v_normal;
void main(){ gl_Position=u_view_projection*u_model*vec4(a_position,1.0); v_color=a_color; v_normal=mat3(u_model)*a_normal; })";
    constexpr const char* fs=R"(#version 330 core
in vec3 v_color; in vec3 v_normal; out vec4 out_color; uniform vec3 u_tint;
void main(){ float light=0.35+0.65*max(dot(normalize(v_normal),normalize(vec3(0.4,0.85,0.3))),0.0); out_color=vec4(v_color*u_tint*light,1.0); })";
    const GLuint v=compile(vertex_shader,vs), f=compile(fragment_shader,fs);
    if (!v || !f) { if(v)g.delete_shader(v); if(f)g.delete_shader(f); return false; }
    program_=g.create_program(); g.attach_shader(program_,v); g.attach_shader(program_,f); g.link_program(program_);
    g.delete_shader(v); g.delete_shader(f);
    GLint ok=0; g.get_program_iv(program_,link_status,&ok);
    if (!ok) {
        GLint length=0; g.get_program_iv(program_,info_log_length,&length);
        std::string log(static_cast<std::size_t>(length>1?length:1),'\0');
        g.get_program_info_log(program_,static_cast<GLsizei>(log.size()),nullptr,log.data());
        std::cerr << "[GLSL] link failed: " << log << '\n';
        g.delete_program(program_); program_=0; return false;
    }
    vp_location_=g.get_uniform_location(program_,"u_view_projection");
    model_location_=g.get_uniform_location(program_,"u_model");
    tint_location_=g.get_uniform_location(program_,"u_tint");
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDisable(GL_CULL_FACE);
    glClearColor(0.48F,0.70F,0.91F,1.0F);
    return vp_location_>=0 && model_location_>=0 && tint_location_>=0;
}
void GlRenderer::resize(int width,int height) {
    width_=width>0?width:1; height_=height>0?height:1; glViewport(0,0,width_,height_);
}
void GlRenderer::begin_frame() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    auto& g=render::gl::api();
    if (!program_ || (!chunks_&&!bodies_)) return;
    g.use_program(program_);
    g.uniform_matrix4fv(vp_location_,1,GL_FALSE,&view_projection_[0][0]);
    g.uniform3f(tint_location_,1.0F,1.0F,1.0F);
    if(chunks_) for (const auto& chunk:*chunks_) {
        if (!chunk.mesh || !chunk.mesh->ready()) continue;
        const glm::mat4 model=glm::translate(glm::mat4(1.0F),glm::vec3(chunk.origin_x,0.0F,chunk.origin_z));
        g.uniform_matrix4fv(model_location_,1,GL_FALSE,&model[0][0]);
        chunk.mesh->draw();
    }
    if(bodies_&&unit_cube_&&unit_cube_->ready()) for(const auto& body:*bodies_) {
        const glm::mat4 translated=glm::translate(glm::mat4(1.0F),body.position-body.half_extents);
        const glm::mat4 model=glm::scale(translated,body.half_extents*2.0F);
        const auto tint=tint_for(body.material);
        g.uniform3f(tint_location_,tint[0],tint[1],tint[2]);
        g.uniform_matrix4fv(model_location_,1,GL_FALSE,&model[0][0]);
        unit_cube_->draw();
    }
    g.use_program(0);
}
void GlRenderer::end_frame() {}
void GlRenderer::shutdown() {
    if (program_) render::gl::api().delete_program(program_);
    program_=0; chunks_=nullptr; bodies_=nullptr; unit_cube_=nullptr; vp_location_=model_location_=tint_location_=-1;
}
}
