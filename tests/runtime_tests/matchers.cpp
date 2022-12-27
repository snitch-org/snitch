#include "testing.hpp"

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
} // namespace snitch::matchers

TEST_CASE("example matcher has_prefix", "[utility]") {
    CHECK("info: hello"sv == snitch::matchers::has_prefix{"info"});
    CHECK("info: hello"sv != snitch::matchers::has_prefix{"warning"});
    CHECK("hello"sv != snitch::matchers::has_prefix{"info"});
    CHECK(snitch::matchers::has_prefix{"info"} == "info: hello"sv);
    CHECK(snitch::matchers::has_prefix{"warning"} != "info: hello"sv);
    CHECK(snitch::matchers::has_prefix{"info"} != "hello"sv);

    CHECK(
        snitch::matchers::has_prefix{"info"}.describe_match(
            "info: hello"sv, snitch::matchers::match_status::matched) ==
        "found prefix 'info:' in 'info: hello'"sv);
    CHECK(
        snitch::matchers::has_prefix{"warning"}.describe_match(
            "info: hello"sv, snitch::matchers::match_status::failed) ==
        "could not find prefix 'warning:' in 'info: hello'; found prefix 'info:'"sv);
}

TEST_CASE("matcher contains_substring", "[utility]") {
    CHECK("info: hello"sv == snitch::matchers::contains_substring{"hello"});
    CHECK("info: hello"sv != snitch::matchers::contains_substring{"warning"});
    CHECK(snitch::matchers::contains_substring{"hello"} == "info: hello"sv);
    CHECK(snitch::matchers::contains_substring{"warning"} != "info: hello"sv);

    CHECK(
        snitch::matchers::contains_substring{"hello"}.describe_match(
            "info: hello"sv, snitch::matchers::match_status::matched) ==
        "found 'hello' in 'info: hello'"sv);
    CHECK(
        snitch::matchers::contains_substring{"warning"}.describe_match(
            "info: hello"sv, snitch::matchers::match_status::failed) ==
        "could not find 'warning' in 'info: hello'"sv);
}

TEST_CASE("matcher with_what_contains", "[utility]") {
    CHECK(std::runtime_error{"not good"} == snitch::matchers::with_what_contains{"good"});
    CHECK(std::runtime_error{"not good"} == snitch::matchers::with_what_contains{"not good"});
    CHECK(std::runtime_error{"not good"} != snitch::matchers::with_what_contains{"bad"});
    CHECK(std::runtime_error{"not good"} != snitch::matchers::with_what_contains{"is good"});
    CHECK(snitch::matchers::with_what_contains{"good"} == std::runtime_error{"not good"});
    CHECK(snitch::matchers::with_what_contains{"not good"} == std::runtime_error{"not good"});
    CHECK(snitch::matchers::with_what_contains{"bad"} != std::runtime_error{"not good"});
    CHECK(snitch::matchers::with_what_contains{"is good"} != std::runtime_error{"not good"});

    CHECK(
        snitch::matchers::with_what_contains{"good"}.describe_match(
            std::runtime_error{"not good"}, snitch::matchers::match_status::matched) ==
        "found 'good' in 'not good'"sv);
    CHECK(
        snitch::matchers::with_what_contains{"bad"}.describe_match(
            std::runtime_error{"not good"}, snitch::matchers::match_status::failed) ==
        "could not find 'bad' in 'not good'"sv);
}

TEST_CASE("matcher is_any_of", "[utility]") {
    const auto m = snitch::matchers::is_any_of{1u, 2u, 3u};

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

    CHECK(
        m.describe_match(2u, snitch::matchers::match_status::matched) ==
        "'2' was found in {'1', '2', '3'}"sv);
    CHECK(
        m.describe_match(5u, snitch::matchers::match_status::failed) ==
        "'5' was not found in {'1', '2', '3'}"sv);
}
