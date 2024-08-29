#ifndef SNITCH_STRING_UTILITY_HPP
#define SNITCH_STRING_UTILITY_HPP

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_string.hpp"

#include <cstddef>
#include <string_view>
#include <utility>

namespace snitch {
constexpr void truncate_end(small_string_span ss) noexcept {
    std::size_t num_dots     = 3;
    std::size_t final_length = ss.size() + num_dots;
    if (final_length > ss.capacity()) {
        final_length = ss.capacity();
    }

    const std::size_t offset = final_length >= num_dots ? final_length - num_dots : 0;
    num_dots                 = final_length - offset;

    ss.resize(final_length);
    for (std::size_t i = 0; i < num_dots; ++i) {
        ss[offset + i] = '.';
    }
}

template<string_appendable... Args>
constexpr bool append_or_truncate(small_string_span ss, Args&&... args) noexcept {
    if (!append(ss, std::forward<Args>(args)...)) {
        truncate_end(ss);
        return false;
    }

    return true;
}

template<std::size_t N, std::size_t M>
constexpr small_string<N> resize_or_truncate(const small_string<M>& str) noexcept {
    if constexpr (N == M) {
        return str;
    } else if constexpr (N > M) {
        small_string<N> out;
        append(out, std::string_view{str});
        return out;
    } else {
        small_string<N> out;
        append_or_truncate(out, std::string_view{str});
        return out;
    }
}

template<std::size_t N, std::size_t M>
constexpr small_string<N> resize_or_truncate(std::string_view str) noexcept {
    small_string<N> out;
    append(out, str);
    return out;
}

SNITCH_EXPORT [[nodiscard]] bool replace_all(
    small_string_span string, std::string_view pattern, std::string_view replacement) noexcept;

// Requires: replacement.length() > pattern.length()
SNITCH_EXPORT [[nodiscard]] bool escape_all_or_truncate(
    small_string_span string, std::string_view pattern, std::string_view replacement);

SNITCH_EXPORT [[nodiscard]] std::size_t
find_first_not_escaped(std::string_view str, char c) noexcept;

SNITCH_EXPORT [[nodiscard]] bool is_match(std::string_view string, std::string_view regex) noexcept;
} // namespace snitch

#endif
