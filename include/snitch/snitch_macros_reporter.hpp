#ifndef SNITCH_MACROS_REPORTER_HPP
#define SNITCH_MACROS_REPORTER_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_macros_utility.hpp"
#include "snitch/snitch_registry.hpp"

#define SNITCH_REGISTER_REPORTER_CALLBACKS(NAME, ...)                                              \
    static const std::string_view SNITCH_MACRO_CONCAT(reporter_id_, __COUNTER__)                   \
        [[maybe_unused]] = snitch::tests.add_reporter(NAME, __VA_ARGS__)

#define SNITCH_REGISTER_REPORTER_IMPL(NAME, TYPE, COUNTER)                                         \
    static std::optional<TYPE> SNITCH_MACRO_CONCAT(reporter_, COUNTER);                            \
    static void SNITCH_MACRO_CONCAT(reporter_init_, COUNTER)(snitch::registry & r) noexcept {      \
        SNITCH_MACRO_CONCAT(reporter_, COUNTER).emplace(r);                                        \
    }                                                                                              \
    template<typename T>                                                                           \
    static bool SNITCH_MACRO_CONCAT(reporter_config_, COUNTER)(                                    \
        snitch::registry & r, std::string_view k, std::string_view v) noexcept {                   \
        return SNITCH_MACRO_CONCAT(reporter_, COUNTER)->configure(r, k, v);                        \
    }                                                                                              \
    static void SNITCH_MACRO_CONCAT(reporter_report_, COUNTER)(                                    \
        const snitch::registry& r, const snitch::event::data& e) noexcept {                        \
        SNITCH_MACRO_CONCAT(reporter_, COUNTER)->report(r, e);                                     \
    }                                                                                              \
    static void SNITCH_MACRO_CONCAT(reporter_finish_, COUNTER)(snitch::registry&) noexcept {       \
        SNITCH_MACRO_CONCAT(reporter_, COUNTER).reset();                                           \
    }                                                                                              \
    static const std::string_view SNITCH_MACRO_CONCAT(reporter_id_, COUNTER) [[maybe_unused]] =    \
        snitch::tests.add_reporter(                                                                \
            NAME, &SNITCH_MACRO_CONCAT(reporter_init_, COUNTER),                                   \
            &SNITCH_MACRO_CONCAT(reporter_config_, COUNTER) < TYPE >,                              \
            &SNITCH_MACRO_CONCAT(reporter_report_, COUNTER),                                       \
            &SNITCH_MACRO_CONCAT(reporter_finish_, COUNTER))

#define SNITCH_REGISTER_REPORTER(NAME, TYPE) SNITCH_REGISTER_REPORTER_IMPL(NAME, TYPE, __COUNTER__)

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define REGISTER_REPORTER_CALLBACKS(...) SNITCH_REGISTER_REPORTER_CALLBACKS(__VA_ARGS__)
#    define REGISTER_REPORTER(...)           SNITCH_REGISTER_REPORTER(__VA_ARGS__)
#endif
// clang-format on

#endif
