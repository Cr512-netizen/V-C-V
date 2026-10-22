#pragma once
#include "core/ecs/world.h"
#include <tuple>
namespace nuc::ecs {
template<class...C>class View{static_assert(sizeof...(C)>0);public:explicit View(World&w):world_(w){}template<class F>void each(F&&f){using First=std::tuple_element_t<0,std::tuple<C...>>;auto*p=world_.template try_pool<First>();if(!p)return;const auto indices=p->dense_indices();for(auto i:indices){auto e=world_.entity_at(i);if(e==null_entity||!(world_.template has<C>(e)&&...))continue;f(e,world_.template get<C>(e)...);}}private:World&world_;};
template<class...C>View<C...>view(World&w){return View<C...>(w);}
}