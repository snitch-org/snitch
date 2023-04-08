#include "testing.hpp"
#include "testing_event.hpp"

#include <algorithm>
#include <compare>

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

template<std::size_t MaxEvents>
struct event_catcher {
    snitch::registry mock_registry;

    snitch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snitch::impl::test_case_state::not_run};

    snitch::impl::test_state mock_test{.reg = mock_registry, .test = mock_case};

    snitch::small_vector<event_deep_copy, MaxEvents> events;

    event_catcher() {
        mock_registry.report_callback = {*this, snitch::constant<&event_catcher::report>{}};
    }

    void report(const snitch::registry&, const snitch::event::data& e) noexcept {
        events.push_back(deep_copy(e));
    }
};

struct test_override {
    snitch::impl::test_state* previous;

    template<std::size_t N>
    explicit test_override(event_catcher<N>& catcher) :
        previous(snitch::impl::try_get_current_test()) {
        snitch::impl::set_current_test(&catcher.mock_test);
    }

    ~test_override() {
        snitch::impl::set_current_test(previous);
    }
};
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

#define CHECK_EXPR_SUCCESS(CATCHER)                                                                \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 1u);                                                  \
        CHECK((CATCHER).events.empty());                                                           \
    } while (0)

#define CHECK_EVENT_FAILURE(CATCHER, EVENT, FAILURE_LINE, MESSAGE)                                 \
    do {                                                                                           \
        CHECK((EVENT).event_type == event_deep_copy::type::assertion_failed);                      \
        CHECK_EVENT_TEST_ID((EVENT), (CATCHER).mock_case.id);                                      \
        CHECK_EVENT_LOCATION((EVENT), __FILE__, (FAILURE_LINE));                                   \
        CHECK((EVENT).message == (MESSAGE));                                                       \
    } while (0)

#define CHECK_EXPR_FAILURE(CATCHER, FAILURE_LINE, MESSAGE)                                         \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 1u);                                                  \
        REQUIRE((CATCHER).events.size() == 1u);                                                    \
        CHECK_EVENT_FAILURE(CATCHER, (CATCHER).events[0], FAILURE_LINE, MESSAGE);                  \
    } while (0)

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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value), got false"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(!value), got false"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value++), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(2 * value), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(2 / value), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(2 + value), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(2 - value), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(2 % value), got 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 == value2), got 0 != 1"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 != value2), got 0 == 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 < value2), got 1 >= 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 > value2), got 0 <= 1"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 <= value2), got 1 > 0"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 >= value2), got 0 < 1"sv);
    }
}

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_PRECEDENCE
SNITCH_WARNING_DISABLE_ASSIGNMENT

TEST_CASE("check no decomposition", "[test macros]") {
    event_catcher<1> catcher;

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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 <=> value2 != 0)"sv);
    }

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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 == 1 && value2 == 0)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 == 1 || value2 == 0)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value = 0)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value += -1)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value -= 1)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value *= 0)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value /= 10)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value ^ 1)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value & 0)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value | 0)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 == value2 == value3)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 == value2 ^ value3)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(value1 == value2 ^ value3 ^ value4)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(++value1, ++value2)"sv);
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
        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK_FALSE(value1 >= value2), got 1 >= 0"sv);
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
            catcher, failure_line,
            "CHECK_FALSE(\"hello\"sv == snitch::matchers::contains_substring{\"lo\"}), got found 'lo' in 'hello'"sv);
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
            catcher, failure_line,
            "CHECK_THAT(i, snitch::matchers::is_even{}), got input value 9 is not even; remainder: 1"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(unary_long_string{})"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(string1.str() == string2.str())"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(string1.str() == string2.str())"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CHECK(string1.str() == string2.str())"sv);
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
            catcher, failure_line,
            "CHECK(non_relocatable(1) == non_relocatable(2)), got non_relocatable{1} != non_relocatable{2}"sv);
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
            catcher, failure_line, "CHECK(non_appendable(1) == non_appendable(2)), got ? != ?"sv);
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
            catcher, failure_line,
            "CHECK(snitch::matchers::long_matcher_always_fails{} == \"hello\"sv)"sv);
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
            catcher, failure_line,
            "CHECK(\"hello\"sv == snitch::matchers::long_matcher_always_fails{})"sv);
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
            catcher, failure_line,
            "CHECK(snitch::matchers::contains_substring{\"foo\"} == \"hello\"sv), got could not find 'foo' in 'hello'"sv);
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
            catcher, failure_line,
            "CHECK(\"hello\"sv == snitch::matchers::contains_substring{\"foo\"}), got could not find 'foo' in 'hello'"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CONSTEVAL_CHECK(i == 10), got 9 != 10"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CONSTEVAL_CHECK(i == 10 || i == 8)"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CONSTEVAL_CHECK_FALSE(i == 9), got 9 == 9"sv);
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

        CHECK_EXPR_FAILURE(catcher, failure_line, "CONSTEVAL_CHECK_FALSE(i == 10 || i == 9)"sv);
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
            catcher, failure_line,
            "CONSTEVAL_CHECK_THAT(i, snitch::matchers::is_even{}), got input value 9 is not even; remainder: 1"sv);
    }
}

#define CONSTEXPR_CHECK_EXPR_SUCCESS(CATCHER)                                                      \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        CHECK((CATCHER).events.empty());                                                           \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_FAILURE(CATCHER)                                                      \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 1u);                                                    \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_FAILURE_2(CATCHER)                                                    \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
    } while (0)

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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK[compile-time](i == 10), got 9 != 10"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line,
            "CONSTEXPR_CHECK[run-time](i == 10), got 9 != 10"sv);
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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK[compile-time](i == 10 || i == 8)"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line,
            "CONSTEXPR_CHECK[run-time](i == 10 || i == 8)"sv);
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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK[compile-time](compile_time_bug{}.foo()), got false"sv);
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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK[run-time](compile_time_bug{}.foo()), got false"sv);
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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK_FALSE[compile-time](i == 9), got 9 == 9"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line,
            "CONSTEXPR_CHECK_FALSE[run-time](i == 9), got 9 == 9"sv);
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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK_FALSE[compile-time](i == 10 || i == 9)"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line,
            "CONSTEXPR_CHECK_FALSE[run-time](i == 10 || i == 9)"sv);
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
            catcher, catcher.events[0u], failure_line,
            "CONSTEXPR_CHECK_THAT[compile-time](i, snitch::matchers::is_even{}), got input value 9 is not even; remainder: 1"sv);
        CHECK_EVENT_FAILURE(
            catcher, catcher.events[1u], failure_line,
            "CONSTEXPR_CHECK_THAT[run-time](i, snitch::matchers::is_even{}), got input value 9 is not even; remainder: 1"sv);
    }
}
