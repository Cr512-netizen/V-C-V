#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
namespace nuc::sim {
class StateHash {
public:
 static constexpr std::uint64_t offset_basis=14695981039346656037ULL;
 static constexpr std::uint64_t prime=1099511628211ULL;
 explicit StateHash(std::uint64_t seed=offset_basis)noexcept:value_(seed){}
 void bytes(std::span<const std::byte> data)noexcept;
 void text(std::string_view value)noexcept;
 template<class T> requires(std::is_integral_v<T>||std::is_enum_v<T>) void integer(T value)noexcept {
  using Raw=typename raw_type<T>::type;using U=std::make_unsigned_t<Raw>;U v=static_cast<U>(value);
  for(std::size_t i=0;i<sizeof(U);++i){const auto b=static_cast<std::uint8_t>((v>>(i*8U))&U{0xff});mix(b);}
 }
 std::uint64_t value()const noexcept{return value_;}
private:
 template<class T,bool E=std::is_enum_v<T>>struct raw_type_impl{using type=T;};
 template<class T>struct raw_type_impl<T,true>{using type=std::underlying_type_t<T>;};
 template<class T>struct raw_type:raw_type_impl<T>{};
 void mix(std::uint8_t b)noexcept{value_^=b;value_*=prime;}
 std::uint64_t value_;
};
}