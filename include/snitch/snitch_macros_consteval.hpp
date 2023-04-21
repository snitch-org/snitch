#ifndef SNITCH_MACROS_CONSTEVAL_HPP
#define SNITCH_MACROS_CONSTEVAL_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_macros_check_base.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"

#define SNITCH_CONSTEVAL_REQUIRE(...)                                                              \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE("CONSTEVAL_REQUIRE", __VA_ARGS__)) {      \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_REQUIRE(" #__VA_ARGS__ ")");                                        \
                SNITCH_TESTING_ABORT;                                                              \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_REQUIRE(" #__VA_ARGS__ ")");                                        \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_CHECK(...)                                                                \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE("CONSTEVAL_CHECK", __VA_ARGS__)) {        \
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
                    "CONSTEVAL_CHECK(" #__VA_ARGS__ ")");                                          \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_CHECK(" #__VA_ARGS__ ")");                                          \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_REQUIRE_FALSE(...)                                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE("CONSTEVAL_REQUIRE_FALSE", __VA_ARGS__)) { \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_REQUIRE_FALSE(" #__VA_ARGS__ ")");                                  \
                SNITCH_TESTING_ABORT;                                                              \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_REQUIRE_FALSE(" #__VA_ARGS__ ")");                                  \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_CHECK_FALSE(...)                                                          \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE("CONSTEVAL_CHECK_FALSE", __VA_ARGS__)) {   \
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
                    "CONSTEVAL_CHECK_FALSE(" #__VA_ARGS__ ")");                                    \
            } else {                                                                               \
                SNITCH_CURRENT_TEST.reg.report_success(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_CHECK_FALSE(" #__VA_ARGS__ ")");                                    \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_REQUIRE_THAT(EXPR, ...)                                                   \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEVAL_REQUIRE_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                       \
                SNITCH_TEMP_ERROR.value());                                                        \
            SNITCH_TESTING_ABORT;                                                                  \
        } else {                                                                                   \
            SNITCH_CURRENT_TEST.reg.report_success(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEVAL_REQUIRE_THAT(" #EXPR ", " #__VA_ARGS__ ")");                            \
        }                                                                                          \
    } while (0)

#define SNITCH_CONSTEVAL_CHECK_THAT(EXPR, ...)                                                     \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEVAL_CHECK_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                         \
                SNITCH_TEMP_ERROR.value());                                                        \
        } else {                                                                                   \
            SNITCH_CURRENT_TEST.reg.report_success(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEVAL_CHECK_THAT(" #EXPR ", " #__VA_ARGS__ ")");                              \
        }                                                                                          \
    } while (0)

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
