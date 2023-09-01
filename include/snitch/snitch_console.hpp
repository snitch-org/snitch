#ifndef SNITCH_CONSOLE_HPP
#define SNITCH_CONSOLE_HPP

#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"

#include <string_view>

namespace snitch::impl {
SNITCH_EXPORT void stdout_print(std::string_view message) noexcept;

using color_t = std::string_view;

namespace color {
constexpr color_t error [[maybe_unused]]      = "\x1b[1;31m";
constexpr color_t warning [[maybe_unused]]    = "\x1b[1;33m";
constexpr color_t status [[maybe_unused]]     = "\x1b[1;36m";
constexpr color_t fail [[maybe_unused]]       = "\x1b[1;31m";
constexpr color_t skipped [[maybe_unused]]    = "\x1b[1;33m";
constexpr color_t pass [[maybe_unused]]       = "\x1b[1;32m";
constexpr color_t highlight1 [[maybe_unused]] = "\x1b[1;35m";
constexpr color_t highlight2 [[maybe_unused]] = "\x1b[1;36m";
constexpr color_t reset [[maybe_unused]]      = "\x1b[0m";
} // namespace color

template<typename T>
struct colored {
    const T& value;
    color_t  color_start;
    color_t  color_end;
};

template<typename T>
colored<T> make_colored(const T& t, bool with_color, color_t start) noexcept {
    return {t, with_color ? start : "", with_color ? color::reset : ""};
}

template<typename T>
bool append(small_string_span ss, const colored<T>& colored_value) noexcept {
    if (ss.available() <= colored_value.color_start.size() + colored_value.color_end.size()) {
        return false;
    }

    bool could_fit = true;
    if (!append(ss, colored_value.color_start, colored_value.value)) {
        ss.resize(ss.capacity() - colored_value.color_end.size());
        could_fit = false;
    }

    return append(ss, colored_value.color_end) && could_fit;
}
} // namespace snitch::impl

#endif
