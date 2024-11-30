#ifndef SNITCH_MATCHER_HPP
#define SNITCH_MATCHER_HPP

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_error_handling.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"

#include <optional>

namespace snitch::matchers {
enum class match_status { failed, matched };
}

namespace snitch {
template<typename T, typename U>
concept matcher_for = requires(const T& m, const U& value) {
    { m.match(value) } -> convertible_to<bool>;
    { m.describe_match(value, matchers::match_status{}) } -> convertible_to<std::string_view>;
};
} // namespace snitch

namespace snitch::impl {
template<typename T>
concept exception_with_what = requires(const T& e) {
    { e.what() } -> convertible_to<std::string_view>;
};

template<typename T, typename M>
[[nodiscard]] constexpr auto match(T&& value, M&& matcher) noexcept {
    using result_type = decltype(matcher.describe_match(value, matchers::match_status::failed));
    if (!matcher.match(value)) {
        return std::pair<bool, result_type>(
            false, matcher.describe_match(value, matchers::match_status::failed));
    } else {
        return std::pair<bool, result_type>(
            true, matcher.describe_match(value, matchers::match_status::matched));
    }
}
} // namespace snitch::impl

namespace snitch::matchers {
struct contains_substring {
    std::string_view substring_pattern;

    SNITCH_EXPORT explicit contains_substring(std::string_view pattern) noexcept;

    SNITCH_EXPORT bool match(std::string_view message) const noexcept;

    SNITCH_EXPORT small_string<max_message_length>
                  describe_match(std::string_view message, match_status status) const noexcept;
};

template<typename T, std::size_t N>
struct is_any_of {
    small_vector<T, N> list;

    template<typename... Args>
    explicit is_any_of(const Args&... args) noexcept : list({args...}) {}

    bool match(const T& value) const noexcept {
        for (const auto& v : list) {
            if (v == value) {
                return true;
            }
        }

        return false;
    }

    small_string<max_message_length>
    describe_match(const T& value, match_status status) const noexcept {
        small_string<max_message_length> description_buffer;
        append_or_truncate(
            description_buffer, "'", value, "' was ",
            (status == match_status::failed ? "not " : ""), "found in {");

        bool first = true;
        for (const auto& v : list) {
            if (!first) {
                append_or_truncate(description_buffer, ", '", v, "'");
            } else {
                append_or_truncate(description_buffer, "'", v, "'");
            }
            first = false;
        }
        append_or_truncate(description_buffer, "}");

        return description_buffer;
    }
};

template<typename T, typename... Args>
is_any_of(T, Args...) -> is_any_of<T, sizeof...(Args) + 1>;

struct with_what_contains : private contains_substring {
    SNITCH_EXPORT explicit with_what_contains(std::string_view pattern) noexcept;

    template<snitch::impl::exception_with_what E>
    bool match(const E& e) const noexcept {
        return contains_substring::match(e.what());
    }

    template<snitch::impl::exception_with_what E>
    small_string<max_message_length>
    describe_match(const E& e, match_status status) const noexcept {
        return contains_substring::describe_match(e.what(), status);
    }
};

template<typename T, matcher_for<T> M>
bool operator==(const T& value, const M& m) noexcept {
    return m.match(value);
}

template<typename T, matcher_for<T> M>
bool operator==(const M& m, const T& value) noexcept {
    return m.match(value);
}
} // namespace snitch::matchers

#endif
