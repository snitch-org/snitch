#include "snitch/snitch_reporter_console.hpp"

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_console.hpp"
#include "snitch/snitch_macros_reporter.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"
#include "snitch/snitch_test_data.hpp"

namespace snitch::reporter::console {
namespace {
using namespace std::literals;
using namespace snitch::impl;

std::string_view locatation_label(location_type type) {
    switch (type) {
    case location_type::exact: return "at";
    case location_type::section_scope: return "somewhere inside section at";
    case location_type::test_case_scope: return "somewhere inside test case at";
    case location_type::in_check: return "somewhere inside check at";
    default: return "at";
    }
}

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

    r.print(
        "          ", locatation_label(location.type), " ", location.file, ":", location.line,
        "\n");

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
                r.print(indent, make_colored(message, r.with_color, color::highlight2), "\n");
            },
            [&](const expression_info& exp) {
                small_string<max_message_length> message_buffer;
                append_or_truncate(message_buffer, exp.type, "(", exp.expected, ")");
                r.print(
                    indent, make_colored(message_buffer.str(), r.with_color, color::highlight2));

                constexpr std::size_t long_line_threshold = 64;
                if (!exp.actual.empty()) {
                    if (exp.expected.size() + exp.type.size() + 3 > long_line_threshold ||
                        exp.actual.size() + 5 > long_line_threshold) {
                        r.print(
                            "\n", indent,
                            "got: ", make_colored(exp.actual, r.with_color, color::highlight2),
                            "\n");
                    } else {
                        r.print(
                            ", got: ", make_colored(exp.actual, r.with_color, color::highlight2),
                            "\n");
                    }
                } else {
                    r.print("\n");
                }
            }},
        data);
}
} // namespace

reporter::reporter(registry&) noexcept {}

bool reporter::configure(registry& r, std::string_view option, std::string_view value) noexcept {
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

void reporter::report(const registry& r, const event::data& event) noexcept {
    std::visit(
        snitch::overload{
            [&](const snitch::event::test_run_started& e) {
                r.print(
                    make_colored("starting ", r.with_color, color::highlight2),
                    make_colored(e.name, r.with_color, color::highlight1),
                    make_colored(" with ", r.with_color, color::highlight2),
                    make_colored(
                        "snitch v" SNITCH_FULL_VERSION "\n", r.with_color, color::highlight1));
                r.print("==========================================\n");
            },
            [&](const snitch::event::test_run_ended& e) {
                r.print("==========================================\n");

                if (e.success) {
                    r.print(
                        make_colored("success:", r.with_color, color::pass), " all tests passed (",
                        e.run_count, " test cases, ", e.assertion_count, " assertions");
                } else {
                    r.print(
                        make_colored("error:", r.with_color, color::fail), " ",
                        (e.fail_count == e.run_count ? "all" : "some"), " tests failed (",
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
            },
            [&](const snitch::event::test_case_started& e) {
                small_string<max_test_name_length> full_name;
                make_full_name(full_name, e.id);

                r.print(
                    make_colored("starting:", r.with_color, color::status), " ",
                    make_colored(full_name, r.with_color, color::highlight1), " at ",
                    e.location.file, ":", e.location.line, "\n");
            },
            [&](const snitch::event::test_case_ended& e) {
                small_string<max_test_name_length> full_name;
                make_full_name(full_name, e.id);

#if SNITCH_WITH_TIMINGS
                r.print(
                    make_colored("finished:", r.with_color, color::status), " ",
                    make_colored(full_name, r.with_color, color::highlight1), " (", e.duration,
                    "s)\n");
#else
                r.print(
                    make_colored("finished:", r.with_color, color::status), " ",
                    make_colored(full_name, r.with_color, color::highlight1), "\n");
#endif
            },
            [&](const snitch::event::section_started& e) {
                r.print(
                    make_colored("entering section:", r.with_color, color::status), " ",
                    make_colored(e.id.name, r.with_color, color::highlight1), " at ",
                    e.location.file, ":", e.location.line, "\n");
            },
            [&](const snitch::event::section_ended& e) {
                r.print(
                    make_colored("leaving section:", r.with_color, color::status), " ",
                    make_colored(e.id.name, r.with_color, color::highlight1), "\n");
            },
            [&](const snitch::event::test_case_skipped& e) {
                r.print(make_colored("skipped: ", r.with_color, color::skipped));
                print_location(r, e.id, e.sections, e.captures, e.location);
                r.print(
                    "          ", make_colored(e.message, r.with_color, color::highlight2), "\n");
            },
            [&](const snitch::event::assertion_failed& e) {
                if (e.expected) {
                    r.print(make_colored("expected failure: ", r.with_color, color::pass));
                } else if (e.allowed) {
                    r.print(make_colored("allowed failure: ", r.with_color, color::pass));
                } else {
                    r.print(make_colored("failed: ", r.with_color, color::fail));
                }
                print_location(r, e.id, e.sections, e.captures, e.location);
                print_message(r, e.data);
            },
            [&](const snitch::event::assertion_succeeded& e) {
                r.print(make_colored("passed: ", r.with_color, color::pass));
                print_location(r, e.id, e.sections, e.captures, e.location);
                print_message(r, e.data);
            },
            [&](const snitch::event::list_test_run_started&) {
                r.print("Matching test cases:\n");
                counter = 0;
            },
            [&](const snitch::event::list_test_run_ended&) {
                r.print(counter, " matching test cases\n");
            },
            [&](const snitch::event::test_case_listed& e) {
                small_string<max_test_name_length> full_name;
                ++counter;
                make_full_name(full_name, e.id);
                r.print("  ", full_name, "\n");
                if (!e.id.tags.empty()) {
                    r.print("      ", e.id.tags, "\n");
                }
            }},
        event);
}
} // namespace snitch::reporter::console

SNITCH_REGISTER_REPORTER("console", snitch::reporter::console::reporter);
