#ifndef SNITCH_SECTION_HPP
#define SNITCH_SECTION_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_test_data.hpp"

namespace snitch::impl {
struct section_entry_checker {
    section     data = {};
    test_state& state;
    bool        entered = false;

    ~section_entry_checker();

    // Requires: number of sections < max_nested_sections.
    explicit operator bool();
};
} // namespace snitch::impl

#endif
