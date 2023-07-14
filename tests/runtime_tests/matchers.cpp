#include "testing.hpp"

#include <cmath>
#include <stdexcept>

using namespace std::literals;

namespace snitch::matchers {
struct has_prefix {
    std::string_view prefix;

    bool match(std::string_view s) const noexcept {
        return s.starts_with(prefix) && s.size() >= prefix.size() + 1 && s[prefix.size()] == ':';
    }

    small_string<max_message_length>
    describe_match(std::string_view s, match_status status) const noexcept {
        small_string<max_message_length> message;
        append_or_truncate(
            message, status == match_status::matched ? "found" : "could not find", " prefix '",
            prefix, ":' in '", s, "'");

        if (status == match_status::failed) {
            if (auto pos = s.find_first_of(':'); pos != s.npos) {
                append_or_truncate(message, "; found prefix '", s.substr(0, pos), ":'");
            } else {
                append_or_truncate(message, "; no prefix found");
            }
        }

        return message;
    }
};

template<floating_point T>
T get_abs_tolerance(T, T abs_tol) {
    return abs_tol;
}

template<typename T>
concept tolerance_function = requires(const T& func, double v) {
                                 { func(v) } -> std::convertible_to<double>;
                             };

template<floating_point T, tolerance_function F>
T get_abs_tolerance(T value, const F& func) {
    return func(value);
}

template<floating_point T, typename Tolerance>
struct within {
    T         expected  = {};
    Tolerance tolerance = {};

    mutable struct {
        T magnitude     = {};
        T abs_diff      = {};
        T abs_tolerance = {};
    } state = {};

    explicit within(T e, const Tolerance& tol) : expected{e}, tolerance{tol} {}

    constexpr bool match(T value) const noexcept {
        state.magnitude     = std::max(std::abs(value), std::abs(expected));
        state.abs_tolerance = get_abs_tolerance(state.magnitude, tolerance);
        state.abs_diff      = std::abs(value - expected);
        return state.abs_diff <= state.abs_tolerance;
    }

    constexpr small_string<max_message_length>
    describe_match(T value, match_status status) const noexcept {
        small_string<max_message_length> message;
        append_or_truncate(
            message, value, status == match_status::failed ? " != "sv : " == "sv, expected,
            " (abs. diff.: ", state.abs_diff, ", tol.: ", state.abs_tolerance, ")");
        return message;
    }
};

template<floating_point T>
struct relative {
    T tolerance = {};
};

template<floating_point T>
T get_abs_tolerance(T value, relative<T> reltol) {
    return value * reltol.tolerance;
}

template<floating_point T>
struct absolute {
    T tolerance = {};
};

template<floating_point T>
T get_abs_tolerance(T, absolute<T> abstol) {
    return abstol.tolerance;
}
} // namespace snitch::matchers

TEST_CASE("example matcher has_prefix", "[utility]") {
    using namespace snitch::matchers;

    CHECK("info: hello"sv == has_prefix{"info"});
    CHECK("info: hello"sv != has_prefix{"warning"});
    CHECK("hello"sv != has_prefix{"info"});
    CHECK(has_prefix{"info"} == "info: hello"sv);
    CHECK(has_prefix{"warning"} != "info: hello"sv);
    CHECK(has_prefix{"info"} != "hello"sv);

    CHECK(
        has_prefix{"info"}.describe_match("info: hello"sv, match_status::matched) ==
        "found prefix 'info:' in 'info: hello'"sv);
    CHECK(
        has_prefix{"warning"}.describe_match("info: hello"sv, match_status::failed) ==
        "could not find prefix 'warning:' in 'info: hello'; found prefix 'info:'"sv);
}

TEST_CASE("matcher contains_substring", "[utility]") {
    using namespace snitch::matchers;

    CHECK("info: hello"sv == contains_substring{"hello"});
    CHECK("info: hello"sv != contains_substring{"warning"});
    CHECK(contains_substring{"hello"} == "info: hello"sv);
    CHECK(contains_substring{"warning"} != "info: hello"sv);

    CHECK(
        contains_substring{"hello"}.describe_match("info: hello"sv, match_status::matched) ==
        "found 'hello' in 'info: hello'"sv);
    CHECK(
        contains_substring{"warning"}.describe_match("info: hello"sv, match_status::failed) ==
        "could not find 'warning' in 'info: hello'"sv);
}

