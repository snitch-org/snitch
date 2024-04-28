#include "testing.hpp"
#include "testing_event.hpp"

#include <algorithm>
#include <compare>

#if SNITCH_WITH_EXCEPTIONS
#    include <stdexcept>
#endif

using namespace std::literals;

namespace {
struct non_relocatable {
    int value = 0;

    explicit non_relocatable(int v) : value(v) {}
    non_relocatable(const non_relocatable&)            = delete;
    non_relocatable(non_relocatable&&)                 = delete;
    non_relocatable& operator=(const non_relocatable&) = delete;
    non_relocatable& operator=(non_relocatable&&)      = delete;
    ~non_relocatable() {
        value = 0;
    }

    bool operator==(const non_relocatable& other) const {
        return this->value == other.value;
    }

    bool operator!=(const non_relocatable& other) const {
        return this->value != other.value;
    }
};

bool append(snitch::small_string_span ss, const non_relocatable& o) noexcept {
    return append(ss, "non_relocatable{", o.value, "}");
}

struct non_appendable {
    int value = 0;

    explicit non_appendable(int v) : value(v) {}

    bool operator==(const non_appendable& other) const {
        return this->value == other.value;
    }

    bool operator!=(const non_appendable& other) const {
        return this->value != other.value;
    }
};

struct unary_long_string {
    snitch::small_string<2048> value;

    unary_long_string() {
        value.resize(2048);
        std::fill(value.begin(), value.end(), '0');
    }

    explicit operator bool() const {
        return false;
    }

    bool operator!() const {
        return true;
    }
};

bool append(snitch::small_string_span ss, const unary_long_string& u) noexcept {
    return append(ss, u.value);
}
} // namespace

namespace snitch::matchers {
struct long_matcher_always_fails {
    bool match(std::string_view) const noexcept {
        return false;
    }

    small_string<max_expr_length * 2>
    describe_match(std::string_view, match_status) const noexcept {
        small_string<max_expr_length * 2> message;
        message.resize(message.capacity());
        std::fill(message.begin(), message.end(), '0');
        return message;
    }
};
} // namespace snitch::matchers

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_INT_BOOLEAN

TEST_CASE("check unary", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("bool true") {
        bool value = true;

        {
            test_override override(catcher);
            SNITCH_CHECK(value);
        }

        CHECK(value == true);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("bool false") {
        bool        value        = false;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == false);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value"sv, "false"sv);
    }

    SECTION("bool !true") {
        bool        value        = true;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(!value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == true);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "!value"sv, "false"sv);
    }

    SECTION("bool !false") {
        bool value = false;

        {
            test_override override(catcher);
            SNITCH_CHECK(!value);
        }

        CHECK(value == false);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer non-zero") {
        int value = 5;

        {
            test_override override(catcher);
            SNITCH_CHECK(value);
        }

        CHECK(value == 5);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer zero") {
        int         value        = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value"sv, "0"sv);
    }

    SECTION("integer pre increment") {
        int value = 0;

        {
            test_override override(catcher);
            SNITCH_CHECK(++value);
        }

        CHECK(value == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer post increment") {
        int         value        = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value++); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value++"sv, "0"sv);
    }

    SECTION("integer expression * pass") {
        int value = 1;

        {
            test_override override(catcher);
            SNITCH_CHECK(2 * value);
        }

        CHECK(value == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer expression / pass") {
        int value = 1;

        {
            test_override override(catcher);
            SNITCH_CHECK(2 / value);
        }

        CHECK(value == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer expression + pass") {
        int value = 1;

        {
            test_override override(catcher);
            SNITCH_CHECK(2 + value);
        }

        CHECK(value == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer expression - pass") {
        int value = 3;

        {
            test_override override(catcher);
            SNITCH_CHECK(2 - value);
        }

        CHECK(value == 3);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer expression % pass") {
        int value = 3;

        {
            test_override override(catcher);
            SNITCH_CHECK(2 % value);
        }

        CHECK(value == 3);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer expression * fail") {
        int         value        = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(2 * value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "2 * value"sv, "0"sv);
    }

    SECTION("integer expression / fail") {
        int         value        = 5;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(2 / value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 5);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "2 / value"sv, "0"sv);
    }

    SECTION("integer expression + fail") {
        int         value        = -2;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(2 + value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == -2);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "2 + value"sv, "0"sv);
    }

    SECTION("integer expression - fail") {
        int         value        = 2;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(2 - value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 2);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "2 - value"sv, "0"sv);
    }

    SECTION("integer expression % fail") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(2 % value); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "2 % value"sv, "0"sv);
    }
}

