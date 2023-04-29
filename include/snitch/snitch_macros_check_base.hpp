#ifndef SNITCH_MACROS_CHECK_BASE_HPP
#define SNITCH_MACROS_CHECK_BASE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_expression.hpp"

#include <string_view>

#define SNITCH_EXPR(TYPE, EXPECTED, ...)                                                           \
    auto SNITCH_CURRENT_EXPRESSION =                                                               \
        (snitch::impl::expression_extractor<EXPECTED>{TYPE "(" #__VA_ARGS__ ")"} <= __VA_ARGS__)   \
            .to_expression()

#define SNITCH_EXPR_IS_TRUE(TYPE, ...)                                                             \
    auto SNITCH_CURRENT_EXPRESSION =                                                               \
        (snitch::impl::expression_extractor<true>{TYPE "(" #__VA_ARGS__ ")"} <= __VA_ARGS__)       \
            .to_expression()

#define SNITCH_EXPR_IS_FALSE(TYPE, ...)                                                            \
    auto SNITCH_CURRENT_EXPRESSION =                                                               \
        (snitch::impl::expression_extractor<false>{TYPE "(" #__VA_ARGS__ ")"} <= __VA_ARGS__)      \
            .to_expression()

#define SNITCH_IS_DECOMPOSABLE(...)                                                                \
    snitch::impl::is_decomposable<decltype((snitch::impl::expression_extractor<true>{              \
                                                std::declval<std::string_view>()} <= __VA_ARGS__)  \
                                               .to_expression())>

#endif
