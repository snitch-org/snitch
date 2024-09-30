#ifndef SNITCH_TIME_HPP
#define SNITCH_TIME_HPP

#include "snitch/snitch_config.hpp"

#if SNITCH_WITH_TIMINGS

namespace snitch {
using time_point_t = std::size_t;

SNITCH_EXPORT time_point_t get_current_time() noexcept;

SNITCH_EXPORT float get_duration_in_seconds(time_point_t start, time_point_t end) noexcept;
} // namespace snitch

#endif
#endif