SNITCH_WARNING_POP

TEST_CASE("check binary", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("integer == pass") {
        int value1 = 0;
        int value2 = 0;

        {
            test_override override(catcher);
            SNITCH_CHECK(value1 == value2);
        }

        CHECK(value1 == 0);
        CHECK(value2 == 0);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer != pass") {
        int value1 = 0;
        int value2 = 1;

        {
            test_override override(catcher);
            SNITCH_CHECK(value1 != value2);
        }

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer < pass") {
        int value1 = 0;
        int value2 = 1;

        {
            test_override override(catcher);
            SNITCH_CHECK(value1 < value2);
        }

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer > pass") {
        int value1 = 1;
        int value2 = 0;

        {
            test_override override(catcher);
            SNITCH_CHECK(value1 > value2);
        }

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer <= pass") {
        int value1 = 0;
        int value2 = 1;

        {
            test_override override(catcher);
            SNITCH_CHECK(value1 <= value2);
        }

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer >= pass") {
        int value1 = 1;
        int value2 = 0;

        {
            test_override override(catcher);
            SNITCH_CHECK(value1 >= value2);
        }

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("integer == fail") {
        int         value1       = 0;
        int         value2       = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 == value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 == value2"sv, "0 != 1"sv);
    }

    SECTION("integer != fail") {
        int         value1       = 0;
        int         value2       = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 != value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 0);
        CHECK(value2 == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 != value2"sv, "0 == 0"sv);
    }

    SECTION("integer < fail") {
        int         value1       = 1;
        int         value2       = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 < value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 < value2"sv, "1 >= 0"sv);
    }

    SECTION("integer > fail") {
        int         value1       = 0;
        int         value2       = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 > value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 > value2"sv, "0 <= 1"sv);
    }

    SECTION("integer <= fail") {
        int         value1       = 1;
        int         value2       = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 <= value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 <= value2"sv, "1 > 0"sv);
    }

    SECTION("integer >= fail") {
        int         value1       = 0;
        int         value2       = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 >= value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 >= value2"sv, "0 < 1"sv);
    }
}

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_PRECEDENCE
SNITCH_WARNING_DISABLE_ASSIGNMENT

#if defined(__cpp_consteval) && __cpp_consteval <= 202211L
// Regression in MSVC compiler
// https://github.com/snitch-org/snitch/issues/140
// https://developercommunity.visualstudio.com/t/Regression:-False-positive-C7595:-std::/10509214
#    define SNITCH_TEST_NO_SPACESHIP
#endif

TEST_CASE("check no decomposition", "[test macros]") {
    event_catcher<1> catcher;

#if !defined(SNITCH_TEST_NO_SPACESHIP)
    SECTION("spaceship") {
        int         value1       = 1;
        int         value2       = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 <=> value2 != 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 1);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 <=> value2 != 0"sv, ""sv);
    }
#endif

    SECTION("with operator &&") {
        int         value1       = 1;
        int         value2       = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 == 1 && value2 == 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 1);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 == 1 && value2 == 0"sv, ""sv);
    }

    SECTION("with operator ||") {
        int         value1       = 2;
        int         value2       = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 == 1 || value2 == 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 2);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK", "value1 == 1 || value2 == 0"sv, ""sv);
    }

    SECTION("with operator =") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value = 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value = 0"sv, ""sv);
    }

    SECTION("with operator +=") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value += -1); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value += -1"sv, ""sv);
    }

    SECTION("with operator -=") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value -= 1); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value -= 1"sv, ""sv);
    }

    SECTION("with operator *=") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value *= 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value *= 0"sv, ""sv);
    }

    SECTION("with operator /=") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value /= 10); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value /= 10"sv, ""sv);
    }

    SECTION("with operator ^") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value ^ 1); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value ^ 1"sv, ""sv);
    }

    SECTION("with operator &") {
        int         value        = 1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value & 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value & 0"sv, ""sv);
    }

    SECTION("with operator |") {
        int         value        = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value | 0); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value | 0"sv, ""sv);
    }

    SECTION("with multiple comparisons") {
        int         value1       = 2;
        int         value2       = 1;
        bool        value3       = true;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 == value2 == value3); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 2);
        CHECK(value2 == 1);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 == value2 == value3"sv, ""sv);
    }

    SECTION("with final ^") {
        int         value1       = 2;
        int         value2       = 1;
        bool        value3       = false;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 == value2 ^ value3); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 2);
        CHECK(value2 == 1);
        CHECK(value3 == false);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "value1 == value2 ^ value3"sv, ""sv);
    }

    SECTION("with two final ^") {
        int         value1       = 2;
        int         value2       = 1;
        bool        value3       = false;
        bool        value4       = false;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(value1 == value2 ^ value3 ^ value4); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 2);
        CHECK(value2 == 1);
        CHECK(value3 == false);
        CHECK(value4 == false);
        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv, "value1 == value2 ^ value3 ^ value4"sv, ""sv);
    }

    SECTION("with operator , (int,int)") {
        int         value1       = 1;
        int         value2       = -1;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(++value1, ++value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 2);
        CHECK(value2 == 0);
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "++value1, ++value2"sv, ""sv);
    }
}

