#ifndef SNITCH_MACROS_WARNINGS_HPP
#define SNITCH_MACROS_WARNINGS_HPP

#include "snitch/snitch_config.hpp"

// clang-format off
#if defined(__clang__)
#    define SNITCH_WARNING_PUSH _Pragma("clang diagnostic push")
#    define SNITCH_WARNING_POP _Pragma("clang diagnostic pop")
#    define SNITCH_WARNING_DISABLE_PARENTHESES _Pragma("clang diagnostic ignored \"-Wparentheses\"")
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON
#elif defined(__GNUC__)
#    define SNITCH_WARNING_PUSH _Pragma("GCC diagnostic push")
#    define SNITCH_WARNING_POP _Pragma("GCC diagnostic pop")
#    define SNITCH_WARNING_DISABLE_PARENTHESES _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON
#elif defined(_MSC_VER)
#    define SNITCH_WARNING_PUSH _Pragma("warning(push)")
#    define SNITCH_WARNING_POP _Pragma("warning(pop)")
#    define SNITCH_WARNING_DISABLE_PARENTHESES
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON _Pragma("warning(disable: 4127)")
#else
#    define SNITCH_WARNING_PUSH
#    define SNITCH_WARNING_POP
#    define SNITCH_WARNING_DISABLE_PARENTHESES
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON
#endif
// clang-format on

#endif
