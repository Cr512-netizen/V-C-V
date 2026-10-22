#include <catch2/catch_test_macros.hpp>
#include "platform/camera.h"
#include "render/chunk_mesh.h"
#include "render/gl/gl_renderer.h"
#include "render/gl/mesh_upload.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cstdlib>

TEST_CASE("gl_mesh renders a pixel different from the clear color") {
    if (!glfwInit()) SKIP("GLFW initialization unavailable on this runner");
    struct GlfwGuard { ~GlfwGuard(){ glfwTerminate(); } } glfw_guard;
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GLFW_TRUE);
    GLFWwindow* window=glfwCreateWindow(64,64,"renderer-smoke",nullptr,nullptr);
    if (!window) SKIP("Hidden OpenGL 3.3 context unavailable; pixel smoke test skipped");
    glfwMakeContextCurrent(window);
    nuc::render::gl::MeshUpload gpu_mesh,unit_cube_gpu;
    nuc::GlRenderer renderer;
    if (!renderer.init()) {
        renderer.shutdown();
        glfwDestroyWindow(window);
        SKIP("OpenGL 3.3 shader renderer unavailable; inspect GL/GLSL diagnostics");
    }
    nuc::world::Chunk chunk;
    chunk.set(0,0,0,3);
    const auto cpu=nuc::render::build_chunk_mesh(chunk);
    REQUIRE(gpu_mesh.upload(cpu));
    REQUIRE(gpu_mesh.index_count()==cpu.indices.size());
    REQUIRE(gpu_mesh.ready());
    const std::vector<nuc::ChunkDraw> draws{{&gpu_mesh,0.0F,0.0F}};
    renderer.set_world(&draws);
    nuc::world::Chunk unit_cube_chunk;
    unit_cube_chunk.set(0,0,0,1);
    auto unit_cube_cpu=nuc::render::build_chunk_mesh(unit_cube_chunk);
    for(auto& vertex:unit_cube_cpu.vertices)vertex.material=0;
    REQUIRE(unit_cube_gpu.upload(unit_cube_cpu));
    const std::vector<nuc::BodyDraw> body_draws{{{1.2F,0.5F,0.0F},{0.2F,0.2F,0.2F},4}};
    renderer.set_bodies(&body_draws,&unit_cube_gpu);
    nuc::Camera camera({0.5F,0.5F,4.0F},1.0F);
    camera.set_rotation(-90.0F,0.0F);
    renderer.set_view_projection(camera.view_projection());
    renderer.resize(64,64);
    renderer.begin_frame();
    glFinish();
    std::array<unsigned char,4> pixel{},body_pixel{};
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
    glReadPixels(42,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,body_pixel.data());
    const bool colored=pixel[1]>pixel[2]+15 && pixel[1]>pixel[0]+5;
    const bool debris_visible=body_pixel[0]<80 && body_pixel[2]>body_pixel[1]+15;
    renderer.shutdown();
    gpu_mesh.reset();
    unit_cube_gpu.reset();
    glfwDestroyWindow(window);
    REQUIRE(colored);
    REQUIRE(debris_visible);
}

TEST_CASE("empty GPU mesh upload and reset are safe without a GL context") {
    nuc::render::gl::MeshUpload mesh;
    const nuc::render::ChunkMesh empty;
    REQUIRE(mesh.upload(empty));
    REQUIRE_FALSE(mesh.ready());
    REQUIRE(mesh.index_count()==0);
    mesh.reset();
    mesh.reset();
    REQUIRE_FALSE(mesh.ready());
}
