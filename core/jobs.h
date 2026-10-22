#pragma once
#include <TaskScheduler.h>
#include <functional>
namespace nuc {class Jobs{enki::TaskScheduler scheduler_;public:explicit Jobs(unsigned threads=0){scheduler_.Initialize(threads);}void parallel_for(std::size_t,const std::function<void(std::size_t)>&);};}
