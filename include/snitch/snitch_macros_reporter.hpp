#ifndef SNITCH_MACROS_REPORTER_HPP
#define SNITCH_MACROS_REPORTER_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_macros_utility.hpp"
#include "snitch/snitch_registry.hpp"

#if SNITCH_ENABLE
#    define SNITCH_REGISTER_REPORTER_CALLBACKS(NAME, ...)                                          \
        static const std::string_view SNITCH_MACRO_CONCAT(reporter_id_, __COUNTER__)               \
            [[maybe_unused]] = snitch::tests.add_reporter(NAME, __VA_ARGS__)

#    define SNITCH_REGISTER_REPORTER(NAME, TYPE)                                                   \
        static const std::string_view SNITCH_MACRO_CONCAT(reporter_id_, __COUNTER__)               \
            [[maybe_unused]] = snitch::tests.add_reporter<TYPE>(NAME)
#else // SNITCH_ENABLE
#    define SNITCH_REGISTER_REPORTER_CALLBACKS(NAME, ...) /* nothing */
#    define SNITCH_REGISTER_REPORTER(NAME, TYPE) static_assert(NAME)
#endif // SNITCH_ENABLE

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define REGISTER_REPORTER_CALLBACKS(...) SNITCH_REGISTER_REPORTER_CALLBACKS(__VA_ARGS__)
#    define REGISTER_REPORTER(...)           SNITCH_REGISTER_REPORTER(__VA_ARGS__)
#endif
// clang-format on

#endif
