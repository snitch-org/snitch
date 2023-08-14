#include "snitch/snitch_reporter_console.hpp"

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_console.hpp"
#include "snitch/snitch_macros_reporter.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"
#include "snitch/snitch_test_data.hpp"

#include <string_view>

namespace snitch::reporter::console {
namespace {
using namespace std::literals;
using namespace snitch::impl;

void print_location(
    const registry&           r,
    const test_id&            id,
    const section_info&       sections,
    const capture_info&       captures,
    const assertion_location& location) noexcept {

    r.print("running test case \"", make_colored(id.name, r.with_color, color::highlight1), "\"\n");

    for (auto& section : sections) {
        r.print(
            "          in section \"",
            make_colored(section.id.name, r.with_color, color::highlight1), "\"\n");
    }

    r.print("          at ", location.file, ":", location.line, "\n");

    if (!id.type.empty()) {
        r.print(
            "          for type ", make_colored(id.type, r.with_color, color::highlight1), "\n");
    }

    for (auto& capture : captures) {
        r.print("          with ", make_colored(capture, r.with_color, color::highlight1), "\n");
    }
}

void print_message(const registry& r, const assertion_data& data) {
    constexpr auto indent = "          "sv;
    std::visit(
        overload{
            [&](std::string_view message) {
                r.print(indent, make_colored(message, r.with_color, color::highlight2));
            },
            [&](const expression_info& exp) {
                small_string<max_message_length> message_buffer;
                append_or_truncate(message_buffer, exp.type, "(", exp.expected, ")");
                r.print(
                    indent, make_colored(message_buffer.str(), r.with_color, color::highlight2));

                if (!exp.actual.empty()) {
                    if (exp.expected.size() + exp.type.size() + 3 > 64) {
                        r.print(
                            "\n", indent,
                            "got: ", make_colored(exp.actual, r.with_color, color::highlight2));
                    } else {
                        r.print(
                            ", got: ", make_colored(exp.actual, r.with_color, color::highlight2));
                    }
                }
            }},
        data);
}

struct default_reporter_functor {
    const registry& r;

    void operator()(const snitch::event::test_run_started& e) const noexcept {
        r.print(
            make_colored("starting ", r.with_color, color::highlight2),
            make_colored(e.name, r.with_color, color::highlight1),
            make_colored(" with ", r.with_color, color::highlight2),
            make_colored("snitch v" SNITCH_FULL_VERSION "\n", r.with_color, color::highlight1));
        r.print("==========================================\n");
    }

    void operator()(const snitch::event::test_run_ended& e) const noexcept {
        r.print("==========================================\n");

        if (e.success) {
            r.print(
                make_colored("success:", r.with_color, color::pass), " all tests passed (",
                e.run_count, " test cases, ", e.assertion_count, " assertions");
        } else {
            r.print(
                make_colored("error:", r.with_color, color::fail), " some tests failed (",
                e.fail_count, " out of ", e.run_count, " test cases, ", e.assertion_count,
                " assertions");
        }

        if (e.skip_count > 0) {
            r.print(", ", e.skip_count, " test cases skipped");
        }

#if SNITCH_WITH_TIMINGS
        r.print(", ", e.duration, " seconds");
#endif

        r.print(")\n");
    }

    void operator()(const snitch::event::test_case_started& e) const noexcept {
        small_string<max_test_name_length> full_name;
        make_full_name(full_name, e.id);

        r.print(
            make_colored("starting:", r.with_color, color::status), " ",
            make_colored(full_name, r.with_color, color::highlight1), " at ", e.location.file, ":",
            e.location.line);
        r.print("\n");
    }

    void operator()(const snitch::event::test_case_ended& e) const noexcept {
        small_string<max_test_name_length> full_name;
        make_full_name(full_name, e.id);

#if SNITCH_WITH_TIMINGS
        r.print(
            make_colored("finished:", r.with_color, color::status), " ",
            make_colored(full_name, r.with_color, color::highlight1), " (", e.duration, "s)");
#else
        r.print(
            make_colored("finished:", r.with_color, color::status), " ",
            make_colored(full_name, r.with_color, color::highlight1));
#endif
        r.print("\n");
    }

    void operator()(const snitch::event::test_case_skipped& e) const noexcept {
        r.print(make_colored("skipped: ", r.with_color, color::skipped));
        print_location(r, e.id, e.sections, e.captures, e.location);
        r.print("          ", make_colored(e.message, r.with_color, color::highlight2));
        r.print("\n");
    }

    void operator()(const snitch::event::assertion_failed& e) const noexcept {
        if (e.expected) {
            r.print(make_colored("expected failure: ", r.with_color, color::pass));
        } else {
            r.print(make_colored("failed: ", r.with_color, color::fail));
        }
        print_location(r, e.id, e.sections, e.captures, e.location);
        print_message(r, e.data);
        r.print("\n");
    }

    void operator()(const snitch::event::assertion_succeeded& e) const noexcept {
        r.print(make_colored("passed: ", r.with_color, color::pass));
        print_location(r, e.id, e.sections, e.captures, e.location);
        print_message(r, e.data);
        r.print("\n");
    }
};
} // namespace

void initialize(registry&) noexcept {}

bool configure(registry& r, std::string_view option, std::string_view value) noexcept {
    if (option == "color") {
        parse_color_option(r, value);
        return true;
    }
    if (option == "colour-mode") {
        parse_colour_mode_option(r, value);
        return true;
    }

    return false;
}

void report(const registry& r, const event::data& event) noexcept {
    std::visit(default_reporter_functor{r}, event);
}
} // namespace snitch::reporter::console

SNITCH_REGISTER_REPORTER_CALLBACKS(
    "console",
    &snitch::reporter::console::initialize,
    &snitch::reporter::console::configure,
    &snitch::reporter::console::report,
    {});
