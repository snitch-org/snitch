#ifndef SNITCH_REGISTRY_HPP
#define SNITCH_REGISTRY_HPP

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_cli.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_console.hpp"
#include "snitch/snitch_error_handling.hpp"
#include "snitch/snitch_expression.hpp"
#include "snitch/snitch_function.hpp"
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
} // namespace snitch

namespace snitch::impl {
void default_reporter(const registry& r, const event::data& event) noexcept;

template<typename T, typename F>
constexpr test_ptr to_test_case_ptr(const F&) noexcept {
    return []() { F{}.template operator()<T>(); };
}

struct abort_exception {};
} // namespace snitch::impl

namespace snitch {
template<typename... Args>
struct type_list {};

enum class filter_result { included, excluded, not_included, not_excluded };

[[nodiscard]] filter_result
is_filter_match_name(std::string_view name, std::string_view filter) noexcept;

[[nodiscard]] filter_result
is_filter_match_tags(std::string_view tags, std::string_view filter) noexcept;

[[nodiscard]] filter_result is_filter_match_id(const test_id& id, std::string_view filter) noexcept;

class registry {
    small_vector<impl::test_case, max_test_cases> test_list;

public:
    enum class verbosity { quiet, normal, high } verbose = verbosity::normal;
    bool with_color                                      = true;

    using print_function  = small_function<void(std::string_view) noexcept>;
    using report_function = small_function<void(const registry&, const event::data&) noexcept>;

    print_function  print_callback  = &snitch::impl::stdout_print;
    report_function report_callback = &snitch::impl::default_reporter;

    template<typename... Args>
    void print(Args&&... args) const noexcept {
        small_string<max_message_length> message;
        const bool                       could_fit = append(message, std::forward<Args>(args)...);
        this->print_callback(message);
        if (!could_fit) {
            this->print_callback("...");
        }
    }

    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    const char* add(const test_id& id, impl::test_ptr func);

    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename... Args, typename F>
    const char* add_with_types(std::string_view name, std::string_view tags, const F& func) {
        return (add({name, tags, type_name<Args>}, impl::to_test_case_ptr<Args>(func)), ...);
    }

    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename T, typename F>
    const char* add_with_type_list(std::string_view name, std::string_view tags, const F& func) {
        return [&]<template<typename...> typename TL, typename... Args>(type_list<TL<Args...>>) {
            return this->add_with_types<Args...>(name, tags, func);
        }(type_list<T>{});
    }

    void report_assertion(
        bool                      critical,
        bool                      success,
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message) const;

    void report_assertion(
        bool                      critical,
        bool                      success,
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message1,
        std::string_view          message2) const;

    void report_assertion(
        bool                      critical,
        bool                      success,
        impl::test_state&         state,
        const assertion_location& location,
        const impl::expression&   exp) const;

    void report_skipped(
        bool                      critical,
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message) const;

    impl::test_state run(impl::test_case& test) noexcept;

    bool run_tests(std::string_view run_name) noexcept;

    bool run_selected_tests(
        std::string_view                                     run_name,
        const small_function<bool(const test_id&) noexcept>& filter) noexcept;

    bool run_tests(const cli::input& args) noexcept;

    void configure(const cli::input& args) noexcept;

    void list_all_tests() const noexcept;

    // Requires: number unique tags <= max_unique_tags.
    void list_all_tags() const;

    void list_tests_with_tag(std::string_view tag) const noexcept;

    impl::test_case*       begin() noexcept;
    impl::test_case*       end() noexcept;
    const impl::test_case* begin() const noexcept;
    const impl::test_case* end() const noexcept;
};

extern constinit registry tests;
} // namespace snitch

#endif
