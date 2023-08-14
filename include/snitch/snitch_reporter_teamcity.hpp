#ifndef SNITCH_TEAMCITY_HPP
#define SNITCH_TEAMCITY_HPP

#include "snitch/snitch_config.hpp"

#if defined(SNITCH_WITH_TEAMCITY_REPORTER) || defined(SNITCH_WITH_ALL_REPORTERS)

#    include "snitch/snitch_test_data.hpp"

namespace snitch::teamcity {
void initialize(registry& r) noexcept;

void report(const registry& r, const snitch::event::data& event) noexcept;
} // namespace snitch::teamcity

#endif
#endif