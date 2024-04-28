#ifndef SNITCH_TEST_DATA_HPP
#define SNITCH_TEST_DATA_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_vector.hpp"

#include <cstddef>
#include <string_view>

namespace snitch {
class registry;

/// Identifies a location in source code
struct source_location {
    /// Absolute path to the file
    std::string_view file = {};
    /// Line number (starts at 1)
    std::size_t line = 0u;
};

/// Identifies a test case
struct test_id {
    /// Name of the test case, as given in the source
    std::string_view name = {};
    /// Tags of the test case, as given in the source
    std::string_view tags = {};
    /// Name of the type for which this test case is instanciated (templated test cases only)
    std::string_view type = {};
    /// Name of the fixture class from which the test case is instanciated (method test cases only)
    std::string_view fixture = {};
};

/// Identies a section
struct section_id {
    /// Name of the section, as given in the source
    std::string_view name = {};
    /// Description of the section, as given in the source
    std::string_view description = {};
};

/// Section data
struct section {
    /// Identifiers (name, description)
    section_id id = {};
    /// Location (file, line)
    source_location location = {};
};

/// List of test case filters
using filter_info = small_vector_span<const std::string_view>;
/// List of active sections (in increasing nesting level)
using section_info = small_vector_span<const section>;
/// List of active captures (in order of declaration)
using capture_info = small_vector_span<const std::string_view>;

enum class location_type {
    /// The true location is precisely at the indicated location
    exact,
    /// The true location is somewhere inside the section starting at the indicated location
    section_scope,
    /// The true location is somewhere inside the test case starting at the indicated location
    test_case_scope,
    /// The true location is somehere further down the call stack from the indicated location
    in_check
};

/// Identifies a location in source code
struct assertion_location {
    /// Absolute path to the file
    std::string_view file = {};
    /// Line number (starts at 1)
    std::size_t line = 0u;
    /// Type of location
    location_type type = location_type::exact;
};

/// State of a test case after execution
enum class test_case_state {
    /// All checks passed
    success,
    /// Some checks failed and the test does not allow failure
    failed,
    /// Some checks failed and the tests allows failure (e.g., [!shouldfail] and [!mayfail])
    allowed_fail,
    /// Test case explicitly skipped (with SKIP(...))
    skipped
};

/// Content of an expression
struct expression_info {
    /// Macro used for the assertion (CHECK, etc.)
    std::string_view type;
    /// Expression as written in the source code
    std::string_view expected;
    /// Expression with evaluated operands
    std::string_view actual;
};

/// Payload of an assertion (error message, expression, ...)
using assertion_data = std::variant<std::string_view, expression_info>;
} // namespace snitch

namespace snitch::event {
/// Fired at the start of a test run (application started)
struct test_run_started {
    /// Name of the test application
    std::string_view name = {};
    /// List of test case filters, as given in the command-line arguments
    filter_info filters = {};
};

/// Fired at the end of a test run (application finished)
struct test_run_ended {
    /// Name of the test application
    std::string_view name = {};
    /// List of test case filters, as given in the command-line arguments
    filter_info filters = {};

    /// Counts all test cases; passed, failed, allowed to fail, or skipped
    std::size_t run_count = 0;
    /// Counts all failed test cases
    std::size_t fail_count = 0;
    /// Counts all allowed failed test cases
    std::size_t allowed_fail_count = 0;
    /// Counts all skipped test cases
    std::size_t skip_count = 0;

    /// Counts all assertions; passed, failed, or allowed failed
    std::size_t assertion_count = 0;
    /// Counts failed assertions
    std::size_t assertion_failure_count = 0;
    /// Counts allowed failed assertions (e.g., [!shouldfail] and [!mayfail])
    std::size_t allowed_assertion_failure_count = 0;

#if SNITCH_WITH_TIMINGS
    /// Total test duration, in seconds
    float duration = 0.0f;
#endif

