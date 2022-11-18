#include "testing.hpp"

#include <algorithm>

using namespace std::literals;

struct non_relocatable {
    int value = 0;

    explicit non_relocatable(int v) : value(v) {}
    non_relocatable(const non_relocatable&) = delete;
    non_relocatable(non_relocatable&&)      = delete;
    non_relocatable& operator=(const non_relocatable&) = delete;
    non_relocatable& operator=(non_relocatable&&) = delete;
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

bool append(snatch::small_string_span ss, const non_relocatable& o) noexcept {
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
    snatch::small_string<2048> value;

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

bool append(snatch::small_string_span ss, const unary_long_string& u) noexcept {
    return append(ss, u.value);
}

struct event_deep_copy {
    enum class type { unknown, assertion_failed };

    type event_type = type::unknown;

    snatch::small_string<snatch::max_test_name_length> test_id_name;
    snatch::small_string<snatch::max_test_name_length> test_id_tags;
    snatch::small_string<snatch::max_test_name_length> test_id_type;

    snatch::small_string<snatch::max_message_length> location_file;
    std::size_t                                      location_line = 0u;

    snatch::small_string<snatch::max_message_length> message;
};

namespace snatch::matchers {
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
} // namespace snatch::matchers

event_deep_copy deep_copy(const snatch::event::data& e) {
    return std::visit(
        snatch::overload{
            [](const snatch::event::assertion_failed& a) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::assertion_failed;
                append_or_truncate(c.test_id_name, a.id.name);
                append_or_truncate(c.test_id_tags, a.id.tags);
                append_or_truncate(c.test_id_type, a.id.type);
                append_or_truncate(c.location_file, a.location.file);
                c.location_line = a.location.line;
                append_or_truncate(c.message, a.message);
                return c;
            },
            [](const auto&) -> event_deep_copy { snatch::terminate_with("event not handled"); }},
        e);
}

#define CHECK_EVENT_TEST_ID(ACTUAL, EXPECTED)                                                      \
    CHECK(ACTUAL.test_id_name == EXPECTED.name);                                                   \
    CHECK(ACTUAL.test_id_tags == EXPECTED.tags);                                                   \
    CHECK(ACTUAL.test_id_type == EXPECTED.type)

#define CHECK_EVENT_LOCATION(ACTUAL, FILE, LINE)                                                   \
    CHECK(ACTUAL.location_file == std::string_view(FILE));                                         \
    CHECK(ACTUAL.location_line == LINE)

TEST_CASE("check unary", "[test macros]") {
    snatch::registry mock_registry;

    snatch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snatch::impl::test_state::not_run};

    snatch::impl::test_run mock_run {
        .reg = mock_registry, .test = mock_case, .sections = {}, .captures = {}, .asserts = 0,
#if SNATCH_WITH_TIMINGS
        .duration = 0.0f
#endif
    };

    std::optional<event_deep_copy> last_event;
    auto report = [&](const snatch::registry&, const snatch::event::data& e) noexcept {
        last_event.emplace(deep_copy(e));
    };

    mock_registry.report_callback = report;

    SECTION("bool true") {
        bool value = true;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == true);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("bool false") {
        bool value = false;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == false);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value), got false"sv);
    }

    SECTION("bool !true") {
        bool value = true;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(!value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == true);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(!value), got false"sv);
    }

    SECTION("bool !false") {
        bool value = false;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(!value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == false);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer non-zero") {
        int value = 5;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 5);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer zero") {
        int value = 0;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == 0);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value), got 0"sv);
    }

    SECTION("integer pre increment") {
        int value = 0;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(++value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer post increment") {
        int value = 0;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value++); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == 1);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value++), got 0"sv);
    }

    SECTION("integer expression * pass") {
        int value = 1;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(2 * value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer expression / pass") {
        int value = 1;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(2 / value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer expression + pass") {
        int value = 1;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(2 + value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer expression - pass") {
        int value = 3;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(2 - value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 3);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer expression % pass") {
        int value = 3;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(2 % value);
#undef SNATCH_CURRENT_TEST

        CHECK(value == 3);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer expression * fail") {
        int value = 0;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(2 * value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == 0);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(2 * value), got 0"sv);
    }

    SECTION("integer expression / fail") {
        int value = 5;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(2 / value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == 5);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(2 / value), got 0"sv);
    }

    SECTION("integer expression + fail") {
        int value = -2;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(2 + value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == -2);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(2 + value), got 0"sv);
    }

    SECTION("integer expression - fail") {
        int value = 2;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(2 - value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == 2);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(2 - value), got 0"sv);
    }

    SECTION("integer expression % fail") {
        int value = 1;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(2 % value); const std::size_t failure_line = __LINE__;
        // clang-format on
#undef SNATCH_CURRENT_TEST

        CHECK(value == 1);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(2 % value), got 0"sv);
    }
};

TEST_CASE("check binary", "[test macros]") {
    snatch::registry mock_registry;

    snatch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snatch::impl::test_state::not_run};

    snatch::impl::test_run mock_run {
        .reg = mock_registry, .test = mock_case, .sections = {}, .captures = {}, .asserts = 0,
#if SNATCH_WITH_TIMINGS
        .duration = 0.0f
#endif
    };

    std::optional<event_deep_copy> last_event;
    auto report = [&](const snatch::registry&, const snatch::event::data& e) noexcept {
        last_event.emplace(deep_copy(e));
    };

    mock_registry.report_callback = report;

    SECTION("integer == pass") {
        int value1 = 0;
        int value2 = 0;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value1 == value2);
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 0);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer != pass") {
        int value1 = 0;
        int value2 = 1;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value1 != value2);
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer < pass") {
        int value1 = 0;
        int value2 = 1;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value1 < value2);
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer > pass") {
        int value1 = 1;
        int value2 = 0;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value1 > value2);
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer <= pass") {
        int value1 = 0;
        int value2 = 1;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value1 <= value2);
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer >= pass") {
        int value1 = 1;
        int value2 = 0;

