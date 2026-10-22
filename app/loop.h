#pragma once
#include <functional>
namespace nuc {class Loop{public:using Tick=std::function<void(double)>;using Render=std::function<void(double)>;static unsigned consume_ticks(double,double&,double);static void run(const Tick&,const Render&,const std::function<bool()>&,int=15);};}
