#ifndef SNITCH_CONCEPTS_HPP
#define SNITCH_CONCEPTS_HPP

#include "snitch/snitch_config.hpp"

#include <type_traits>

namespace snitch {
template<typename T>
concept signed_integral = std::is_signed_v<T>;

template<typename T>
concept unsigned_integral = std::is_unsigned_v<T>;

template<typename T>
concept floating_point = std::is_floating_point_v<T>;

template<typename T, typename U>
concept convertible_to = std::is_convertible_v<T, U>;

template<typename T>
concept enumeration = std::is_enum_v<T>;
} // namespace snitch

#endif
