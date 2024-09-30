#include "snitch/snitch_section.hpp"

#include "snitch/snitch_console.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_test_data.hpp"
#include "snitch/snitch_time.hpp"

#if SNITCH_WITH_EXCEPTIONS
#    include <exception>
#endif

namespace snitch::impl {
section_entry_checker::~section_entry_checker() {
    auto& sections = state.info.sections;

    if (entered) {
#if SNITCH_WITH_EXCEPTIONS
        if (std::uncaught_exceptions() > 0 && !state.held_info.has_value()) {
            // We are unwinding the stack because an exception has been thrown;
            // keep a copy of the full section state since we will want to preserve the information
            // when reporting the exception.
            state.held_info = state.info;
        }
#endif

        pop_location(state);

        bool last_entry = false;
        if (sections.depth == sections.levels.size()) {
            // We just entered this section, and there was no child section in it.
            // This is a leaf; flag that a leaf has been executed so that no other leaf
            // is executed in this run.
            // Note: don't pop this level from the section state yet, it may have siblings
            // that we don't know about yet. Popping will be done when we exit from the parent,
            // since then we will know if there is any sibling.
            sections.leaf_executed = true;
            last_entry             = true;
        } else {
            // Check if there is any child section left to execute, at any depth below this one.
            bool no_child_section_left = true;
            for (std::size_t c = sections.depth; c < sections.levels.size(); ++c) {
                auto& child = sections.levels[c];
                if (child.previous_section_id != child.max_section_id) {
                    no_child_section_left = false;
                    break;
                }
            }

            if (no_child_section_left) {
                // No more children, we can pop this level and never go back.
                sections.levels.pop_back();
                last_entry = true;
            }
        }

        // Emit the section end event (only on last entry, and only if no exception in flight).
#if SNITCH_WITH_EXCEPTIONS
        if (last_entry && std::uncaught_exceptions() == 0)
#else
        if (last_entry)
#endif
        {
            registry::report_section_ended(sections.current_section.back());
        }

        sections.current_section.pop_back();
    }

    --sections.depth;
}

section_entry_checker::operator bool() {
#if SNITCH_WITH_EXCEPTIONS
    if (std::uncaught_exceptions() == 0) {
        notify_exception_handled();
    }
#endif

    auto& sections = state.info.sections;

    if (sections.depth >= sections.levels.size()) {
        if (sections.depth >= max_nested_sections) {
            using namespace snitch::impl;
            state.reg.print(
                make_colored("error:", state.reg.with_color, color::fail),
                " max number of nested sections reached; "
                "please increase 'SNITCH_MAX_NESTED_SECTIONS' (currently ",
                max_nested_sections, ")\n.");
            assertion_failed("max number of nested sections reached");
        }

        sections.levels.push_back({});
    }

    ++sections.depth;

    auto& level = sections.levels[sections.depth - 1];

    ++level.current_section_id;
    if (level.current_section_id > level.max_section_id) {
        level.max_section_id = level.current_section_id;
    }

    if (sections.leaf_executed) {
        // We have already executed another leaf section; can't execute more
        // on this run, so don't bother going inside this one now.
        return false;
    }

    const bool previous_was_preceeding_sibling =
        level.current_section_id == level.previous_section_id + 1;
    const bool children_remaining_in_self = level.current_section_id == level.previous_section_id &&
                                            sections.depth < sections.levels.size();

    if (!previous_was_preceeding_sibling && !children_remaining_in_self) {
        // Skip this section if:
        //  - The section entered in the previous run was not its immediate previous sibling, and
        //  - This section was not already entered in the previous run with remaining children.
        return false;
    }

    // Entering this section.

    // Push new section on the stack.
    level.previous_section_id = level.current_section_id;
    sections.current_section.push_back(
#if SNITCH_WITH_TIMINGS
        section{.id = id, .location = location, .start_time = get_current_time()}
#else
        section{.id = id, .location = location}
#endif
    );

    push_location(state, {location.file, location.line, location_type::section_scope});
    entered = true;

    // Emit the section start event (only on first entry).
    if (previous_was_preceeding_sibling) {
        registry::report_section_started(sections.current_section.back());
    }

    return true;
}
} // namespace snitch::impl
