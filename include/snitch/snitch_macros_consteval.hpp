#ifndef SNITCH_MACROS_CONSTEVAL_HPP
#define SNITCH_MACROS_CONSTEVAL_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_macros_check_base.hpp"
#include "snitch/snitch_macros_warnings.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"

#define SNITCH_CONSTEVAL_REQUIRE_IMPL(CHECK, EXPECTED, MAYBE_ABORT, ...)                           \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            constexpr SNITCH_EXPR(CHECK, EXPECTED, __VA_ARGS__);                                   \
            SNITCH_CURRENT_TEST.reg.report_assertion(                                              \
                SNITCH_CURRENT_EXPRESSION.success, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},      \
                SNITCH_CURRENT_EXPRESSION);                                                        \
            MAYBE_ABORT;                                                                           \
        } else {                                                                                   \
            constexpr bool SNITCH_TEMP_RESULT = static_cast<bool>(__VA_ARGS__);                    \
            SNITCH_CURRENT_TEST.reg.report_assertion(                                              \
                SNITCH_TEMP_RESULT == EXPECTED, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},         \
                CHECK "(" #__VA_ARGS__ ")");                                                       \
            MAYBE_ABORT;                                                                           \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

// clang-format off
#define SNITCH_CONSTEVAL_REQUIRE(...)       SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_REQUIRE",       true,  SNITCH_TESTING_ABORT,  __VA_ARGS__)
#define SNITCH_CONSTEVAL_CHECK(...)         SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_CHECK",         true,  (void)0,               __VA_ARGS__)
#define SNITCH_CONSTEVAL_REQUIRE_FALSE(...) SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_REQUIRE_FALSE", false, SNITCH_TESTING_ABORT,  __VA_ARGS__)
#define SNITCH_CONSTEVAL_CHECK_FALSE(...)   SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_CHECK_FALSE",   false, (void)0,               __VA_ARGS__)
// clang-format on

#define SNITCH_CONSTEVAL_REQUIRE_THAT_IMPL(CHECK, MAYBE_ABORT, EXPR, ...)                          \
    do {                                                                                           \
        auto&          SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                     \
        constexpr auto SNITCH_TEMP_RESULT  = snitch::impl::match(EXPR, __VA_ARGS__);               \
        SNITCH_CURRENT_TEST.reg.report_assertion(                                                  \
            SNITCH_TEMP_RESULT.first, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                   \
            CHECK "(" #EXPR ", " #__VA_ARGS__ "), got ", SNITCH_TEMP_RESULT.second);               \
        MAYBE_ABORT;                                                                               \
    } while (0)

// clang-format off
#define SNITCH_CONSTEVAL_REQUIRE_THAT(EXPR, ...) SNITCH_CONSTEVAL_REQUIRE_THAT_IMPL("CONSTEVAL_REQUIRE_THAT", SNITCH_TESTING_ABORT,  EXPR, __VA_ARGS__)
#define SNITCH_CONSTEVAL_CHECK_THAT(EXPR, ...)   SNITCH_CONSTEVAL_REQUIRE_THAT_IMPL("CONSTEVAL_CHECK_THAT",   (void)0,               EXPR, __VA_ARGS__)
// clang-format on

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define CONSTEVAL_REQUIRE(...)           SNITCH_CONSTEVAL_REQUIRE(__VA_ARGS__)
#    define CONSTEVAL_CHECK(...)             SNITCH_CONSTEVAL_CHECK(__VA_ARGS__)
#    define CONSTEVAL_REQUIRE_FALSE(...)     SNITCH_CONSTEVAL_REQUIRE_FALSE(__VA_ARGS__)
#    define CONSTEVAL_CHECK_FALSE(...)       SNITCH_CONSTEVAL_CHECK_FALSE(__VA_ARGS__)
#    define CONSTEVAL_REQUIRE_THAT(EXP, ...) SNITCH_CONSTEVAL_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CONSTEVAL_CHECK_THAT(EXP, ...)   SNITCH_CONSTEVAL_CHECK_THAT(EXP, __VA_ARGS__)
#endif
// clang-format on

#endif
