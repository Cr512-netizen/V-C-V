#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace nuc {
// Free-fly camera. Math accessors are display-independent; update polls GLFW input.
class Camera final {
public:
    explicit Camera(glm::vec3 position = {16.0F,28.0F,64.0F},
                    float aspect = 16.0F/9.0F);
    void update(GLFWwindow* window, double dt);
    void set_aspect(float aspect) noexcept;
    void set_rotation(float yaw, float pitch) noexcept;
    void move_local(const glm::vec3& direction, double dt) noexcept;
    [[nodiscard]] glm::vec3 forward() const noexcept;
    [[nodiscard]] glm::vec3 position() const noexcept { return position_; }
    [[nodiscard]] glm::mat4 view() const;
    [[nodiscard]] glm::mat4 projection() const;
    [[nodiscard]] glm::mat4 view_projection() const;
private:
    glm::vec3 position_;
    float yaw_{-90.0F}, pitch_{-18.0F}, aspect_{16.0F/9.0F};
    float fov_degrees_{70.0F}, near_{0.05F}, far_{1200.0F};
    bool cursor_initialized_{};
    double last_mouse_x_{}, last_mouse_y_{};
};
}
