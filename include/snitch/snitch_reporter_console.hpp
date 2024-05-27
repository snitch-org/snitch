#ifndef SNITCH_REPORTER_CONSOLE_HPP
#define SNITCH_REPORTER_CONSOLE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_test_data.hpp"

#include <string_view>

namespace snitch::reporter::console {
struct reporter {
    std::size_t counter = 0;

    reporter() = default;

    SNITCH_EXPORT explicit reporter(registry& r) noexcept;

    SNITCH_EXPORT bool configure(registry&, std::string_view, std::string_view) noexcept;

    SNITCH_EXPORT void report(const registry& r, const snitch::event::data& event) noexcept;
};
} // namespace snitch::reporter::console

#endif
