#ifndef SNITCH_TYPE_ID_HPP
#define SNITCH_TYPE_ID_HPP

#include "snitch/snitch_config.hpp"

namespace snitch {
using type_id_t = const void*;
}

namespace snitch::impl {
template<typename T>
struct type_id {
    constexpr static char value = 0;
};
} // namespace snitch::impl

namespace snitch {
template<typename T>
type_id_t type_id() noexcept {
    return &impl::type_id<T>::value;
}

template<>
constexpr type_id_t type_id<void>() noexcept {
    return nullptr;
}
} // namespace snitch

#endif
