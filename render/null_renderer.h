#pragma once
#include "render/renderer.h"
namespace nuc {class NullRenderer final:public IRenderer{unsigned long long frames_=0;public:bool init()override;void resize(int,int)override;void begin_frame()override;void end_frame()override{++frames_;}void shutdown()override{}unsigned long long frames()const{return frames_;}};}
