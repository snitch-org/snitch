#ifndef SNITCH_MACROS_EXCEPTIONS_HPP
#define SNITCH_MACROS_EXCEPTIONS_HPP

#if SNITCH_WITH_EXCEPTIONS
#    include "snitch/snitch_config.hpp"
#    include "snitch/snitch_macros_utility.hpp"
#    include "snitch/snitch_matcher.hpp"
#    include "snitch/snitch_registry.hpp"

#    include <exception>

#    if SNITCH_ENABLE
#        define SNITCH_REQUIRE_THROWS_AS_IMPL(MAYBE_ABORT, EXPRESSION, ...)                        \
            do {                                                                                   \
                auto SNITCH_CURRENT_CHECK       = SNITCH_NEW_CHECK;                                \
                bool SNITCH_NO_EXCEPTION_THROWN = false;                                           \
                try {                                                                              \
                    static_cast<void>(EXPRESSION);                                                 \
                    SNITCH_NO_EXCEPTION_THROWN = true;                                             \
                } catch (const __VA_ARGS__&) {                                                     \
                    snitch::registry::report_assertion(                                            \
                        true, #__VA_ARGS__ " was thrown as expected");                             \
                    snitch::notify_exception_handled();                                            \
                } catch (...) {                                                                    \
                    try {                                                                          \
                        throw;                                                                     \
                    } catch (const std::exception& e) {                                            \
                        snitch::registry::report_assertion(                                        \
                            false,                                                                 \
                            #__VA_ARGS__ " expected but other std::exception thrown; message: ",   \
                            e.what());                                                             \
                    } catch (...) {                                                                \
                        snitch::registry::report_assertion(                                        \
                            false, #__VA_ARGS__ " expected but other unknown exception thrown");   \
                    }                                                                              \
                    snitch::notify_exception_handled();                                            \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
                if (SNITCH_NO_EXCEPTION_THROWN) {                                                  \
                    snitch::registry::report_assertion(                                            \
                        false, #__VA_ARGS__ " expected but no exception thrown");                  \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
            } while (0)

#        define SNITCH_REQUIRE_THROWS_AS(EXPRESSION, ...)                                          \
            SNITCH_REQUIRE_THROWS_AS_IMPL(SNITCH_TESTING_ABORT, EXPRESSION, __VA_ARGS__)
#        define SNITCH_CHECK_THROWS_AS(EXPRESSION, ...)                                            \
            SNITCH_REQUIRE_THROWS_AS_IMPL((void)0, EXPRESSION, __VA_ARGS__)

#        define SNITCH_REQUIRE_THROWS_MATCHES_IMPL(MAYBE_ABORT, EXPRESSION, EXCEPTION, ...)        \
            do {                                                                                   \
                auto SNITCH_CURRENT_CHECK       = SNITCH_NEW_CHECK;                                \
                bool SNITCH_NO_EXCEPTION_THROWN = false;                                           \
                try {                                                                              \
                    static_cast<void>(EXPRESSION);                                                 \
                    SNITCH_NO_EXCEPTION_THROWN = true;                                             \
                } catch (const EXCEPTION& e) {                                                     \
                    auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                      \
                    if (!SNITCH_TEMP_MATCHER.match(e)) {                                           \
                        snitch::registry::report_assertion(                                        \
                            false,                                                                 \
                            "could not match caught " #EXCEPTION " with expected content: ",       \
                            SNITCH_TEMP_MATCHER.describe_match(                                    \
                                e, snitch::matchers::match_status::failed));                       \
                        snitch::notify_exception_handled();                                        \
                        MAYBE_ABORT;                                                               \
                    } else {                                                                       \
                        snitch::registry::report_assertion(                                        \
                            true, "caught " #EXCEPTION " matched expected content: ",              \
                            SNITCH_TEMP_MATCHER.describe_match(                                    \
                                e, snitch::matchers::match_status::matched));                      \
                        snitch::notify_exception_handled();                                        \
                    }                                                                              \
                } catch (...) {                                                                    \
                    try {                                                                          \
                        throw;                                                                     \
                    } catch (const std::exception& e) {                                            \
                        snitch::registry::report_assertion(                                        \
                            false,                                                                 \
                            #EXCEPTION " expected but other std::exception thrown; message: ",     \
                            e.what());                                                             \
                    } catch (...) {                                                                \
                        snitch::registry::report_assertion(                                        \
                            false, #EXCEPTION " expected but other unknown exception thrown");     \
                    }                                                                              \
                    snitch::notify_exception_handled();                                            \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
                if (SNITCH_NO_EXCEPTION_THROWN) {                                                  \
                    snitch::registry::report_assertion(                                            \
                        false, #EXCEPTION " expected but no exception thrown");                    \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
            } while (0)

#        define SNITCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)                          \
            SNITCH_REQUIRE_THROWS_MATCHES_IMPL(                                                    \
                SNITCH_TESTING_ABORT, EXPRESSION, EXCEPTION, __VA_ARGS__)
#        define SNITCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)                            \
            SNITCH_REQUIRE_THROWS_MATCHES_IMPL((void)0, EXPRESSION, EXCEPTION, __VA_ARGS__)

#        define SNITCH_REQUIRE_NOTHROW_IMPL(MAYBE_ABORT, ...)                                      \
            do {                                                                                   \
                auto SNITCH_CURRENT_CHECK = SNITCH_NEW_CHECK;                                      \
                try {                                                                              \
                    static_cast<void>(__VA_ARGS__);                                                \
                    snitch::registry::report_assertion(true, #__VA_ARGS__ " did not throw");       \
                } catch (...) {                                                                    \
                    try {                                                                          \
                        throw;                                                                     \
                    } catch (const std::exception& e) {                                            \
                        snitch::registry::report_assertion(                                        \
                            false,                                                                 \
                            "expected " #__VA_ARGS__                                               \
                            " not to throw but it threw a std::exception; message: ",              \
                            e.what());                                                             \
                    } catch (...) {                                                                \
                        snitch::registry::report_assertion(                                        \
                            false, "expected " #__VA_ARGS__                                        \
                                   " not to throw but it threw an unknown exception");             \
                    }                                                                              \
                    snitch::notify_exception_handled();                                            \
                    MAYBE_ABORT;                                                                   \
                }                                                                                  \
            } while (0)

#        define SNITCH_REQUIRE_NOTHROW(...)                                                        \
            SNITCH_REQUIRE_NOTHROW_IMPL(SNITCH_TESTING_ABORT, __VA_ARGS__)
#        define SNITCH_CHECK_NOTHROW(...) SNITCH_REQUIRE_NOTHROW_IMPL((void)0, __VA_ARGS__)

#    else // SNITCH_ENABLE

// clang-format off
#    define SNITCH_REQUIRE_THROWS_AS(EXPRESSION, ...)                 SNITCH_DISCARD_ARGS(EXPRESSION, sizeof(__VA_ARGS__))
#    define SNITCH_CHECK_THROWS_AS(EXPRESSION, ...)                   SNITCH_DISCARD_ARGS(EXPRESSION, sizeof(__VA_ARGS__))
#    define SNITCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...) SNITCH_DISCARD_ARGS(EXPRESSION, sizeof(EXCEPTION), __VA_ARGS__)
#    define SNITCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)   SNITCH_DISCARD_ARGS(EXPRESSION, sizeof(EXCEPTION), __VA_ARGS__)
#    define SNITCH_REQUIRE_NOTHROW(...)                               SNITCH_DISCARD_ARGS(__VA_ARGS__)
#    define SNITCH_CHECK_NOTHROW(...)                                 SNITCH_DISCARD_ARGS(__VA_ARGS__)
// clang-format on

#    endif // SNITCH_ENABLE

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