#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(value1 >= value2);
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("integer == fail") {
        int value1 = 0;
        int value2 = 1;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value1 == value2); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value1 == value2), got 0 != 1"sv);
    }

    SECTION("integer != fail") {
        int value1 = 0;
        int value2 = 0;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value1 != value2); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 0);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value1 != value2), got 0 == 0"sv);
    }

    SECTION("integer < fail") {
        int value1 = 1;
        int value2 = 0;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value1 < value2); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value1 < value2), got 1 >= 0"sv);
    }

    SECTION("integer > fail") {
        int value1 = 0;
        int value2 = 1;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value1 > value2); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value1 > value2), got 0 <= 1"sv);
    }

    SECTION("integer <= fail") {
        int value1 = 1;
        int value2 = 0;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value1 <= value2); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 1);
        CHECK(value2 == 0);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value1 <= value2), got 1 > 0"sv);
    }

    SECTION("integer >= fail") {
        int value1 = 0;
        int value2 = 1;

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(value1 >= value2); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(value1 == 0);
        CHECK(value2 == 1);
        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(value1 >= value2), got 0 < 1"sv);
    }
};

TEST_CASE("check misc", "[test macros]") {
    snatch::registry mock_registry;

    snatch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snatch::impl::test_state::not_run};

    snatch::impl::test_run mock_run {
        .reg = mock_registry, .test = mock_case, .sections = {}, .captures = {}, .asserts = 0,
#if SNATCH_WITH_TIMINGS
        .duration = 0.0f
#endif
    };

    std::optional<event_deep_copy> last_event;
    auto report = [&](const snatch::registry&, const snatch::event::data& e) noexcept {
        last_event.emplace(deep_copy(e));
    };

    mock_registry.report_callback = report;

    SECTION("out of space unary") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(unary_long_string{}); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(unary_long_string{})"sv);
    }

    SECTION("out of space binary lhs") {
        constexpr std::size_t large_string_length = snatch::max_expr_length*2;
        snatch::small_string<large_string_length> string1;
        snatch::small_string<large_string_length> string2;

        string1.resize(large_string_length);
        string2.resize(large_string_length);
        std::fill(string1.begin(), string1.end(), '0');
        std::fill(string2.begin(), string2.end(), '1');

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(string1.str() == string2.str()); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(string1.str() == string2.str())"sv);
    }

    SECTION("out of space binary rhs") {
        constexpr std::size_t large_string_length = snatch::max_expr_length*3/2;
        snatch::small_string<large_string_length> string1;
        snatch::small_string<large_string_length> string2;

        string1.resize(large_string_length);
        string2.resize(large_string_length);
        std::fill(string1.begin(), string1.end(), '0');
        std::fill(string2.begin(), string2.end(), '1');

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(string1.str() == string2.str()); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(string1.str() == string2.str())"sv);
    }

    SECTION("out of space binary op") {
        constexpr std::size_t large_string_length = snatch::max_expr_length - 2;
        snatch::small_string<large_string_length> string1;
        snatch::small_string<large_string_length> string2;

        string1.resize(large_string_length);
        string2.resize(large_string_length);
        std::fill(string1.begin(), string1.end(), '0');
        std::fill(string2.begin(), string2.end(), '1');

#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(string1.str() == string2.str()); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(string1.str() == string2.str())"sv);
    }

    SECTION("non copiable non movable pass") {
#define SNATCH_CURRENT_TEST mock_run
        SNATCH_CHECK(non_relocatable(1) != non_relocatable(2));
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);
        CHECK(!last_event.has_value());
    }

    SECTION("non copiable non movable fail") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(non_relocatable(1) == non_relocatable(2)); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(non_relocatable(1) == non_relocatable(2)), got non_relocatable{1} != non_relocatable{2}"sv);
    }

    SECTION("non appendable fail") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(non_appendable(1) == non_appendable(2)); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(non_appendable(1) == non_appendable(2)), got ? != ?"sv);
    }

    SECTION("matcher fail lhs") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(snatch::matchers::long_matcher_always_fails{} == "hello"sv); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(snatch::matchers::long_matcher_always_fails{} == \"hello\"sv)"sv);
    }

    SECTION("matcher fail rhs") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK("hello"sv == snatch::matchers::long_matcher_always_fails{}); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(\"hello\"sv == snatch::matchers::long_matcher_always_fails{})"sv);
    }

    SECTION("out of space matcher lhs") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK(snatch::matchers::contains_substring{"foo"} == "hello"sv); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(snatch::matchers::contains_substring{\"foo\"} == \"hello\"sv), got could not find 'foo' in 'hello'"sv);
    }

    SECTION("out of space matcher rhs") {
#define SNATCH_CURRENT_TEST mock_run
        // clang-format off
        SNATCH_CHECK("hello"sv == snatch::matchers::contains_substring{"foo"}); const std::size_t failure_line = __LINE__;
        // clang-foramt on
#undef SNATCH_CURRENT_TEST

        CHECK(mock_run.asserts == 1u);

        REQUIRE(last_event.has_value());
        const auto& event = last_event.value();
        CHECK(event.event_type == event_deep_copy::type::assertion_failed);

        CHECK_EVENT_TEST_ID(event, mock_case.id);
        CHECK_EVENT_LOCATION(event, __FILE__, failure_line);
        CHECK(event.message == "CHECK(\"hello\"sv == snatch::matchers::contains_substring{\"foo\"}), got could not find 'foo' in 'hello'"sv);
    }
};
