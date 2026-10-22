#include "render/null_renderer.h"
#include <GLFW/glfw3.h>
namespace nuc {bool NullRenderer::init(){return true;}void NullRenderer::resize(int w,int h){glViewport(0,0,w,h);}void NullRenderer::begin_frame(){glClearColor(.035F,.045F,.065F,1.F);glClear(GL_COLOR_BUFFER_BIT);}}
