#ifndef SNITCH_MACROS_CONSTEXPR_HPP
#define SNITCH_MACROS_CONSTEXPR_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_macros_check_base.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"

#define SNITCH_CONSTEXPR_REQUIRE(...)                                                              \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        bool SNITCH_CURRENT_ASSERTION_FAILED = false;                                              \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE(                                          \
                              "CONSTEXPR_REQUIRE[compile-time]", __VA_ARGS__)) {                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
            if (SNITCH_EXPR_IS_FALSE("CONSTEXPR_REQUIRE[run-time]", __VA_ARGS__)) {                \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE[compile-time](" #__VA_ARGS__ ")");                          \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE[compile-time](" #__VA_ARGS__ ")");                          \
            }                                                                                      \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE[run-time](" #__VA_ARGS__ ")");                              \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE[run-time](" #__VA_ARGS__ ")");                              \
            }                                                                                      \
        }                                                                                          \
        if (SNITCH_CURRENT_ASSERTION_FAILED) {                                                     \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_CHECK(...)                                                                \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE(                                          \
                              "CONSTEXPR_CHECK[compile-time]", __VA_ARGS__)) {                     \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
            if (SNITCH_EXPR_IS_FALSE("CONSTEXPR_CHECK[run-time]", __VA_ARGS__)) {                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK[compile-time](" #__VA_ARGS__ ")");                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK[compile-time](" #__VA_ARGS__ ")");                            \
            }                                                                                      \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK[run-time](" #__VA_ARGS__ ")");                                \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK[run-time](" #__VA_ARGS__ ")");                                \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_REQUIRE_FALSE(...)                                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        bool SNITCH_CURRENT_ASSERTION_FAILED = false;                                              \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE(                                           \
                              "CONSTEXPR_REQUIRE_FALSE[compile-time]", __VA_ARGS__)) {             \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
            if (SNITCH_EXPR_IS_TRUE("CONSTEXPR_REQUIRE_FALSE[run-time]", __VA_ARGS__)) {           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_FALSE[compile-time](" #__VA_ARGS__ ")");                    \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_FALSE[compile-time](" #__VA_ARGS__ ")");                    \
            }                                                                                      \
            if (__VA_ARGS__) {                                                                     \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_FALSE[run-time](" #__VA_ARGS__ ")");                        \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_FALSE[run-time](" #__VA_ARGS__ ")");                        \
            }                                                                                      \
        }                                                                                          \
        if (SNITCH_CURRENT_ASSERTION_FAILED) {                                                     \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_CHECK_FALSE(...)                                                          \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE(                                           \
                              "CONSTEXPR_CHECK_FALSE[compile-time]", __VA_ARGS__)) {               \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
            if (SNITCH_EXPR_IS_TRUE("CONSTEXPR_CHECK_FALSE[run-time]", __VA_ARGS__)) {             \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_FALSE[compile-time](" #__VA_ARGS__ ")");                      \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_FALSE[compile-time](" #__VA_ARGS__ ")");                      \
            }                                                                                      \
            if (__VA_ARGS__) {                                                                     \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_FALSE[run-time](" #__VA_ARGS__ ")");                          \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_FALSE[run-time](" #__VA_ARGS__ ")");                          \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_REQUIRE_THAT(EXPR, ...)                                                   \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        bool SNITCH_CURRENT_ASSERTION_FAILED = false;                                              \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEXPR_REQUIRE_THAT[compile-time](" #EXPR ", " #__VA_ARGS__ "), got ",         \
                SNITCH_TEMP_ERROR.value());                                                        \
            SNITCH_CURRENT_ASSERTION_FAILED = true;                                                \
        } else {                                                                                   \
            SNITCH_CURRENT_TEST.reg.report_success(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEXPR_REQUIRE_THAT[compile-time](" #EXPR ", " #__VA_ARGS__ ")");              \
        }                                                                                          \
        {                                                                                          \
            auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                   \
            auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                              \
            if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_THAT[run-time](" #EXPR ", " #__VA_ARGS__ "), got ",         \
                    SNITCH_TEMP_MATCHER.describe_match(                                            \
                        SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));               \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_THAT[run-time](" #EXPR ", " #__VA_ARGS__ "), got ",         \
                    SNITCH_TEMP_MATCHER.describe_match(                                            \
                        SNITCH_TEMP_VALUE, snitch::matchers::match_status::matched));              \
            }                                                                                      \
        }                                                                                          \
        if (SNITCH_CURRENT_ASSERTION_FAILED) {                                                     \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
    } while (0)

#define SNITCH_CONSTEXPR_CHECK_THAT(EXPR, ...)                                                     \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEXPR_CHECK_THAT[compile-time](" #EXPR ", " #__VA_ARGS__ "), got ",           \
                SNITCH_TEMP_ERROR.value());                                                        \
        } else {                                                                                   \
            SNITCH_CURRENT_TEST.reg.report_success(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEXPR_CHECK_THAT[compile-time](" #EXPR ", " #__VA_ARGS__ ")");                \
        }                                                                                          \
        {                                                                                          \
            auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                   \
            auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                              \
            if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_THAT[run-time](" #EXPR ", " #__VA_ARGS__ "), got ",           \
                    SNITCH_TEMP_MATCHER.describe_match(                                            \
                        SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));               \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_THAT[run-time](" #EXPR ", " #__VA_ARGS__ "), got ",           \
                    SNITCH_TEMP_MATCHER.describe_match(                                            \
                        SNITCH_TEMP_VALUE, snitch::matchers::match_status::matched));              \
            }                                                                                      \
        }                                                                                          \
    } while (0)

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define CONSTEXPR_REQUIRE(...)           SNITCH_CONSTEXPR_REQUIRE(__VA_ARGS__)
#    define CONSTEXPR_CHECK(...)             SNITCH_CONSTEXPR_CHECK(__VA_ARGS__)
#    define CONSTEXPR_REQUIRE_FALSE(...)     SNITCH_CONSTEXPR_REQUIRE_FALSE(__VA_ARGS__)
#    define CONSTEXPR_CHECK_FALSE(...)       SNITCH_CONSTEXPR_CHECK_FALSE(__VA_ARGS__)
#    define CONSTEXPR_REQUIRE_THAT(EXP, ...) SNITCH_CONSTEXPR_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CONSTEXPR_CHECK_THAT(EXP, ...)   SNITCH_CONSTEXPR_CHECK_THAT(EXP, __VA_ARGS__)
#endif
// clang-format on

#endif
