#ifndef SNITCH_MACROS_UTILITY_HPP
#define SNITCH_MACROS_UTILITY_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_macros_warnings.hpp"

#define SNITCH_CONCAT_IMPL(x, y) x##y
#define SNITCH_MACRO_CONCAT(x, y) SNITCH_CONCAT_IMPL(x, y)

#define SNITCH_CURRENT_LOCATION                                                                    \
    snitch::source_location {                                                                      \
        std::string_view{__FILE__}, static_cast<std::size_t>(__LINE__)                             \
    }

#define SNITCH_DISCARD_ARGS(...)                                                                   \
    do {                                                                                           \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        static_cast<void>(sizeof(__VA_ARGS__, 0));                                                 \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_VOID_STATEMENT static_cast<void>(0)

#endif
