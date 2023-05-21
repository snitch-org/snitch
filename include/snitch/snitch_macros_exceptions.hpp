#ifndef SNITCH_MACROS_EXCEPTIONS_HPP
#define SNITCH_MACROS_EXCEPTIONS_HPP

#if SNITCH_WITH_EXCEPTIONS
#    include "snitch/snitch_config.hpp"
#    include "snitch/snitch_matcher.hpp"
#    include "snitch/snitch_registry.hpp"

#    include <exception>

#    define SNITCH_REQUIRE_THROWS_AS_IMPL(MAYBE_ABORT, EXPRESSION, ...)                            \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                static_cast<void>(EXPRESSION);                                                     \
                SNITCH_CURRENT_TEST.reg.report_assertion(                                          \
                    false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                              \
                    #__VA_ARGS__ " expected but no exception thrown");                             \
                MAYBE_ABORT;                                                                       \
            } catch (const __VA_ARGS__&) {                                                         \
                SNITCH_CURRENT_TEST.reg.report_assertion(                                          \
                    true, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                               \
                    #__VA_ARGS__ " was thrown as expected");                                       \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        #__VA_ARGS__ " expected but other std::exception thrown; message: ",       \
                        e.what());                                                                 \
                    MAYBE_ABORT;                                                                   \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        #__VA_ARGS__ " expected but other unknown exception thrown");              \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
            }                                                                                      \
        } while (0)

#    define SNITCH_REQUIRE_THROWS_AS(EXPRESSION, ...)                                              \
        SNITCH_REQUIRE_THROWS_AS_IMPL(SNITCH_TESTING_ABORT, EXPRESSION, __VA_ARGS__)
#    define SNITCH_CHECK_THROWS_AS(EXPRESSION, ...)                                                \
        SNITCH_REQUIRE_THROWS_AS_IMPL((void)0, EXPRESSION, __VA_ARGS__)

#    define SNITCH_REQUIRE_THROWS_MATCHES_IMPL(MAYBE_ABORT, EXPRESSION, EXCEPTION, ...)            \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                static_cast<void>(EXPRESSION);                                                     \
                SNITCH_CURRENT_TEST.reg.report_assertion(                                          \
                    false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                              \
                    #EXCEPTION " expected but no exception thrown");                               \
                MAYBE_ABORT;                                                                       \
            } catch (const EXCEPTION& e) {                                                         \
                auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                          \
                if (!SNITCH_TEMP_MATCHER.match(e)) {                                               \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        SNITCH_TEMP_MATCHER.describe_match(                                        \
                            e, snitch::matchers::match_status::failed));                           \
                    MAYBE_ABORT;                                                                   \
                } else {                                                                           \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        true, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                           \
                        "caught " #EXCEPTION " matched expected content: ",                        \
                        SNITCH_TEMP_MATCHER.describe_match(                                        \
                            e, snitch::matchers::match_status::matched));                          \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                    MAYBE_ABORT;                                                                   \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
            }                                                                                      \
        } while (0)

#    define SNITCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)                              \
        SNITCH_REQUIRE_THROWS_MATCHES_IMPL(SNITCH_TESTING_ABORT, EXPRESSION, EXCEPTION, __VA_ARGS__)
#    define SNITCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)                                \
        SNITCH_REQUIRE_THROWS_MATCHES_IMPL((void)0, EXPRESSION, EXCEPTION, __VA_ARGS__)

#    define SNITCH_REQUIRE_NOTHROW_IMPL(MAYBE_ABORT, ...)                                          \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                static_cast<void>(__VA_ARGS__);                                                    \
                SNITCH_CURRENT_TEST.reg.report_assertion(                                          \
                    true, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                               \
                    #__VA_ARGS__ " did not throw");                                                \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        "expected " #__VA_ARGS__                                                   \
                        " not to throw but it threw a std::exception; message: ",                  \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_assertion(                                      \
                        false, SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                          \
                        "expected " #__VA_ARGS__                                                   \
                        " not to throw but it threw an unknown exception");                        \
                }                                                                                  \
                MAYBE_ABORT;                                                                       \
            }                                                                                      \
        } while (0)

#    define SNITCH_REQUIRE_NOTHROW(...)                                                            \
        SNITCH_REQUIRE_NOTHROW_IMPL(SNITCH_TESTING_ABORT, __VA_ARGS__)
#    define SNITCH_CHECK_NOTHROW(...) SNITCH_REQUIRE_NOTHROW_IMPL((void)0, __VA_ARGS__)

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define REQUIRE_THROWS_AS(EXPRESSION, ...)                 SNITCH_REQUIRE_THROWS_AS(EXPRESSION, __VA_ARGS__)
#    define CHECK_THROWS_AS(EXPRESSION, ...)                   SNITCH_CHECK_THROWS_AS(EXPRESSION, __VA_ARGS__)
#    define REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...) SNITCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, __VA_ARGS__)
#    define CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)   SNITCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, __VA_ARGS__)
#    define REQUIRE_NOTHROW(...)                               SNITCH_REQUIRE_NOTHROW(__VA_ARGS__)
#    define CHECK_NOTHROW(...)                                 SNITCH_CHECK_NOTHROW(__VA_ARGS__)
#endif
// clang-format on

#endif

#endif