SNITCH_WARNING_POP

TEST_CASE("check false", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("binary pass") {
        int value1 = 1;
        int value2 = 0;

        {
            test_override override(catcher);
            SNITCH_CHECK_FALSE(value1 < value2);
        }

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("binary fail") {
        int         value1       = 1;
        int         value2       = 0;
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK_FALSE(value1 >= value2); failure_line = __LINE__;
            // clang-format on
        }

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK_FALSE"sv, "value1 >= value2"sv, "1 >= 0"sv);
    }

    SECTION("matcher pass") {
        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK_FALSE("hello"sv != snitch::matchers::contains_substring{"lo"});
            // clang-format on
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("matcher fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK_FALSE("hello"sv == snitch::matchers::contains_substring{"lo"}); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK_FALSE",
            "\"hello\"sv == snitch::matchers::contains_substring{\"lo\"}"sv,
            "found 'lo' in 'hello'"sv);
    }
}

namespace snitch::matchers {
struct is_even {
    // Some silly state; to make sure we support this.
    int remainder = -1;

    constexpr bool match(int i) {
        remainder = i % 2;
        return remainder == 0;
    }

    constexpr snitch::small_string<snitch::max_message_length>
    describe_match(int i, match_status status) {
        small_string<max_message_length> description_buffer;
        append_or_truncate(
            description_buffer, "input value ", i, " ",
            (status == match_status::matched ? "is" : "is not"), " even; remainder: ", remainder);
        return description_buffer;
    }
};
} // namespace snitch::matchers

TEST_CASE("check that", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CHECK_THAT(i, snitch::matchers::is_even{});
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CHECK_THAT(i, snitch::matchers::is_even{}); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK_THAT"sv, "i, snitch::matchers::is_even{}"sv,
            "input value 9 is not even; remainder: 1"sv);
    }
}

