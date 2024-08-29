#include "snitch/snitch_section.hpp"

#include "snitch/snitch_console.hpp"
#include "snitch/snitch_registry.hpp"

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

        if (sections.depth == sections.levels.size()) {
            // We just entered this section, and there was no child section in it.
            // This is a leaf; flag that a leaf has been executed so that no other leaf
            // is executed in this run.
            // Note: don't pop this level from the section state yet, it may have siblings
            // that we don't know about yet. Popping will be done when we exit from the parent,
            // since then we will know if there is any sibling.
            sections.leaf_executed = true;
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
            }
        }

        sections.current_section.pop_back();
    }

    --sections.depth;
}

section_entry_checker::operator bool() {
#if SNITCH_WITH_EXCEPTIONS
    state.held_info.reset();
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

    // Only enter this section if:
    //  - The section entered in the previous run was its immediate previous sibling, or
    //  - This section was already entered in the previous run, and child sections exist in it.
    if (level.current_section_id == level.previous_section_id + 1 ||
        (level.current_section_id == level.previous_section_id &&
         sections.depth < sections.levels.size())) {

        level.previous_section_id = level.current_section_id;
        sections.current_section.push_back(data);
        push_location(
            state, {data.location.file, data.location.line, location_type::section_scope});
        entered = true;
        return true;
    }

    return false;
}
} // namespace snitch::impl
