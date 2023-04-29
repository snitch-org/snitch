#ifndef SNITCH_MACROS_CHECK_HPP
#define SNITCH_MACROS_CHECK_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_macros_check_base.hpp"
#include "snitch/snitch_macros_warnings.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"

#define SNITCH_REQUIRE_IMPL(CHECK, EXPECTED, CRITICAL, ...)                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            SNITCH_EXPR(CHECK, EXPECTED, __VA_ARGS__);                                             \
            SNITCH_CURRENT_TEST.reg.report_assertion(                                              \
                CRITICAL, SNITCH_CURRENT_EXPRESSION.success, SNITCH_CURRENT_TEST,                  \
                {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);                                  \
        } else {                                                                                   \
            SNITCH_CURRENT_TEST.reg.report_assertion(                                              \
                CRITICAL, static_cast<bool>(__VA_ARGS__) == EXPECTED, SNITCH_CURRENT_TEST,         \
                {__FILE__, __LINE__}, CHECK "(" #__VA_ARGS__ ")");                                 \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

// clang-format off
#define SNITCH_REQUIRE(...)       SNITCH_REQUIRE_IMPL("REQUIRE",       true,  true,  __VA_ARGS__)
#define SNITCH_CHECK(...)         SNITCH_REQUIRE_IMPL("CHECK",         true,  false, __VA_ARGS__)
#define SNITCH_REQUIRE_FALSE(...) SNITCH_REQUIRE_IMPL("REQUIRE_FALSE", false, true,  __VA_ARGS__)
#define SNITCH_CHECK_FALSE(...)   SNITCH_REQUIRE_IMPL("CHECK_FALSE",   false, false, __VA_ARGS__)
// clang-format on

#define SNITCH_FAIL(MESSAGE)                                                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_assertion(                                                  \
            true, false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                    \
    } while (0)

#define SNITCH_FAIL_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_assertion(                                                  \
            false, false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                   \
    } while (0)

#define SNITCH_SKIP(MESSAGE)                                                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_skipped(                                                    \
            true, SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                           \
    } while (0)

#define SNITCH_SKIP_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_skipped(                                                    \
            false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                          \
    } while (0)

#define SNITCH_REQUIRE_THAT_IMPL(CHECK, CRITICAL, EXPR, ...)                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        auto  SNITCH_TEMP_RESULT  = snitch::impl::match(EXPR, __VA_ARGS__);                        \
        SNITCH_CURRENT_TEST.reg.report_assertion(                                                  \
            CRITICAL, SNITCH_TEMP_RESULT.first, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},         \
            CHECK "(" #EXPR ", " #__VA_ARGS__ "), got ", SNITCH_TEMP_RESULT.second);               \
    } while (0)

// clang-format off
#define SNITCH_REQUIRE_THAT(EXPR, ...) SNITCH_REQUIRE_THAT_IMPL("REQUIRE_THAT", true,  EXPR, __VA_ARGS__)
#define SNITCH_CHECK_THAT(EXPR, ...)   SNITCH_REQUIRE_THAT_IMPL("CHECK_THAT",   false, EXPR, __VA_ARGS__)
// clang-format on

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define FAIL(MESSAGE)       SNITCH_FAIL(MESSAGE)
#    define FAIL_CHECK(MESSAGE) SNITCH_FAIL_CHECK(MESSAGE)
#    define SKIP(MESSAGE)       SNITCH_SKIP(MESSAGE)

#    define REQUIRE(...)           SNITCH_REQUIRE(__VA_ARGS__)
#    define CHECK(...)             SNITCH_CHECK(__VA_ARGS__)
#    define REQUIRE_FALSE(...)     SNITCH_REQUIRE_FALSE(__VA_ARGS__)
#    define CHECK_FALSE(...)       SNITCH_CHECK_FALSE(__VA_ARGS__)
#    define REQUIRE_THAT(EXP, ...) SNITCH_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CHECK_THAT(EXP, ...)   SNITCH_CHECK_THAT(EXP, __VA_ARGS__)
#endif
// clang-format on

#endif
