#ifndef SNITCH_SECTION_HPP
#define SNITCH_SECTION_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_test_data.hpp"
#if SNITCH_WITH_TIMINGS
#    include <chrono>
#endif

namespace snitch::impl {
struct section_entry_checker {
    section     data = {};
    test_state& state;
    bool        entered          = false;
    std::size_t asserts          = 0;
    std::size_t failures         = 0;
    std::size_t allowed_failures = 0;
#if SNITCH_WITH_TIMINGS
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
#endif

    SNITCH_EXPORT ~section_entry_checker();

    // Requires: number of sections < max_nested_sections.
    SNITCH_EXPORT explicit operator bool();
};
} // namespace snitch::impl

#endif