TEST_CASE("check misc", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("out of space unary") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(unary_long_string{}); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK"sv, "unary_long_string{}"sv, ""sv);
    }

    SECTION("out of space binary lhs") {
        constexpr std::size_t                     large_string_length = snitch::max_expr_length * 2;
        snitch::small_string<large_string_length> string1;
        snitch::small_string<large_string_length> string2;

        string1.resize(large_string_length);
        string2.resize(large_string_length);
        std::fill(string1.begin(), string1.end(), '0');
        std::fill(string2.begin(), string2.end(), '1');

        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(string1.str() == string2.str()); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv, "string1.str() == string2.str()"sv, ""sv);
    }

    SECTION("out of space binary rhs") {
        constexpr std::size_t large_string_length = snitch::max_expr_length * 3 / 2;
        snitch::small_string<large_string_length> string1;
        snitch::small_string<large_string_length> string2;

        string1.resize(large_string_length);
        string2.resize(large_string_length);
        std::fill(string1.begin(), string1.end(), '0');
        std::fill(string2.begin(), string2.end(), '1');

        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(string1.str() == string2.str()); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv, "string1.str() == string2.str()"sv, ""sv);
    }

    SECTION("out of space binary op") {
        constexpr std::size_t                     large_string_length = snitch::max_expr_length - 2;
        snitch::small_string<large_string_length> string1;
        snitch::small_string<large_string_length> string2;

        string1.resize(large_string_length);
        string2.resize(large_string_length);
        std::fill(string1.begin(), string1.end(), '0');
        std::fill(string2.begin(), string2.end(), '1');

        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(string1.str() == string2.str()); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv, "string1.str() == string2.str()"sv, ""sv);
    }

    SECTION("non copiable non movable pass") {
        {
            test_override override(catcher);
            SNITCH_CHECK(non_relocatable(1) != non_relocatable(2));
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("non copiable non movable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(non_relocatable(1) == non_relocatable(2)); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv, "non_relocatable(1) == non_relocatable(2)"sv,
            "non_relocatable{1} != non_relocatable{2}"sv);
    }

    SECTION("non appendable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(non_appendable(1) == non_appendable(2)); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv, "non_appendable(1) == non_appendable(2)", "? != ?"sv);
    }

    SECTION("matcher fail lhs") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(snitch::matchers::long_matcher_always_fails{} == "hello"sv); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv,
            "snitch::matchers::long_matcher_always_fails{} == \"hello\"sv"sv, ""sv);
    }

    SECTION("matcher fail rhs") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK("hello"sv == snitch::matchers::long_matcher_always_fails{}); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv,
            "\"hello\"sv == snitch::matchers::long_matcher_always_fails{}"sv, ""sv);
    }

    SECTION("out of space matcher lhs") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK(snitch::matchers::contains_substring{"foo"} == "hello"sv); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv,
            "snitch::matchers::contains_substring{\"foo\"} == \"hello\"sv"sv,
            "could not find 'foo' in 'hello'"sv);
    }

    SECTION("out of space matcher rhs") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CHECK("hello"sv == snitch::matchers::contains_substring{"foo"}); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CHECK"sv,
            "\"hello\"sv == snitch::matchers::contains_substring{\"foo\"}"sv,
            "could not find 'foo' in 'hello'"sv);
    }
}

TEST_CASE("consteval check", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CONSTEVAL_CHECK(i == 10);
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEVAL_CHECK(i == 10); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(catcher, failure_line, "CONSTEVAL_CHECK"sv, "i == 10"sv, "9 != 10"sv);
    }

    SECTION("not decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 9;
            SNITCH_CONSTEVAL_CHECK(i == 10 || i == 9);
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("not decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEVAL_CHECK(i == 10 || i == 8); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(catcher, failure_line, "CONSTEVAL_CHECK"sv, "i == 10 || i == 8"sv, ""sv);
    }

    // This triggers a compile-time error, so we can't enable it.
    // But we keep it here to monitor that the error message being displayed is reasonable.

#if 0
    SECTION("not constexpr") {
        struct not_fully_constexpr {
            bool foo() {
                std::printf("runtime stuff\n");
                return true;
            }
        };

        constexpr not_fully_constexpr c{};
        SNITCH_CONSTEVAL_CHECK(not_fully_constexpr{c}.foo());
    }
#endif
}

TEST_CASE("consteval check false", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CONSTEVAL_CHECK_FALSE(i == 9);
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEVAL_CHECK_FALSE(i == 9); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CONSTEVAL_CHECK_FALSE"sv, "i == 9"sv, "9 == 9"sv);
    }

    SECTION("not decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 9;
            SNITCH_CONSTEVAL_CHECK_FALSE(i == 10 || i == 8);
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("not decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEVAL_CHECK_FALSE(i == 10 || i == 9); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CONSTEVAL_CHECK_FALSE"sv, "i == 10 || i == 9"sv, ""sv);
    }
}

TEST_CASE("consteval check that", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CONSTEVAL_CHECK_THAT(i, snitch::matchers::is_even{});
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEVAL_CHECK_THAT(i, snitch::matchers::is_even{}); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "CONSTEVAL_CHECK_THAT"sv, "i, snitch::matchers::is_even{}"sv,
            "input value 9 is not even; remainder: 1"sv);
    }
}

