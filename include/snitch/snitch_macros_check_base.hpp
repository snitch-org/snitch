#ifndef SNITCH_MACROS_CHECK_BASE_HPP
#define SNITCH_MACROS_CHECK_BASE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_registry.hpp"

#include <string_view>

#if SNITCH_WITH_EXCEPTIONS
#    define SNITCH_TESTING_ABORT                                                                   \
        throw snitch::impl::abort_exception {}
#else
#    define SNITCH_TESTING_ABORT std::terminate()
#endif

#define SNITCH_NEW_CHECK                                                                           \
    snitch::impl::scoped_test_check {                                                              \
        SNITCH_CURRENT_LOCATION                                                                    \
    }

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
    snitch::registry::report_assertion(                                                            \
        SNITCH_CURRENT_EXPRESSION.success, SNITCH_CURRENT_EXPRESSION);                             \
    if (!SNITCH_CURRENT_EXPRESSION.success) {                                                      \
        MAYBE_ABORT;                                                                               \
    }

#endif
