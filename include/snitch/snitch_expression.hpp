#ifndef SNITCH_EXPRESSION_HPP
#define SNITCH_EXPRESSION_HPP

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_matcher.hpp"
#include "snitch/snitch_string.hpp"

#include <string_view>

namespace snitch {
// Maximum length of a `CHECK(...)` or `REQUIRE(...)` expression,
// beyond which automatic variable printing is disabled.
constexpr std::size_t max_expr_length = SNITCH_MAX_EXPR_LENGTH;
} // namespace snitch

namespace snitch::impl {
#define DEFINE_OPERATOR(OP, NAME, DISP, DISP_INV)                                                  \
    struct operator_##NAME {                                                                       \
        static constexpr std::string_view actual  = DISP;                                          \
        static constexpr std::string_view inverse = DISP_INV;                                      \
                                                                                                   \
        template<typename T, typename U>                                                           \
        constexpr bool operator()(const T& lhs, const U& rhs) const noexcept(noexcept(lhs OP rhs)) \
            requires(requires(const T& lhs, const U& rhs) { lhs OP rhs; })                         \
        {                                                                                          \
            return lhs OP rhs;                                                                     \
        }                                                                                          \
    }

DEFINE_OPERATOR(<, less, " < ", " >= ");
DEFINE_OPERATOR(>, greater, " > ", " <= ");
DEFINE_OPERATOR(<=, less_equal, " <= ", " > ");
DEFINE_OPERATOR(>=, greater_equal, " >= ", " < ");
DEFINE_OPERATOR(==, equal, " == ", " != ");
DEFINE_OPERATOR(!=, not_equal, " != ", " == ");

#undef DEFINE_OPERATOR

struct expression {
    std::string_view              type     = {};
    std::string_view              expected = {};
    small_string<max_expr_length> actual   = {};
    bool                          success  = true;

    template<string_appendable T>
    [[nodiscard]] constexpr bool append_value(T&& value) noexcept {
        return append(actual, std::forward<T>(value));
    }

    template<typename T>
    [[nodiscard]] constexpr bool append_value(T&&) noexcept {
        constexpr std::string_view unknown_value = "?";
        return append(actual, unknown_value);
    }
};

struct nondecomposable_expression : expression {};

struct invalid_expression {
    // This is an invalid expression; any further operator should produce another invalid
    // expression. We don't want to decompose these operators, but we need to declare them
    // so the expression compiles until calling to_expression(). This enable conditional
    // decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    constexpr invalid_expression operator OP(const V&) noexcept {                                  \
        return {};                                                                                 \
    }

    EXPR_OPERATOR_INVALID(<=)
    EXPR_OPERATOR_INVALID(<)
    EXPR_OPERATOR_INVALID(>=)
    EXPR_OPERATOR_INVALID(>)
    EXPR_OPERATOR_INVALID(==)
    EXPR_OPERATOR_INVALID(!=)
    EXPR_OPERATOR_INVALID(&&)
    EXPR_OPERATOR_INVALID(||)
    EXPR_OPERATOR_INVALID(=)
    EXPR_OPERATOR_INVALID(+=)
    EXPR_OPERATOR_INVALID(-=)
    EXPR_OPERATOR_INVALID(*=)
    EXPR_OPERATOR_INVALID(/=)
    EXPR_OPERATOR_INVALID(%=)
    EXPR_OPERATOR_INVALID(^=)
    EXPR_OPERATOR_INVALID(&=)
    EXPR_OPERATOR_INVALID(|=)
    EXPR_OPERATOR_INVALID(<<=)
    EXPR_OPERATOR_INVALID(>>=)
    EXPR_OPERATOR_INVALID(^)
    EXPR_OPERATOR_INVALID(|)
    EXPR_OPERATOR_INVALID(&)

#undef EXPR_OPERATOR_INVALID

    constexpr nondecomposable_expression to_expression() const noexcept {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression. But the code will be instantiated in
        // constexpr expressions, so don't static_assert.
        return nondecomposable_expression{};
    }
};

template<bool Expected, typename T, typename O, typename U>
struct extracted_binary_expression {
    std::string_view type;
    std::string_view expected;
    const T&         lhs;
    const U&         rhs;

    // This is a binary expression; any further operator should produce an invalid
    // expression, since we can't/won't decompose complex expressions. We don't want to decompose
    // these operators, but we need to declare them so the expression compiles until cast to bool.
    // This enable conditional decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    constexpr invalid_expression operator OP(const V&) noexcept {                                  \
        return {};                                                                                 \
    }

    EXPR_OPERATOR_INVALID(<=)
    EXPR_OPERATOR_INVALID(<)
    EXPR_OPERATOR_INVALID(>=)
    EXPR_OPERATOR_INVALID(>)
    EXPR_OPERATOR_INVALID(==)
    EXPR_OPERATOR_INVALID(!=)
    EXPR_OPERATOR_INVALID(&&)
    EXPR_OPERATOR_INVALID(||)
    EXPR_OPERATOR_INVALID(=)
    EXPR_OPERATOR_INVALID(+=)
    EXPR_OPERATOR_INVALID(-=)
    EXPR_OPERATOR_INVALID(*=)
    EXPR_OPERATOR_INVALID(/=)
    EXPR_OPERATOR_INVALID(%=)
    EXPR_OPERATOR_INVALID(^=)
    EXPR_OPERATOR_INVALID(&=)
    EXPR_OPERATOR_INVALID(|=)
    EXPR_OPERATOR_INVALID(<<=)
    EXPR_OPERATOR_INVALID(>>=)
    EXPR_OPERATOR_INVALID(^)
    EXPR_OPERATOR_INVALID(|)
    EXPR_OPERATOR_INVALID(&)

