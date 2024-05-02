#ifndef SNITCH_CONCEPTS_HPP
#define SNITCH_CONCEPTS_HPP

#include "snitch/snitch_config.hpp"

#include <type_traits>

namespace snitch {

template<typename T>
concept integral = std::is_integral_v<T>;

template<typename T>
concept signed_integral = integral<T> && std::is_signed_v<T>;

template<typename T>
concept unsigned_integral = integral<T> && std::is_unsigned_v<T>;

template<typename T>
concept floating_point = std::is_floating_point_v<T>;

template<typename T, typename U>
concept convertible_to = std::is_convertible_v<T, U>;

template<typename T, typename U>
concept same_as = std::is_same_v<T, U>;

template<typename T>
concept enumeration = std::is_enum_v<T>;

namespace impl {
template<typename T>
using decay_object = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
struct is_function_pointer : std::false_type {};
template<typename T>
struct is_function_pointer<T*> : std::is_function<T> {};
} // namespace impl

template<typename T>
struct is_function_pointer : impl::is_function_pointer<std::remove_cv_t<T>> {};

template<typename T>
constexpr bool is_function_pointer_v = is_function_pointer<T>::value;

template<typename T>
concept function_pointer = is_function_pointer_v<impl::decay_object<T>>;

template<typename T>
concept member_function_pointer = std::is_member_function_pointer_v<impl::decay_object<T>>;

template<std::size_t N>
using char_array = char[N];

template<typename T>
struct is_raw_string : std::false_type {};
template<std::size_t N>
struct is_raw_string<char_array<N>> : std::true_type {};

template<typename T>
constexpr bool is_raw_string_v = is_raw_string<T>::value;

template<typename T>
concept raw_string = is_raw_string_v<impl::decay_object<T>>;

template<typename T>
concept pointer = std::is_pointer_v<impl::decay_object<T>>;
} // namespace snitch

#endif
