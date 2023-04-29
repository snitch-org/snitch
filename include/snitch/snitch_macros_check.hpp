#ifndef SNITCH_MACROS_CHECK_HPP
#define SNITCH_MACROS_CHECK_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_macros_check_base.hpp"
#include "snitch/snitch_macros_warnings.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"

#define SNITCH_REQUIRE(...)                                                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_FALSE("REQUIRE", __VA_ARGS__)) {                                    \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "REQUIRE(" #__VA_ARGS__ ")");       \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CHECK(...)                                                                          \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_FALSE("CHECK", __VA_ARGS__)) {                                      \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "CHECK(" #__VA_ARGS__ ")");         \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_REQUIRE_FALSE(...)                                                                  \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_TRUE("REQUIRE_FALSE", __VA_ARGS__)) {                               \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "REQUIRE_FALSE(" #__VA_ARGS__ ")"); \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CHECK_FALSE(...)                                                                    \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_TRUE("CHECK_FALSE", __VA_ARGS__)) {                                 \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "CHECK_FALSE(" #__VA_ARGS__ ")");   \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_FAIL(MESSAGE)                                                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_CURRENT_TEST.reg.report_failure(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
        SNITCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNITCH_FAIL_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_CURRENT_TEST.reg.report_failure(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
    } while (0)

#define SNITCH_SKIP(MESSAGE)                                                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_skipped(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
        SNITCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNITCH_SKIP_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_skipped(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
    } while (0)

#define SNITCH_REQUIRE_THAT(EXPR, ...)                                                             \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                       \
        auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                                  \
        if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                       \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "REQUIRE_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                                 \
                SNITCH_TEMP_MATCHER.describe_match(                                                \
                    SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));                   \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
    } while (0)

#define SNITCH_CHECK_THAT(EXPR, ...)                                                               \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                       \
        auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                                  \
        if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                       \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CHECK_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                                   \
                SNITCH_TEMP_MATCHER.describe_match(                                                \
                    SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));                   \
        }                                                                                          \
    } while (0)

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