TEST_CASE("matcher with_what_contains", "[utility]") {
    using namespace snitch::matchers;

    CHECK(std::runtime_error{"not good"} == with_what_contains{"good"});
    CHECK(std::runtime_error{"not good"} == with_what_contains{"not good"});
    CHECK(std::runtime_error{"not good"} != with_what_contains{"bad"});
    CHECK(std::runtime_error{"not good"} != with_what_contains{"is good"});
    CHECK(with_what_contains{"good"} == std::runtime_error{"not good"});
    CHECK(with_what_contains{"not good"} == std::runtime_error{"not good"});
    CHECK(with_what_contains{"bad"} != std::runtime_error{"not good"});
    CHECK(with_what_contains{"is good"} != std::runtime_error{"not good"});

    CHECK(
        with_what_contains{"good"}.describe_match(
            std::runtime_error{"not good"}, match_status::matched) ==
        "found 'good' in 'not good'"sv);
    CHECK(
        with_what_contains{"bad"}.describe_match(
            std::runtime_error{"not good"}, match_status::failed) ==
        "could not find 'bad' in 'not good'"sv);
}

TEST_CASE("matcher is_any_of", "[utility]") {
    using namespace snitch::matchers;

    const auto m = is_any_of{1u, 2u, 3u};

    CHECK(1u == m);
    CHECK(2u == m);
    CHECK(3u == m);
    CHECK(0u != m);
    CHECK(4u != m);
    CHECK(5u != m);
    CHECK(m == 1u);
    CHECK(m == 2u);
    CHECK(m == 3u);
    CHECK(m != 0u);
    CHECK(m != 4u);
    CHECK(m != 5u);

    CHECK(m.describe_match(2u, match_status::matched) == "'2' was found in {'1', '2', '3'}"sv);
    CHECK(m.describe_match(5u, match_status::failed) == "'5' was not found in {'1', '2', '3'}"sv);
}

TEST_CASE("matcher within", "[utility]") {
    using namespace snitch::matchers;

    SECTION("abs") {
        CHECK(0.0f == within{0.0f, 0.0f});
        CHECK(1.0f == within{1.0f, 0.0f});
        CHECK(1.0f == within{1.09f, 0.1f});
        CHECK(1.0f == within{0.91f, 0.1f});
        CHECK(1.09f == within{1.0f, 0.1f});
        CHECK(0.91f == within{1.0f, 0.1f});
        CHECK(-1.0f == within{-1.0f, 0.0f});
        CHECK(-1.0f == within{-1.09f, 0.1f});
        CHECK(-1.0f == within{-0.91f, 0.1f});
        CHECK(-1.09f == within{-1.0f, 0.1f});
        CHECK(-0.91f == within{-1.0f, 0.1f});

        CHECK(std::nextafter(1.0f, 2.0f) != within{1.0f, 0.0f});
        CHECK(std::nextafter(1.0f, 0.0f) != within{1.0f, 0.0f});
        CHECK(std::nextafter(-1.0f, -2.0f) != within{-1.0f, 0.0f});
        CHECK(std::nextafter(-1.0f, 0.0f) != within{-1.0f, 0.0f});
        CHECK(1.11f != within{1.0f, 0.1f});
        CHECK(0.89f != within{1.0f, 0.1f});
        CHECK(-1.11f != within{-1.0f, 0.1f});
        CHECK(-0.89 != within{-1.0f, 0.1f});

        CHECK(0.0f == within{0.0f, absolute{0.0f}});
    }

    SECTION("rel") {
        CHECK(0.0f == within{0.0f, relative{1e-2f}});
        CHECK(0.0f == within{0.0f, relative{0.0f}});
        CHECK(1.0f == within{1.0f, relative{0.0f}});
        CHECK(1.001f == within{1.0f, relative{1e-2f}});
        CHECK(1.0f == within{1.001f, relative{1e-2f}});
        CHECK(-1.0f == within{-1.0f, relative{0.0f}});
        CHECK(-1.001f == within{-1.0f, relative{1e-2f}});
        CHECK(-1.0f == within{-1.001f, relative{1e-2f}});
        CHECK(10.1f == within{10.0f, relative{1e-1f}});

        CHECK(0.0f != within{100.0f, relative{1e-2f}});
        CHECK(100.0f != within{0.0f, relative{1e-2f}});
        CHECK(0.0f != within{-100.0f, relative{1e-2f}});
        CHECK(-100.0f != within{0.0f, relative{1e-2f}});
        CHECK(12.0f != within{10.0f, relative{1e-1f}});
    }

    SECTION("lambda") {
        CHECK(1.001f == within{1.0f, [](auto v) { return std::sqrt(1e-12 + v * v * 1e-6); }});
    }
}