TEST_CASE("constexpr check", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CONSTEXPR_CHECK(i == 10);
        }

        CONSTEXPR_CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEXPR_CHECK(i == 10); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE_2(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[0u], failure_line, "CONSTEXPR_CHECK[compile-time]"sv,
            "i == 10"sv, "9 != 10"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line, "CONSTEXPR_CHECK[run-time]"sv, "i == 10"sv,
            "9 != 10"sv);
    }

    SECTION("not decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 9;
            SNITCH_CONSTEXPR_CHECK(i == 10 || i == 9);
        }

        CONSTEXPR_CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("not decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEXPR_CHECK(i == 10 || i == 8); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE_2(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[0u], failure_line, "CONSTEXPR_CHECK[compile-time]"sv,
            "i == 10 || i == 8"sv, ""sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line, "CONSTEXPR_CHECK[run-time]"sv,
            "i == 10 || i == 8"sv, ""sv);
    }

    SECTION("compile-time failure only") {
        std::size_t failure_line = 0u;

        struct compile_time_bug {
            constexpr bool foo() {
                if (std::is_constant_evaluated()) {
                    return false;
                } else {
                    return true;
                }
            }
        };

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CONSTEXPR_CHECK(compile_time_bug{}.foo()); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[0u], failure_line, "CONSTEXPR_CHECK[compile-time]"sv,
            "compile_time_bug{}.foo()", "false"sv);
    }

    SECTION("run-time failure only") {
        std::size_t failure_line = 0u;

        struct compile_time_bug {
            constexpr bool foo() {
                if (std::is_constant_evaluated()) {
                    return true;
                } else {
                    return false;
                }
            }
        };

        {
            test_override override(catcher);
            // clang-format off
            SNITCH_CONSTEXPR_CHECK(compile_time_bug{}.foo()); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line, "CONSTEXPR_CHECK[run-time]"sv,
            "compile_time_bug{}.foo()"sv, "false"sv);
    }

    // This triggers a compile-time error, so we can't enable it.
    // But we keep it here to monitor that the error message being displayed is reasonable.

#if 0
    SECTION("not constexpr") {
        struct not_fully_constexpr {
            bool foo() {
                std::printf("runtime stuff\n");
                return true;
            }
        };

        constexpr not_fully_constexpr c{};
        SNITCH_CONSTEXPR_CHECK(not_fully_constexpr{c}.foo());
    }
#endif
}

TEST_CASE("constexpr check false", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CONSTEXPR_CHECK_FALSE(i == 9);
        }

        CONSTEXPR_CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEXPR_CHECK_FALSE(i == 9); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE_2(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[0u], failure_line, "CONSTEXPR_CHECK_FALSE[compile-time]"sv,
            "i == 9"sv, "9 == 9"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line, "CONSTEXPR_CHECK_FALSE[run-time]"sv,
            "i == 9"sv, "9 == 9"sv);
    }

    SECTION("not decomposable pass") {
        {
            test_override override(catcher);
            constexpr int i = 9;
            SNITCH_CONSTEXPR_CHECK_FALSE(i == 10 || i == 8);
        }

        CONSTEXPR_CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("not decomposable fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEXPR_CHECK_FALSE(i == 10 || i == 9); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE_2(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[0u], failure_line, "CONSTEXPR_CHECK_FALSE[compile-time]"sv,
            "i == 10 || i == 9"sv, ""sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line, "CONSTEXPR_CHECK_FALSE[run-time]"sv,
            "i == 10 || i == 9"sv, ""sv);
    }
}

