#ifndef SNITCH_REPORTER_TEAMCITY_HPP
#define SNITCH_REPORTER_TEAMCITY_HPP

#include "snitch/snitch_config.hpp"

#if SNITCH_WITH_TEAMCITY_REPORTER || SNITCH_WITH_ALL_REPORTERS

#    include "snitch/snitch_test_data.hpp"

#    include <string_view>

namespace snitch::reporter::teamcity {
SNITCH_EXPORT void initialize(registry& r) noexcept;

SNITCH_EXPORT void report(const registry& r, const snitch::event::data& event) noexcept;
} // namespace snitch::reporter::teamcity

#endif
#endif
