#include "snitch/snitch_time.hpp"

#if SNITCH_WITH_TIMINGS
#    include <chrono>

namespace snitch {
namespace impl {
using clock           = std::chrono::steady_clock;
using tick_resolution = std::chrono::nanoseconds;
} // namespace impl

time_point_t get_current_time() noexcept {
    static auto start_time = impl::clock::now();
    return static_cast<time_point_t>(
        std::chrono::duration_cast<impl::tick_resolution>(impl::clock::now() - start_time).count());
}

float get_duration_in_seconds(time_point_t start, time_point_t end) noexcept {
    return std::chrono::duration_cast<std::chrono::duration<float>>(
               impl::tick_resolution(end - start))
        .count();
}
} // namespace snitch
#endif
