#ifndef SNITCH_REPORTER_CONSOLE_HPP
#define SNITCH_REPORTER_CONSOLE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_test_data.hpp"

namespace snitch::reporter::console {
void initialize(registry& r) noexcept;

bool configure(registry&, std::string_view, std::string_view) noexcept;

void report(const registry& r, const snitch::event::data& event) noexcept;
} // namespace snitch::reporter::console

#endif
