#ifndef SNITCH_REGISTRY_HPP
#define SNITCH_REGISTRY_HPP

#include "snitch/snitch_any.hpp"
#include "snitch/snitch_append.hpp"
#include "snitch/snitch_cli.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_console.hpp"
#include "snitch/snitch_error_handling.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_file.hpp"
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
// Maximum size of a reporter instance, in bytes.
constexpr std::size_t max_reporter_size_bytes = SNITCH_MAX_REPORTER_SIZE_BYTES;
// Is snitch disabled?
constexpr bool is_enabled = SNITCH_ENABLE;
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

SNITCH_EXPORT std::string_view
make_full_name(small_string<max_test_name_length>& buffer, const test_id& id) noexcept;

template<typename T, typename F>
constexpr test_ptr to_test_case_ptr(const F&) noexcept {
    return []() { F{}.template operator()<T>(); };
}

struct abort_exception {};

SNITCH_EXPORT bool parse_colour_mode_option(registry& reg, std::string_view color_option) noexcept;
SNITCH_EXPORT bool parse_color_option(registry& reg, std::string_view color_option) noexcept;
} // namespace snitch::impl

namespace snitch {
template<typename... Args>
struct type_list {};

struct filter_result {
    bool included = false;
    bool implicit = false;
};

SNITCH_EXPORT [[nodiscard]] filter_result
filter_result_and(filter_result first, filter_result second) noexcept;

SNITCH_EXPORT [[nodiscard]] filter_result
filter_result_or(filter_result first, filter_result second) noexcept;

SNITCH_EXPORT [[nodiscard]] filter_result
is_filter_match_name(std::string_view name, std::string_view filter) noexcept;

SNITCH_EXPORT [[nodiscard]] filter_result
is_filter_match_tags(std::string_view tags, std::string_view filter) noexcept;

SNITCH_EXPORT [[nodiscard]] filter_result
is_filter_match_id(std::string_view name, std::string_view tags, std::string_view filter) noexcept;

using print_function  = function_ref<void(std::string_view) noexcept>;
using report_function = function_ref<void(const registry&, const event::data&) noexcept>;
using configure_report_function =
    function_ref<bool(registry&, std::string_view, std::string_view) noexcept>;
using initialize_report_function = function_ref<void(registry&) noexcept>;
using finish_report_function     = function_ref<void(registry&) noexcept>;

struct registered_reporter {
    std::string_view           name;
    initialize_report_function initialize = [](registry&) noexcept {};
    configure_report_function  configure =
        [](registry&, std::string_view, std::string_view) noexcept { return false; };
    report_function        callback = [](const registry&, const event::data&) noexcept {};
    finish_report_function finish   = [](registry&) noexcept {};
};

template<typename T>
concept reporter_type = requires(registry& reg) {
    T{reg};
} && requires(T& rep, registry& reg, std::string_view k, std::string_view v) {
    { rep.configure(reg, k, v) } -> convertible_to<bool>;
} && requires(T& rep, const registry& reg, const event::data& e) { rep.report(reg, e); };

class registry {
    // Contains all registered test cases.
    small_vector<impl::test_case, max_test_cases> test_list;

    // Contains all registered reporters.
    small_vector<registered_reporter, max_registered_reporters> registered_reporters;

    // Used when writing output to file.
    std::optional<impl::file_writer> file_writer;

    // Type-erased storage for the current reporter instance.
    inplace_any<max_reporter_size_bytes> reporter_storage;

    template<typename T>
    void initialize_reporter(registry&) noexcept {
        this->reporter_storage.emplace<T>(*this);
    }

    template<typename T>
    void report(const registry&, const event::data& e) noexcept {
        this->reporter_storage.get<T>().report(*this, e);
    }

    template<typename T>
    bool configure_reporter(registry&, std::string_view k, std::string_view v) noexcept {
        return this->reporter_storage.get<T>().configure(*this, k, v);
    }

    SNITCH_EXPORT void destroy_reporter(registry&) noexcept;

