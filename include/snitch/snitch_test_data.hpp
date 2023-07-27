#ifndef SNITCH_TEST_DATA_HPP
#define SNITCH_TEST_DATA_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_vector.hpp"

#include <cstddef>
#include <string_view>

namespace snitch {
class registry;

struct source_location {
    std::string_view file = {};
    std::size_t      line = 0u;
};

struct test_id {
    std::string_view name = {};
    std::string_view tags = {};
    std::string_view type = {};
};

struct section_id {
    std::string_view name        = {};
    std::string_view description = {};
};

using filter_info  = small_vector_span<const std::string_view>;
using section_info = small_vector_span<const section_id>;
using capture_info = small_vector_span<const std::string_view>;

using assertion_location = source_location;

enum class test_case_state { success, failed, skipped };
} // namespace snitch

namespace snitch::event {
struct test_run_started {
    std::string_view name    = {};
    filter_info      filters = {};
};

struct test_run_ended {
    std::string_view name            = {};
    filter_info      filters         = {};
    std::size_t      run_count       = 0;
    std::size_t      fail_count      = 0;
    std::size_t      skip_count      = 0;
    std::size_t      assertion_count = 0;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
    bool success = true;
};

struct test_case_started {
    const test_id&         id;
    const source_location& location;
};

struct test_case_ended {
    const test_id&         id;
    const source_location& location;
    std::size_t            assertion_count = 0;
    test_case_state        state           = test_case_state::success;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct assertion_failed {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    std::string_view          message  = {};
    bool                      expected = false;
    bool                      allowed  = false;
};

struct assertion_succeeded {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    std::string_view          message = {};
};

struct test_case_skipped {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    std::string_view          message = {};
};

using data = std::variant<
    test_run_started,
    test_run_ended,
    test_case_started,
    test_case_ended,
    assertion_failed,
    assertion_succeeded,
    test_case_skipped>;
} // namespace snitch::event

namespace snitch {
// Maximum depth of nested sections in a test case (section in section in section ...).
constexpr std::size_t max_nested_sections = SNITCH_MAX_NESTED_SECTIONS;
// Maximum number of captured expressions in a test case.
constexpr std::size_t max_captures = SNITCH_MAX_CAPTURES;
// Maximum length of a captured expression.
constexpr std::size_t max_capture_length = SNITCH_MAX_CAPTURE_LENGTH;
} // namespace snitch

namespace snitch::impl {
using test_ptr = void (*)();

enum class test_case_state { not_run, success, skipped, failed };

struct test_case {
    test_id         id       = {};
    source_location location = {};
    test_ptr        func     = nullptr;
    test_case_state state    = test_case_state::not_run;
};

struct section_nesting_level {
    std::size_t current_section_id  = 0;
    std::size_t previous_section_id = 0;
    std::size_t max_section_id      = 0;
};

struct section_state {
    small_vector<section_id, max_nested_sections>            current_section = {};
    small_vector<section_nesting_level, max_nested_sections> levels          = {};
    std::size_t                                              depth           = 0;
    bool                                                     leaf_executed   = false;
};

using capture_state = small_vector<small_string<max_capture_length>, max_captures>;

struct test_state {
    registry&     reg;
    test_case&    test;
    section_state sections    = {};
    capture_state captures    = {};
    std::size_t   asserts     = 0;
    bool          may_fail    = false;
    bool          should_fail = false;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

test_state& get_current_test() noexcept;

test_state* try_get_current_test() noexcept;

void set_current_test(test_state* current) noexcept;
} // namespace snitch::impl

#endif
