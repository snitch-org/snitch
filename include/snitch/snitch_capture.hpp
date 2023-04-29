#ifndef SNITCH_CAPTURE_HPP
#define SNITCH_CAPTURE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"
#include "snitch/snitch_test_data.hpp"

#include <cstddef>
#include <string_view>

namespace snitch::impl {
struct scoped_capture {
    capture_state& captures;
    std::size_t    count = 0;

    ~scoped_capture() {
        captures.resize(captures.size() - count);
    }
};

std::string_view extract_next_name(std::string_view& names) noexcept;

struct test_state;

// Requires: number of captures < max_captures.
small_string<max_capture_length>& add_capture(test_state& state);

// Requires: number of captures < max_captures.
template<string_appendable T>
void add_capture(test_state& state, std::string_view& names, const T& arg) {
    auto& capture = add_capture(state);
    append_or_truncate(capture, extract_next_name(names), " := ", arg);
}

// Requires: number of captures < max_captures.
template<string_appendable... Args>
scoped_capture add_captures(test_state& state, std::string_view names, const Args&... args) {
    (add_capture(state, names, args), ...);
    return {state.captures, sizeof...(args)};
}

// Requires: number of captures < max_captures.
template<string_appendable... Args>
scoped_capture add_info(test_state& state, const Args&... args) {
    auto& capture = add_capture(state);
    append_or_truncate(capture, args...);
    return {state.captures, 1};
}
} // namespace snitch::impl

#endif