TEST_CASE("constexpr check that", "[test macros]") {
    event_catcher<2> catcher;

    SECTION("pass") {
        {
            test_override override(catcher);
            constexpr int i = 10;
            SNITCH_CONSTEXPR_CHECK_THAT(i, snitch::matchers::is_even{});
        }

        CONSTEXPR_CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("fail") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            constexpr int i = 9;
            // clang-format off
            SNITCH_CONSTEXPR_CHECK_THAT(i, snitch::matchers::is_even{}); failure_line = __LINE__;
            // clang-format on
        }

        CONSTEXPR_CHECK_EXPR_FAILURE_2(catcher);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[0u], failure_line, "CONSTEXPR_CHECK_THAT[compile-time]"sv,
            "i, snitch::matchers::is_even{}"sv, "input value 9 is not even; remainder: 1"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line, "CONSTEXPR_CHECK_THAT[run-time]"sv,
            "i, snitch::matchers::is_even{}"sv, "input value 9 is not even; remainder: 1"sv);
    }
}

#if SNITCH_WITH_EXCEPTIONS
struct my_exception : public std::exception {
    const char* what() const noexcept override {
        return "exception1";
    }
};

struct my_other_exception : public std::exception {
    const char* what() const noexcept override {
        return "exception2";
    }
};

TEST_CASE("check throws as", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("pass") {
        {
            test_override override(catcher);
            auto          fun = []() { throw my_exception{}; };
            SNITCH_CHECK_THROWS_AS(fun(), my_exception);
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("fail no exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun = []() {};
            // clang-format off
            SNITCH_CHECK_THROWS_AS(fun(), my_exception); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "my_exception expected but no exception thrown"sv);
    }

    SECTION("fail other std::exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun = []() { throw my_other_exception{}; };
            // clang-format off
            SNITCH_CHECK_THROWS_AS(fun(), my_exception); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line,
            "my_exception expected but other std::exception thrown; message: exception2"sv);
    }

    SECTION("fail unknown exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun = []() { throw 1; };
            // clang-format off
            SNITCH_CHECK_THROWS_AS(fun(), my_exception); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "my_exception expected but other unknown exception thrown"sv);
    }
}

TEST_CASE("require throws as", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("fail no exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun = []() {};
            try {
                // clang-format off
                failure_line = __LINE__; SNITCH_REQUIRE_THROWS_AS(fun(), my_exception);
                // clang-format on
            } catch (const snitch::impl::abort_exception&) {
                // Ignore the abort signal.
            }
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "my_exception expected but no exception thrown"sv);
    }
}

TEST_CASE("check throws matches", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("pass") {
        {
            test_override override(catcher);
            auto          fun     = []() { throw my_exception{}; };
            auto          matcher = snitch::matchers::with_what_contains{"exception1"};
            SNITCH_CHECK_THROWS_MATCHES(fun(), my_exception, matcher);
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("fail no exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun     = []() {};
            auto          matcher = snitch::matchers::with_what_contains{"exception"};
            // clang-format off
            failure_line = __LINE__; SNITCH_CHECK_THROWS_MATCHES(fun(), my_exception, matcher);
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "my_exception expected but no exception thrown"sv);
    }

    SECTION("fail other std::exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun     = []() { throw my_other_exception{}; };
            auto          matcher = snitch::matchers::with_what_contains{"exception1"};
            // clang-format off
            SNITCH_CHECK_THROWS_MATCHES(fun(), my_exception, matcher); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line,
            "my_exception expected but other std::exception thrown; message: exception2"sv);
    }

    SECTION("fail unknown exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun     = []() { throw 1; };
            auto          matcher = snitch::matchers::with_what_contains{"exception1"};
            // clang-format off
            SNITCH_CHECK_THROWS_MATCHES(fun(), my_exception, matcher); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "my_exception expected but other unknown exception thrown"sv);
    }

    SECTION("fail not a match") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun     = []() { throw my_other_exception{}; };
            auto          matcher = snitch::matchers::with_what_contains{"exception1"};
            // clang-format off
            SNITCH_CHECK_THROWS_MATCHES(fun(), std::exception, matcher); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line,
            "could not match caught std::exception with expected content: could not find 'exception1' in 'exception2'"sv);
    }
}

TEST_CASE("require throws matches", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("fail no exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun     = []() {};
            auto          matcher = snitch::matchers::with_what_contains{"exception"};
            try {
                // clang-format off
                failure_line = __LINE__; SNITCH_REQUIRE_THROWS_MATCHES(fun(), my_exception, matcher);
                // clang-format on
            } catch (const snitch::impl::abort_exception&) {
                // Ignore the abort signal.
            }
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line, "my_exception expected but no exception thrown"sv);
    }
}

