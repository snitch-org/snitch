#include "testing.hpp"

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

TEST_CASE("matcher", "[utility]") {
    CHECK("info: hello"sv == snatch::matchers::has_prefix{"info"});
    CHECK("info: hello"sv != snatch::matchers::has_prefix{"warning"});
    CHECK("hello"sv != snatch::matchers::has_prefix{"info"});
    CHECK(snatch::matchers::has_prefix{"info"} == "info: hello"sv);
    CHECK(snatch::matchers::has_prefix{"warning"} != "info: hello"sv);
    CHECK(snatch::matchers::has_prefix{"info"} != "hello"sv);
};
