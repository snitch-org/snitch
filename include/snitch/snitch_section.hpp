#ifndef SNITCH_SECTION_HPP
#define SNITCH_SECTION_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_test_data.hpp"

#include <type_traits>

namespace snitch::impl {
struct section_entry_checker {
    section     data = {};
    test_state& state;
    bool        entered = false;

#if SNITCH_WITH_TIMINGS
    std::make_signed_t<std::size_t> start_time = 0;
#endif

    SNITCH_EXPORT ~section_entry_checker();

    // Requires: number of sections < max_nested_sections.
    SNITCH_EXPORT explicit operator bool();
};
} // namespace snitch::impl

#endif
