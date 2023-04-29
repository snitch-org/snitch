#ifndef SNITCH_MACROS_CONSTEVAL_HPP
#define SNITCH_MACROS_CONSTEVAL_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_macros_check_base.hpp"
#include "snitch/snitch_macros_warnings.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"

#define SNITCH_CONSTEVAL_REQUIRE_IMPL(CHECK, EXPECTED, CRITICAL, ...)                              \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            constexpr SNITCH_EXPR(CHECK, EXPECTED, __VA_ARGS__);                                   \
            SNITCH_CURRENT_TEST.reg.report_assertion(                                              \
                CRITICAL, SNITCH_CURRENT_EXPRESSION.success, SNITCH_CURRENT_TEST,                  \
                {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);                                  \
        } else {                                                                                   \
            constexpr bool SNITCH_TEMP_RESULT = static_cast<bool>(__VA_ARGS__);                    \
            SNITCH_CURRENT_TEST.reg.report_assertion(                                              \
                CRITICAL, SNITCH_TEMP_RESULT == EXPECTED, SNITCH_CURRENT_TEST,                     \
                {__FILE__, __LINE__}, CHECK "(" #__VA_ARGS__ ")");                                 \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

// clang-format off
#define SNITCH_CONSTEVAL_REQUIRE(...)       SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_REQUIRE",       true,  true,  __VA_ARGS__)
#define SNITCH_CONSTEVAL_CHECK(...)         SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_CHECK",         true,  false, __VA_ARGS__)
#define SNITCH_CONSTEVAL_REQUIRE_FALSE(...) SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_REQUIRE_FALSE", false, true,  __VA_ARGS__)
#define SNITCH_CONSTEVAL_CHECK_FALSE(...)   SNITCH_CONSTEVAL_REQUIRE_IMPL("CONSTEVAL_CHECK_FALSE",   false, false, __VA_ARGS__)
// clang-format on

#define SNITCH_CONSTEVAL_REQUIRE_THAT_IMPL(CHECK, CRITICAL, EXPR, ...)                             \
    do {                                                                                           \
        auto&          SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                     \
        constexpr auto SNITCH_TEMP_RESULT  = snitch::impl::match(EXPR, __VA_ARGS__);               \
        SNITCH_CURRENT_TEST.reg.report_assertion(                                                  \
            CRITICAL, SNITCH_TEMP_RESULT.first, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},         \
            CHECK "(" #EXPR ", " #__VA_ARGS__ "), got ", SNITCH_TEMP_RESULT.second);               \
    } while (0)

// clang-format off
#define SNITCH_CONSTEVAL_REQUIRE_THAT(EXPR, ...) SNITCH_CONSTEVAL_REQUIRE_THAT_IMPL("CONSTEVAL_REQUIRE_THAT", true,  EXPR, __VA_ARGS__)
#define SNITCH_CONSTEVAL_CHECK_THAT(EXPR, ...)   SNITCH_CONSTEVAL_REQUIRE_THAT_IMPL("CONSTEVAL_CHECK_THAT",   false, EXPR, __VA_ARGS__)
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