    SNITCH_EXPORT void report_default(const registry&, const event::data& e) noexcept;

public:
    enum class verbosity { quiet, normal, high, full } verbose = verbosity::normal;
    bool with_color                                            = SNITCH_DEFAULT_WITH_COLOR == 1;

    using print_function             = snitch::print_function;
    using initialize_report_function = snitch::initialize_report_function;
    using configure_report_function  = snitch::configure_report_function;
    using report_function            = snitch::report_function;
    using finish_report_function     = snitch::finish_report_function;

    print_function         print_callback  = &snitch::impl::stdout_print;
    report_function        report_callback = {*this, constant<&registry::report_default>{}};
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
    SNITCH_EXPORT std::string_view add_reporter(
        std::string_view                                 name,
        const std::optional<initialize_report_function>& initialize,
        const std::optional<configure_report_function>&  configure,
        const report_function&                           report,
        const std::optional<finish_report_function>&     finish);

    // Requires: number of reporters + 1 <= max_registered_reporters.
    template<reporter_type T>
    std::string_view add_reporter(std::string_view name) {
        return this->add_reporter(
            name, initialize_report_function{*this, constant<&registry::initialize_reporter<T>>{}},
            configure_report_function{*this, constant<&registry::configure_reporter<T>>{}},
            report_function{*this, constant<&registry::report<T>>{}},
            finish_report_function{*this, constant<&registry::destroy_reporter>{}});
    }

    // Internal API; do not use.
    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    SNITCH_EXPORT const char*
    add_impl(const test_id& id, const source_location& location, impl::test_ptr func);

    // Internal API; do not use.
    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    SNITCH_EXPORT const char*
    add(const impl::name_and_tags& id, const source_location& location, impl::test_ptr func);

    // Internal API; do not use.
    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename... Args, typename F>
    const char*
    add_with_types(const impl::name_and_tags& id, const source_location& location, const F& func) {
        static_assert(sizeof...(Args) > 0, "empty type list in TEMPLATE_TEST_CASE");
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
            static_assert(sizeof...(Args) > 0, "empty type list in TEMPLATE_LIST_TEST_CASE");
            return this->add_with_types<Args...>(id, location, func);
        }(type_list<T>{});
    }

    // Internal API; do not use.
    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    SNITCH_EXPORT const char* add_fixture(
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
    SNITCH_EXPORT static void report_assertion(bool success, std::string_view message) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT static void
    report_assertion(bool success, std::string_view message1, std::string_view message2) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT static void report_assertion(bool success, const impl::expression& exp) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT static void report_skipped(std::string_view message) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT static void report_section_started(const section& sec) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT static void report_section_ended(const section& sec) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT impl::test_state run(impl::test_case& test) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT bool run_tests(std::string_view run_name) noexcept;

    // Internal API; do not use.
    SNITCH_EXPORT bool run_selected_tests(
        std::string_view                                   run_name,
        const filter_info&                                 filter_strings,
        const function_ref<bool(const test_id&) noexcept>& filter) noexcept;

    SNITCH_EXPORT bool run_tests(const cli::input& args) noexcept;

    // Requires: output file path (if configured) is valid
    SNITCH_EXPORT void configure(const cli::input& args);

    SNITCH_EXPORT void list_all_tests() const noexcept;

    // Requires: number unique tags <= max_unique_tags.
    SNITCH_EXPORT void list_all_tags() const;

    SNITCH_EXPORT void list_tests_with_tag(std::string_view tag) const noexcept;

    SNITCH_EXPORT void list_all_reporters() const noexcept;

    SNITCH_EXPORT small_vector_span<impl::test_case> test_cases() noexcept;
    SNITCH_EXPORT small_vector_span<const impl::test_case> test_cases() const noexcept;

    SNITCH_EXPORT small_vector_span<registered_reporter> reporters() noexcept;
    SNITCH_EXPORT small_vector_span<const registered_reporter> reporters() const noexcept;
};

SNITCH_EXPORT extern constinit registry tests;
} // namespace snitch

#endif
