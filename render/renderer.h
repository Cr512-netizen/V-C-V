#pragma once
namespace nuc {class IRenderer{public:virtual ~IRenderer()=default;virtual bool init()=0;virtual void resize(int,int)=0;virtual void begin_frame()=0;virtual void end_frame()=0;virtual void shutdown()=0;};}