namespace {
[[nodiscard]] int nodiscard_function() {
    return 1;
}
} // namespace

TEST_CASE("check nothrow", "[test macros]") {
    event_catcher<1> catcher;

    SECTION("pass void") {
        {
            test_override override(catcher);
            auto          fun = [] {};
            SNITCH_CHECK_NOTHROW(fun());
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("pass int [[nodiscard]]") {
        {
            test_override override(catcher);
            SNITCH_CHECK_NOTHROW(nodiscard_function());
        }

        CHECK_EXPR_SUCCESS(catcher);
    }

    SECTION("fail std::exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun = []() { throw my_exception{}; };
            // clang-format off
            SNITCH_CHECK_NOTHROW(fun()); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line,
            "expected fun() not to throw but it threw a std::exception; message: exception1"sv);
    }

    SECTION("fail other exception") {
        std::size_t failure_line = 0u;

        {
            test_override override(catcher);
            auto          fun = []() { throw 1; };
            // clang-format off
            SNITCH_CHECK_NOTHROW(fun()); failure_line = __LINE__;
            // clang-format on
        }

        CHECK_EXPR_FAILURE(
            catcher, failure_line,
            "expected fun() not to throw but it threw an unknown exception"sv);
    }
}

namespace {
std::size_t test_check_line   = 0u;
std::size_t test_section_line = 0u;

int throw_unexpectedly() {
    throw std::runtime_error("bad function");
}
} // namespace

#    define CHECK_UNHANDLED_EXCEPTION(CATCHER, LINE, MESSAGE)                                      \
        do {                                                                                       \
            auto e = get_failure_event((CATCHER).events);                                          \
            REQUIRE(e.has_value());                                                                \
            CHECK(e.value().location.line == LINE);                                                \
            auto m = std::get_if<std::string_view>(&e.value().data);                               \
            REQUIRE(m != nullptr);                                                                 \
            CHECK(*m == MESSAGE);                                                                  \
        } while (0)

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("unhandled exceptions", "[test macros]") {
    event_catcher<7> catcher;

    test_check_line   = 0u;
    test_section_line = 0u;

    SECTION("throw in check") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            test_check_line = __LINE__; SNITCH_CHECK(throw_unexpectedly() == 1);
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, test_check_line, "unexpected std::exception caught; message: bad function"sv);
    }

    SECTION("throw in section") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            test_section_line = __LINE__; SNITCH_SECTION("section 1") {
                throw_unexpectedly();
            }
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, test_section_line,
            "unexpected std::exception caught; message: bad function"sv);
    }

    SECTION("throw in other section") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            SNITCH_SECTION("section 1") {
                // Nothing.
            }
            test_section_line = __LINE__; SNITCH_SECTION("section 2") {
                throw_unexpectedly();
            }
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, test_section_line,
            "unexpected std::exception caught; message: bad function"sv);
    }

    SECTION("throw in nested section") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            SNITCH_SECTION("section 1") {
                test_section_line = __LINE__; SNITCH_SECTION("section 2") {
                    throw_unexpectedly();
                }
            }
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, test_section_line,
            "unexpected std::exception caught; message: bad function"sv);
    }

    SECTION("throw in check in section") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            test_section_line = __LINE__; SNITCH_SECTION("section 1") {
                test_check_line = __LINE__; SNITCH_CHECK(throw_unexpectedly() == 1);
            }
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, test_check_line, "unexpected std::exception caught; message: bad function"sv);
    }

    SECTION("throw in body") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            throw_unexpectedly();
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, catcher.mock_case.location.line,
            "unexpected std::exception caught; message: bad function"sv);
    }

    SECTION("throw in body after section") {
        // clang-format off
        catcher.mock_case.location.line = __LINE__;
        catcher.mock_case.func = []() {
            SNITCH_SECTION("section 1") {
                // Nothing.
            }
            throw_unexpectedly();
        };
        // clang-format on

        catcher.run_test();

        CHECK_UNHANDLED_EXCEPTION(
            catcher, catcher.mock_case.location.line,
            "unexpected std::exception caught; message: bad function"sv);
    }
}

SNITCH_WARNING_POP

#endif
