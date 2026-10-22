#include "platform/camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace nuc {
namespace {
constexpr float radians = 0.017453292519943295F;
}
Camera::Camera(glm::vec3 position, float aspect) : position_(position) { set_aspect(aspect); }
void Camera::set_aspect(float aspect) noexcept {
    if (std::isfinite(aspect) && aspect > 0.0F) aspect_=aspect;
}
void Camera::set_rotation(float yaw, float pitch) noexcept {
    if (std::isfinite(yaw)) yaw_=yaw;
    if (std::isfinite(pitch)) pitch_=std::clamp(pitch,-89.0F,89.0F);
}
glm::vec3 Camera::forward() const noexcept {
    const float y=pitch_*radians, x=yaw_*radians;
    return glm::normalize(glm::vec3(std::cos(x)*std::cos(y),std::sin(y),std::sin(x)*std::cos(y)));
}
glm::mat4 Camera::view() const { return glm::lookAt(position_,position_+forward(),glm::vec3(0,1,0)); }
glm::mat4 Camera::projection() const {
    return glm::perspective(fov_degrees_*radians,aspect_,near_,far_);
}
glm::mat4 Camera::view_projection() const { return projection()*view(); }
void Camera::move_local(const glm::vec3& direction, double dt) noexcept {
    if (!std::isfinite(dt) || dt<=0.0 || !std::isfinite(direction.x) ||
        !std::isfinite(direction.y) || !std::isfinite(direction.z)) return;
    const float step=static_cast<float>(std::min(dt,0.05))*18.0F;
    const auto f=forward();
    const auto horizontal=glm::normalize(glm::vec3(f.x,0.0F,f.z));
    const auto right=glm::normalize(glm::cross(horizontal,glm::vec3(0,1,0)));
    const glm::vec3 world_direction=right*direction.x+glm::vec3(0,1,0)*direction.y+horizontal*direction.z;
    if (glm::dot(world_direction,world_direction)>0.0F)
        position_+=glm::normalize(world_direction)*step;
}
void Camera::update(GLFWwindow* window, double dt) {
    if (!window) return;
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    double mx=0,my=0;
    glfwGetCursorPos(window,&mx,&my);
    if (!cursor_initialized_) { last_mouse_x_=mx; last_mouse_y_=my; cursor_initialized_=true; }
    else {
        const float dx=static_cast<float>(mx-last_mouse_x_);
        const float dy=static_cast<float>(last_mouse_y_-my);
        last_mouse_x_=mx; last_mouse_y_=my;
        set_rotation(yaw_+dx*0.10F,pitch_+dy*0.10F);
    }
    glm::vec3 move(0.0F);
    if (glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS) move.z+=1.0F;
    if (glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS) move.z-=1.0F;
    if (glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS) move.x+=1.0F;
    if (glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS) move.x-=1.0F;
    if (glfwGetKey(window,GLFW_KEY_SPACE)==GLFW_PRESS) move.y+=1.0F;
    if (glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) move.y-=1.0F;
    move_local(move,dt);
}
}
