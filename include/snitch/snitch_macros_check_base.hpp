#ifndef SNITCH_MACROS_CHECK_BASE_HPP
#define SNITCH_MACROS_CHECK_BASE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"

#include <string_view>

#if SNITCH_WITH_EXCEPTIONS
#    define SNITCH_TESTING_ABORT                                                                   \
        throw snitch::impl::abort_exception {}
#    define SNITCH_TRY try
#    define SNITCH_CATCH(CHECK)                                                                    \
        catch (...) {                                                                              \
            SNITCH_WARNING_PUSH                                                                    \
            SNITCH_WARNING_DISABLE_TERMINATE                                                       \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const snitch::impl::abort_exception&) {                                       \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                SNITCH_CURRENT_TEST.reg.report_assertion(                                          \
                    false, SNITCH_CURRENT_TEST, SNITCH_CURRENT_LOCATION,                           \
                    "std::exception thrown during " #CHECK "(); message: ", e.what());             \
                SNITCH_TESTING_ABORT;                                                              \
            } catch (...) {                                                                        \
                SNITCH_CURRENT_TEST.reg.report_assertion(                                          \
                    false, SNITCH_CURRENT_TEST, SNITCH_CURRENT_LOCATION,                           \
                    "unknown exception thrown during " #CHECK "()");                               \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
            SNITCH_WARNING_POP                                                                     \
        }
#else
#    define SNITCH_TESTING_ABORT std::terminate()
#    define SNITCH_TRY
#    define SNITCH_CATCH(CHECK)
#endif

#define SNITCH_EXPR(TYPE, EXPECTED, ...)                                                           \
    auto SNITCH_CURRENT_EXPRESSION =                                                               \
        (snitch::impl::expression_extractor<EXPECTED>{TYPE, #__VA_ARGS__} <= __VA_ARGS__)          \
            .to_expression()

#define SNITCH_IS_DECOMPOSABLE(...)                                                                \
    snitch::impl::is_decomposable<decltype((snitch::impl::expression_extractor<true>{              \
                                                std::declval<std::string_view>(),                  \
                                                std::declval<std::string_view>()} <= __VA_ARGS__)  \
                                               .to_expression())>

#define SNITCH_REPORT_EXPRESSION(MAYBE_ABORT)                                                      \
    SNITCH_CURRENT_TEST.reg.report_assertion(                                                      \
        SNITCH_CURRENT_EXPRESSION.success, SNITCH_CURRENT_TEST, SNITCH_CURRENT_LOCATION,           \
        SNITCH_CURRENT_EXPRESSION);                                                                \
    if (!SNITCH_CURRENT_EXPRESSION.success) {                                                      \
        MAYBE_ABORT;                                                                               \
    }

#endif