#define EXPR_COMMA ,
    EXPR_OPERATOR_INVALID(EXPR_COMMA)
#undef EXPR_COMMA

#undef EXPR_OPERATOR_INVALID

    // NB: Cannot make this noexcept since user operators may throw.
    constexpr expression to_expression() const noexcept(noexcept(static_cast<bool>(O{}(lhs, rhs))))
        requires(requires(const T& lhs, const U& rhs) { O{}(lhs, rhs); })
    {
        expression expr{type, expected};

        const bool actual = O{}(lhs, rhs);
        expr.success      = (actual == Expected);

        if (!expr.success || SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS) {
            if constexpr (matcher_for<T, U>) {
                using namespace snitch::matchers;
                const auto status = std::is_same_v<O, operator_equal> == actual
                                        ? match_status::matched
                                        : match_status::failed;
                if (!expr.append_value(lhs.describe_match(rhs, status))) {
                    expr.actual.clear();
                }
            } else if constexpr (matcher_for<U, T>) {
                using namespace snitch::matchers;
                const auto status = std::is_same_v<O, operator_equal> == actual
                                        ? match_status::matched
                                        : match_status::failed;
                if (!expr.append_value(rhs.describe_match(lhs, status))) {
                    expr.actual.clear();
                }
            } else {
                if (!expr.append_value(lhs) ||
                    !(actual ? expr.append_value(O::actual) : expr.append_value(O::inverse)) ||
                    !expr.append_value(rhs)) {
                    expr.actual.clear();
                }
            }
        }

        return expr;
    }

    constexpr nondecomposable_expression to_expression() const noexcept
        requires(!requires(const T& lhs, const U& rhs) { O{}(lhs, rhs); })
    {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression. But the code will be instantiated in
        // constexpr expressions, so don't static_assert.
        return nondecomposable_expression{};
    }
};

template<bool Expected, typename T>
struct extracted_unary_expression {
    std::string_view type;
    std::string_view expected;
    const T&         lhs;

    // Operators we want to decompose.
#define EXPR_OPERATOR(OP, OP_TYPE)                                                                 \
    template<typename U>                                                                           \
    constexpr extracted_binary_expression<Expected, T, OP_TYPE, U> operator OP(const U& rhs)       \
        const noexcept {                                                                           \
        return {type, expected, lhs, rhs};                                                         \
    }

    EXPR_OPERATOR(<, operator_less)
    EXPR_OPERATOR(>, operator_greater)
    EXPR_OPERATOR(<=, operator_less_equal)
    EXPR_OPERATOR(>=, operator_greater_equal)
    EXPR_OPERATOR(==, operator_equal)
    EXPR_OPERATOR(!=, operator_not_equal)

#undef EXPR_OPERATOR

    // We don't want to decompose the following operators, but we need to declare them so the
    // expression compiles until cast to bool. This enable conditional decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    constexpr invalid_expression operator OP(const V&) noexcept {                                  \
        return {};                                                                                 \
    }

    EXPR_OPERATOR_INVALID(&&)
    EXPR_OPERATOR_INVALID(||)
    EXPR_OPERATOR_INVALID(=)
    EXPR_OPERATOR_INVALID(+=)
    EXPR_OPERATOR_INVALID(-=)
    EXPR_OPERATOR_INVALID(*=)
    EXPR_OPERATOR_INVALID(/=)
    EXPR_OPERATOR_INVALID(%=)
    EXPR_OPERATOR_INVALID(^=)
    EXPR_OPERATOR_INVALID(&=)
    EXPR_OPERATOR_INVALID(|=)
    EXPR_OPERATOR_INVALID(<<=)
    EXPR_OPERATOR_INVALID(>>=)
    EXPR_OPERATOR_INVALID(^)
    EXPR_OPERATOR_INVALID(|)
    EXPR_OPERATOR_INVALID(&)

#define EXPR_COMMA ,
    EXPR_OPERATOR_INVALID(EXPR_COMMA)
#undef EXPR_COMMA

#undef EXPR_OPERATOR_INVALID

    constexpr expression to_expression() const noexcept(noexcept(static_cast<bool>(lhs)))
        requires(requires(const T& lhs) { static_cast<bool>(lhs); })
    {
        expression expr{type, expected};

        expr.success = (static_cast<bool>(lhs) == Expected);

        if (!expr.success || SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS) {
            if (!expr.append_value(lhs)) {
                expr.actual.clear();
            }
        }

        return expr;
    }

    constexpr nondecomposable_expression to_expression() const noexcept
        requires(!requires(const T& lhs) { static_cast<bool>(lhs); })
    {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression. But the code will be instantiated in
        // constexpr expressions, so don't static_assert.
        return nondecomposable_expression{};
    }
};

template<bool Expected>
struct expression_extractor {
    std::string_view type;
    std::string_view expected;

    template<typename T>
    constexpr extracted_unary_expression<Expected, T> operator<=(const T& lhs) const noexcept {
        return {type, expected, lhs};
    }
};

template<typename T>
constexpr bool is_decomposable = !std::is_same_v<T, nondecomposable_expression>;
} // namespace snitch::impl

#endif
