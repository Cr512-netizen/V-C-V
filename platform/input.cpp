#include "platform/input.h"
namespace nuc {InputSnapshot Input::capture(GLFWwindow* w){InputSnapshot s;s.escape=glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS;s.mouse_left=glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;s.mouse_right=glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;glfwGetCursorPos(w,&s.mouse_x,&s.mouse_y);return s;}}
