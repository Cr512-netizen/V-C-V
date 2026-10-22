#pragma once
#include <GLFW/glfw3.h>
namespace nuc {struct InputSnapshot{bool escape=false,mouse_left=false,mouse_right=false;double mouse_x=0,mouse_y=0;};struct Input{static InputSnapshot capture(GLFWwindow*);};}
