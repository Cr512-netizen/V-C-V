#pragma once
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
namespace nuc::ecs {
class ISystem{public:virtual~ISystem()=default;virtual void tick(double dt)=0;};
class SystemRunner{public:template<class T,class...A>T&add(A&&...a){static_assert(std::is_base_of_v<ISystem,T>);auto p=std::make_unique<T>(std::forward<A>(a)...);auto&r=*p;systems_.push_back(std::move(p));return r;}void tick(double dt){if(dt<0)throw std::invalid_argument("negative system delta");for(auto&s:systems_)s->tick(dt);}std::size_t size()const noexcept{return systems_.size();}private:std::vector<std::unique_ptr<ISystem>>systems_;};
}