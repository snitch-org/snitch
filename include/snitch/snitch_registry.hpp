#ifndef SNITCH_REGISTRY_HPP
#define SNITCH_REGISTRY_HPP

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_cli.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_console.hpp"
#include "snitch/snitch_error_handling.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_function.hpp"
#include "snitch/snitch_reporter_console.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"
#include "snitch/snitch_test_data.hpp"
#include "snitch/snitch_type_name.hpp"
#include "snitch/snitch_vector.hpp"

#include <cstddef>
#include <string_view>
#include <utility>

namespace snitch {
// Maximum number of test cases in the whole program.
// A "test case" is created for each uses of the `*_TEST_CASE` macros,
// and for each type for the `TEMPLATE_LIST_TEST_CASE` macro.
constexpr std::size_t max_test_cases = SNITCH_MAX_TEST_CASES;
// Maximum length of a full test case name.
// The full test case name includes the base name, plus any type.
constexpr std::size_t max_test_name_length = SNITCH_MAX_TEST_NAME_LENGTH;
// Maximum length of a tag, including brackets.
constexpr std::size_t max_tag_length = SNITCH_MAX_TAG_LENGTH;
// Maximum number of unique tags in the whole program.
constexpr std::size_t max_unique_tags = SNITCH_MAX_UNIQUE_TAGS;
// Maximum number of registered reporters to select from the command line.
constexpr std::size_t max_registered_reporters = SNITCH_MAX_REGISTERED_REPORTERS;
} // namespace snitch

namespace snitch::impl {
struct name_and_tags {
    std::string_view name = {};
    std::string_view tags = {};
};

struct fixture_name_and_tags {
    std::string_view fixture = {};
    std::string_view name    = {};
    std::string_view tags    = {};
};

std::string_view
make_full_name(small_string<max_test_name_length>& buffer, const test_id& id) noexcept;

template<typename T, typename F>
constexpr test_ptr to_test_case_ptr(const F&) noexcept {
    return []() { F{}.template operator()<T>(); };
}

struct abort_exception {};

void parse_colour_mode_option(registry& reg, std::string_view color_option) noexcept;
void parse_color_option(registry& reg, std::string_view color_option) noexcept;
} // namespace snitch::impl

namespace snitch {
template<typename... Args>
struct type_list {};

struct filter_result {
    bool included = false;
    bool implicit = false;
};

[[nodiscard]] filter_result filter_result_and(filter_result first, filter_result second) noexcept;

[[nodiscard]] filter_result filter_result_or(filter_result first, filter_result second) noexcept;

[[nodiscard]] filter_result
is_filter_match_name(std::string_view name, std::string_view filter) noexcept;

[[nodiscard]] filter_result
is_filter_match_tags(std::string_view tags, std::string_view filter) noexcept;

[[nodiscard]] filter_result
is_filter_match_id(std::string_view name, std::string_view tags, std::string_view filter) noexcept;

using print_function  = small_function<void(std::string_view) noexcept>;
using report_function = small_function<void(const registry&, const event::data&) noexcept>;
using configure_report_function =
    small_function<bool(registry&, std::string_view, std::string_view) noexcept>;
using initialize_report_function = small_function<void(registry&) noexcept>;
using finish_report_function     = small_function<void(registry&) noexcept>;

struct registered_reporter {
    std::string_view           name;
    initialize_report_function initialize = [](registry&) noexcept {};
    configure_report_function  configure =
        [](registry&, std::string_view, std::string_view) noexcept { return false; };
    report_function        callback = [](const registry&, const event::data&) noexcept {};
    finish_report_function finish   = [](registry&) noexcept {};
};

class registry {
    // Contains all registered test cases.
    small_vector<impl::test_case, max_test_cases> test_list;

    // Contains all registered reporters.
    small_vector<registered_reporter, max_registered_reporters> registered_reporters;

public:
    enum class verbosity { quiet, normal, high, full } verbose = verbosity::normal;
    bool with_color                                            = true;

    using print_function             = snitch::print_function;
    using initialize_report_function = snitch::initialize_report_function;
    using configure_report_function  = snitch::configure_report_function;
    using report_function            = snitch::report_function;
    using finish_report_function     = snitch::finish_report_function;

    print_function         print_callback  = &snitch::impl::stdout_print;
    report_function        report_callback = &snitch::reporter::console::report;
    finish_report_function finish_callback = [](registry&) noexcept {};

