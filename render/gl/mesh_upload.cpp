#include "render/gl/mesh_upload.h"
#include "render/gl/gl_api.h"
#include <array>
#include <iostream>
#include <utility>
#include <vector>

namespace nuc::render::gl {
namespace {
constexpr GLenum array_buffer = 0x8892;
constexpr GLenum element_buffer = 0x8893;
constexpr GLenum static_draw = 0x88E4;
constexpr GLenum float_type = 0x1406;
struct GpuVertex { float position[3]; float normal[3]; float uv[2]; float layer; float color[3]; };
std::array<float,3> color(std::uint32_t id) {
    if(id==0)return {1.0F,1.0F,1.0F};
    switch (id) {
    case 1: return {0.48F,0.49F,0.51F};
    case 2: return {0.43F,0.27F,0.15F};
    case 3: return {0.25F,0.62F,0.20F};
    case 4: return {0.16F,0.40F,0.78F};
    default: { const float v=0.35F+static_cast<float>((id*37U)%45U)/100.0F; return {v,v*0.82F,v*0.68F}; }
    }
}
}
MeshUpload::~MeshUpload() { reset(); }
MeshUpload::MeshUpload(MeshUpload&& o) noexcept
    : vao_(std::exchange(o.vao_,0)), vbo_(std::exchange(o.vbo_,0)),
      ebo_(std::exchange(o.ebo_,0)), index_count_(std::exchange(o.index_count_,0)) {}
MeshUpload& MeshUpload::operator=(MeshUpload&& o) noexcept {
    if (this != &o) { reset(); vao_=std::exchange(o.vao_,0); vbo_=std::exchange(o.vbo_,0);
        ebo_=std::exchange(o.ebo_,0); index_count_=std::exchange(o.index_count_,0); }
    return *this;
}
bool MeshUpload::upload(const ::nuc::render::ChunkMesh& mesh) {
    if (mesh.vertices.empty() || mesh.indices.empty()) { reset(); return true; }
    auto& g=api();
    if (!g.load()) return false;
    std::vector<GpuVertex> vertices;
    vertices.reserve(mesh.vertices.size());
    for (const auto& v:mesh.vertices) {
        const auto c=color(v.material);
        const int layer=atlas_?atlas_->layer(v.material):-1;
        vertices.push_back({{v.x,v.y,v.z},{v.nx,v.ny,v.nz},{v.u,v.v},
            static_cast<float>(layer<0?0:layer),{c[0],c[1],c[2]}});
    }
    GLuint new_vao=0,new_vbo=0,new_ebo=0;
    while(glGetError()!=GL_NO_ERROR) {}
    g.gen_vertex_arrays(1,&new_vao);
    g.gen_buffers(1,&new_vbo);
    g.gen_buffers(1,&new_ebo);
    if (!new_vao || !new_vbo || !new_ebo) {
        if(new_vao) g.delete_vertex_arrays(1,&new_vao);
        if(new_vbo) g.delete_buffers(1,&new_vbo);
        if(new_ebo) g.delete_buffers(1,&new_ebo);
        return false;
    }
    g.bind_vertex_array(new_vao);
    g.bind_buffer(array_buffer,new_vbo);
    g.buffer_data(array_buffer,static_cast<std::ptrdiff_t>(vertices.size()*sizeof(GpuVertex)),vertices.data(),static_draw);
    g.bind_buffer(element_buffer,new_ebo);
    g.buffer_data(element_buffer,static_cast<std::ptrdiff_t>(mesh.indices.size()*sizeof(std::uint32_t)),mesh.indices.data(),static_draw);
    g.vertex_attrib_pointer(0,3,float_type,GL_FALSE,sizeof(GpuVertex),nullptr);
    g.enable_vertex_attrib_array(0);
    g.vertex_attrib_pointer(1,3,float_type,GL_FALSE,sizeof(GpuVertex),reinterpret_cast<void*>(sizeof(float)*3));
    g.enable_vertex_attrib_array(1);
    g.vertex_attrib_pointer(2,2,float_type,GL_FALSE,sizeof(GpuVertex),reinterpret_cast<void*>(sizeof(float)*6));
    g.enable_vertex_attrib_array(2);
    g.vertex_attrib_pointer(3,1,float_type,GL_FALSE,sizeof(GpuVertex),reinterpret_cast<void*>(sizeof(float)*8));
    g.enable_vertex_attrib_array(3);
    g.vertex_attrib_pointer(4,3,float_type,GL_FALSE,sizeof(GpuVertex),reinterpret_cast<void*>(sizeof(float)*9));
    g.enable_vertex_attrib_array(4);
    g.bind_vertex_array(0);
    const GLenum error=glGetError();
    if (error!=GL_NO_ERROR) {
        std::cerr << "[OpenGL] mesh upload error: " << error << '\\n';
        g.delete_vertex_arrays(1,&new_vao);
        g.delete_buffers(1,&new_vbo);
        g.delete_buffers(1,&new_ebo);
        return false;
    }
    reset();
    vao_=new_vao; vbo_=new_vbo; ebo_=new_ebo;
    index_count_=mesh.indices.size();
    return true;
}
void MeshUpload::draw() const {
    if (!ready()) return;
    api().bind_vertex_array(vao_);
    glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(index_count_),GL_UNSIGNED_INT,nullptr);
    api().bind_vertex_array(0);
}
void MeshUpload::reset() noexcept {
    if (vao_ || vbo_ || ebo_) {
        auto& g=api();
        if (g.delete_vertex_arrays && vao_) g.delete_vertex_arrays(1,&vao_);
        if (g.delete_buffers && vbo_) g.delete_buffers(1,&vbo_);
        if (g.delete_buffers && ebo_) g.delete_buffers(1,&ebo_);
    }
    vao_=vbo_=ebo_=0; index_count_=0;
}
}