    /// True if all tests passed, or all failures were allowed
    bool success = true;
};

/// Fired at the start of a test case
struct test_case_started {
    /// Test ID
    const test_id& id;
    /// Test location
    const source_location& location;
};

/// Fired at the end of a test case
struct test_case_ended {
    /// Test ID
    const test_id& id;
    /// Test location
    const source_location& location;

    /// Counts all assertions; passed, failed, or allowed failed
    std::size_t assertion_count = 0;
    /// Counts failed assertions
    std::size_t assertion_failure_count = 0;
    /// Counts allowed failed assertions (e.g., [!shouldfail] and [!mayfail])
    std::size_t allowed_assertion_failure_count = 0;

    /// Test result
    test_case_state state = test_case_state::success;

#if SNITCH_WITH_TIMINGS
    /// Test case duration, in seconds
    float duration = 0.0f;
#endif

    bool failure_expected = false;
    bool failure_allowed  = false;
};

struct section_started {
    /// Identifiers (name, description)
    section_id id = {};
    /// Location (file, line)
    source_location location = {};
};

struct section_ended {
    /// Identifiers (name, description)
    section_id id = {};
    /// Location (file, line)
    source_location location                        = {};
    bool            skipped                         = false;
    std::size_t     assertion_count                 = 0;
    std::size_t     assertion_failure_count         = 0;
    std::size_t     allowed_assertion_failure_count = 0;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct assertion_failed {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    assertion_data            data     = {};
    bool                      expected = false; /// [!shouldfail]
    bool                      allowed  = false; /// [!mayfail]
};

struct assertion_succeeded {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    assertion_data            data = {};
};

struct test_case_skipped {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    std::string_view          message = {};
};

/// Fired at the start of a test listing run (application started)
struct list_test_run_started {
    /// Name of the test application
    std::string_view name = {};
    /// List of test case filters, as given in the command-line arguments
    filter_info filters = {};
};

/// Fired for each selected test case in a test listing run
struct test_case_listed {
    /// Test ID
    const test_id& id;
    /// Test location
    const source_location& location;
};

/// Fired at the end of a test run (application finished)
struct list_test_run_ended {
    /// Name of the test application
    std::string_view name = {};
    /// List of test case filters, as given in the command-line arguments
    filter_info filters = {};
};

using data = std::variant<
    test_run_started,
    test_run_ended,
    test_case_started,
    test_case_ended,
    section_started,
    section_ended,
    assertion_failed,
    assertion_succeeded,
    test_case_skipped,
    list_test_run_started,
    list_test_run_ended,
    test_case_listed>;
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

enum class test_case_state { not_run, success, skipped, failed, allowed_fail };

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
    small_vector<section, max_nested_sections>               current_section = {};
    small_vector<section_nesting_level, max_nested_sections> levels          = {};
    std::size_t                                              depth           = 0;
    bool                                                     leaf_executed   = false;
};

using capture_state = small_vector<small_string<max_capture_length>, max_captures>;

// NB: +2 is because we need one for the test case location, and one for the check location
using location_state = small_vector<assertion_location, max_nested_sections + 2>;

struct test_state {
    registry&      reg;
    test_case&     test;
    section_state  sections  = {};
    capture_state  captures  = {};
    location_state locations = {};

    std::size_t asserts          = 0;
    std::size_t failures         = 0;
    std::size_t allowed_failures = 0;
    bool        may_fail         = false;
    bool        should_fail      = false;
    bool        in_check         = false;

#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

SNITCH_EXPORT test_state& get_current_test() noexcept;

SNITCH_EXPORT test_state* try_get_current_test() noexcept;

SNITCH_EXPORT void set_current_test(test_state* current) noexcept;

SNITCH_EXPORT void push_location(test_state& test, const assertion_location& location) noexcept;

SNITCH_EXPORT void pop_location(test_state& test) noexcept;

struct scoped_test_check {
    test_state& test;

    SNITCH_EXPORT explicit scoped_test_check(const source_location& location) noexcept;

    SNITCH_EXPORT ~scoped_test_check() noexcept;
};
} // namespace snitch::impl

#endif