    // Internal API; do not use.
    template<typename T>
    void append_or_print(small_string<max_message_length>& ss, T&& value) const noexcept {
        const std::size_t init_size = ss.size();
        if (append(ss, value)) {
            return;
        }

        ss.resize(init_size);
        this->print_callback(ss);
        ss.clear();

        if (append(ss, value)) {
            return;
        }

        if constexpr (std::is_convertible_v<std::decay_t<T>, std::string_view>) {
            ss.clear();
            this->print_callback(value);
        } else {
            this->print_callback(ss);
            ss.clear();
            static_cast<void>(append(ss, "..."));
        }
    }

    template<typename... Args>
    void print(Args&&... args) const noexcept {
        small_string<max_message_length> message;
        (append_or_print(message, std::forward<Args>(args)), ...);
        if (!message.empty()) {
            this->print_callback(message);
        }
    }

    template<convertible_to<std::string_view> T>
    void print(const T& str) const noexcept {
        this->print_callback(str);
    }

    // Requires: number of reporters + 1 <= max_registered_reporters.
    std::string_view add_reporter(
        std::string_view                                 name,
        const std::optional<initialize_report_function>& initialize,
        const std::optional<configure_report_function>&  configure,
        const report_function&                           report,
        const std::optional<finish_report_function>&     finish);

    // Internal API; do not use.
    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    const char* add_impl(const test_id& id, const source_location& location, impl::test_ptr func);

    // Internal API; do not use.
    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    const char*
    add(const impl::name_and_tags& id, const source_location& location, impl::test_ptr func);

    // Internal API; do not use.
    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename... Args, typename F>
    const char*
    add_with_types(const impl::name_and_tags& id, const source_location& location, const F& func) {
        return (
            add_impl(
                {id.name, id.tags, type_name<Args>}, location, impl::to_test_case_ptr<Args>(func)),
            ...);
    }

    // Internal API; do not use.
    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename T, typename F>
    const char* add_with_type_list(
        const impl::name_and_tags& id, const source_location& location, const F& func) {
        return [&]<template<typename...> typename TL, typename... Args>(type_list<TL<Args...>>) {
            return this->add_with_types<Args...>(id, location, func);
        }(type_list<T>{});
    }

    // Internal API; do not use.
    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    const char* add_fixture(
        const impl::fixture_name_and_tags& id,
        const source_location&             location,
        impl::test_ptr                     func);

    // Internal API; do not use.
    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename... Args, typename F>
    const char* add_fixture_with_types(
        const impl::fixture_name_and_tags& id, const source_location& location, const F& func) {
        return (
            add_impl(
                {id.name, id.tags, type_name<Args>, id.fixture}, location,
                impl::to_test_case_ptr<Args>(func)),
            ...);
    }

    // Internal API; do not use.
    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename T, typename F>
    const char* add_fixture_with_type_list(
        const impl::fixture_name_and_tags& id, const source_location& location, const F& func) {
        return [&]<template<typename...> typename TL, typename... Args>(type_list<TL<Args...>>) {
            return this->add_fixture_with_types<Args...>(id, location, func);
        }(type_list<T>{});
    }

    // Internal API; do not use.
    void report_assertion(
        bool                      success,
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message) const noexcept;

    // Internal API; do not use.
    void report_assertion(
        bool                      success,
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message1,
        std::string_view          message2) const noexcept;

    // Internal API; do not use.
    void report_assertion(
        bool                      success,
        impl::test_state&         state,
        const assertion_location& location,
        const impl::expression&   exp) const noexcept;

    // Internal API; do not use.
    void report_skipped(
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message) const noexcept;

    // Internal API; do not use.
    impl::test_state run(impl::test_case& test) noexcept;

    // Internal API; do not use.
    bool run_tests(std::string_view run_name) noexcept;

    // Internal API; do not use.
    bool run_selected_tests(
        std::string_view                                     run_name,
        const filter_info&                                   filter_strings,
        const small_function<bool(const test_id&) noexcept>& filter) noexcept;

    bool run_tests(const cli::input& args) noexcept;

    void configure(const cli::input& args) noexcept;

    void list_all_tests() const noexcept;

    // Requires: number unique tags <= max_unique_tags.
    void list_all_tags() const;

    void list_tests_with_tag(std::string_view tag) const noexcept;

    void list_all_reporters() const noexcept;

    small_vector_span<impl::test_case>       test_cases() noexcept;
    small_vector_span<const impl::test_case> test_cases() const noexcept;

    small_vector_span<registered_reporter>       reporters() noexcept;
    small_vector_span<const registered_reporter> reporters() const noexcept;
};

extern constinit registry tests;
} // namespace snitch

#endif
