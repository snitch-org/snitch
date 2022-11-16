#include "testing.hpp"

#include <stdexcept>

using namespace std::literals;

namespace snatch::matchers {
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
} // namespace snatch::matchers

TEST_CASE("example matcher has_prefix", "[utility]") {
    CHECK("info: hello"sv == snatch::matchers::has_prefix{"info"});
    CHECK("info: hello"sv != snatch::matchers::has_prefix{"warning"});
    CHECK("hello"sv != snatch::matchers::has_prefix{"info"});
    CHECK(snatch::matchers::has_prefix{"info"} == "info: hello"sv);
    CHECK(snatch::matchers::has_prefix{"warning"} != "info: hello"sv);
    CHECK(snatch::matchers::has_prefix{"info"} != "hello"sv);
};

TEST_CASE("matcher contains_substring", "[utility]") {
    CHECK("info: hello"sv == snatch::matchers::contains_substring{"hello"});
    CHECK("info: hello"sv != snatch::matchers::contains_substring{"warning"});
    CHECK(snatch::matchers::contains_substring{"hello"} == "info: hello"sv);
    CHECK(snatch::matchers::contains_substring{"warning"} != "info: hello"sv);
};

TEST_CASE("matcher with_what_contains", "[utility]") {
    CHECK(std::runtime_error{"not good"} == snatch::matchers::with_what_contains{"good"});
    CHECK(std::runtime_error{"not good"} == snatch::matchers::with_what_contains{"not good"});
    CHECK(std::runtime_error{"not good"} != snatch::matchers::with_what_contains{"bad"});
    CHECK(std::runtime_error{"not good"} != snatch::matchers::with_what_contains{"is good"});
    CHECK(snatch::matchers::with_what_contains{"good"} == std::runtime_error{"not good"});
    CHECK(snatch::matchers::with_what_contains{"not good"} == std::runtime_error{"not good"});
    CHECK(snatch::matchers::with_what_contains{"bad"} != std::runtime_error{"not good"});
    CHECK(snatch::matchers::with_what_contains{"is good"} != std::runtime_error{"not good"});
};

TEST_CASE("matcher is_any_of", "[utility]") {
    CHECK(1u == (snatch::matchers::is_any_of{1u, 2u, 3u}));
    CHECK(2u == (snatch::matchers::is_any_of{1u, 2u, 3u}));
    CHECK(3u == (snatch::matchers::is_any_of{1u, 2u, 3u}));
    CHECK(0u != (snatch::matchers::is_any_of{1u, 2u, 3u}));
    CHECK(4u != (snatch::matchers::is_any_of{1u, 2u, 3u}));
    CHECK(5u != (snatch::matchers::is_any_of{1u, 2u, 3u}));
    CHECK((snatch::matchers::is_any_of{1u, 2u, 3u}) == 1u);
    CHECK((snatch::matchers::is_any_of{1u, 2u, 3u}) == 2u);
    CHECK((snatch::matchers::is_any_of{1u, 2u, 3u}) == 3u);
    CHECK((snatch::matchers::is_any_of{1u, 2u, 3u}) != 0u);
    CHECK((snatch::matchers::is_any_of{1u, 2u, 3u}) != 4u);
    CHECK((snatch::matchers::is_any_of{1u, 2u, 3u}) != 5u);
};
