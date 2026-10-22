#include "app/loop.h"
#include <algorithm>
#include <chrono>
namespace nuc {unsigned Loop::consume_ticks(double elapsed,double& a,double step){a+=std::clamp(elapsed,0.0,.25);unsigned n=0;while(a>=step&&n<8){a-=step;++n;}if(n==8&&a>=step)a=0;return n;}void Loop::run(const Tick& tick,const Render& render,const std::function<bool()>& keep,int hz){using clock=std::chrono::steady_clock;double step=1.0/std::max(1,hz),acc=0;auto prev=clock::now();while(keep()){auto now=clock::now();double dt=std::chrono::duration<double>(now-prev).count();prev=now;acc+=std::clamp(dt,0.0,.25);unsigned n=0;while(acc>=step&&n<8){tick(step);acc-=step;++n;}if(n==8&&acc>=step)acc=0;render(acc/step);}}}
