#include "snitch/snitch_config.hpp"

#if defined(SNITCH_WITH_TEAMCITY_REPORTER) || defined(SNITCH_WITH_ALL_REPORTERS)

#    include "snitch/snitch_macros_reporter.hpp"
#    include "snitch/snitch_registry.hpp"
#    include "snitch/snitch_reporter_teamcity.hpp"
#    include "snitch/snitch_string.hpp"
#    include "snitch/snitch_string_utility.hpp"
#    include "snitch/snitch_test_data.hpp"

#    include <initializer_list>

namespace snitch::reporter::teamcity {
namespace {
struct assertion {
    const snitch::assertion_location& location;
    const snitch::section_info&       sections;
    const snitch::capture_info&       captures;
    const snitch::assertion_data&     data;
};

struct key_value {
    std::string_view                          key;
    std::variant<std::string_view, assertion> value;
};

bool escape(small_string_span string) noexcept {
    return escape_all_or_truncate(string, "|", "||") && escape_all_or_truncate(string, "'", "|'") &&
           escape_all_or_truncate(string, "\n", "|n") &&
           escape_all_or_truncate(string, "\r", "|r") &&
           escape_all_or_truncate(string, "[", "|[") && escape_all_or_truncate(string, "]", "|]");
}

template<typename T>
std::string_view make_escaped(small_string_span buffer, const T& value) noexcept {
    buffer.clear();
    append_or_truncate(buffer, value);
    escape(buffer);
    return std::string_view{buffer.data(), buffer.size()};
}

void print_assertion(const registry& r, const assertion& msg) noexcept {
    small_string<max_message_length> buffer;

    r.print("'", make_escaped(buffer, msg.location.file), ":", msg.location.line, "|n");
    for (const auto& s : msg.sections) {
        r.print(make_escaped(buffer, s.id.name), "|n");
    }
    for (const auto& c : msg.captures) {
        r.print(make_escaped(buffer, c), "|n");
    }

    constexpr std::string_view indent = "  ";

    std::visit(
        overload{
            [&](std::string_view message) { r.print(indent, make_escaped(buffer, message), "'"); },
            [&](const snitch::expression_info& exp) {
                r.print(indent, exp.type, "(", make_escaped(buffer, exp.expected), ")");

                constexpr std::size_t long_line_threshold = 64;
                if (!exp.actual.empty()) {
                    if (exp.expected.size() + exp.type.size() + 3 > long_line_threshold ||
                        exp.actual.size() + 5 > long_line_threshold) {
                        r.print("|n", indent, "got: ", make_escaped(buffer, exp.actual), "'");
                    } else {
                        r.print(", got: ", make_escaped(buffer, exp.actual), "'");
                    }
                } else {
                    r.print("'");
                }
            }},
        msg.data);
}

void send_message(
    const registry& r, std::string_view message, std::initializer_list<key_value> args) noexcept {
    constexpr std::string_view teamcity_header = "##teamCity[";
    constexpr std::string_view teamcity_footer = "]\n";

    r.print(teamcity_header, message);
    for (const auto& arg : args) {
        r.print(" ", arg.key, "=");
        std::visit(
            snitch::overload{
                [&](std::string_view msg) { r.print("'", msg, "'"); },
                [&](const assertion& msg) { print_assertion(r, msg); }},
            arg.value);
    }
    r.print(teamcity_footer);
}

small_string<max_message_length>
make_suite_name(std::string_view app, const filter_info& filters) noexcept {
    small_string<max_message_length> name;
    append_or_truncate(name, app);
    for (const auto& filter : filters) {
        append_or_truncate(name, " \"", filter, "\"");
    }
    escape(name);
    return name;
}

small_string<max_test_name_length> make_full_name(const test_id& id) noexcept {
    small_string<max_test_name_length> name;
    snitch::impl::make_full_name(name, id);
    escape(name);
    return name;
}

constexpr std::size_t max_duration_length = 32;

#    if SNITCH_WITH_TIMINGS
small_string<max_duration_length> make_duration(float duration) noexcept {
    small_string<max_duration_length> string;
    append_or_truncate(string, static_cast<std::size_t>(duration * 1e6f));
    return string;
}
#    endif
} // namespace

void initialize(registry& r) noexcept {
    // TeamCity needs test_case_started and test_case_ended events, which are only printed on
    // verbosity 'high', so ensure the requested verbosity is at least as much.
    r.verbose = r.verbose < registry::verbosity::high ? registry::verbosity::high : r.verbose;
}

void report(const registry& r, const snitch::event::data& event) noexcept {
    std::visit(
        snitch::overload{
            [&](const snitch::event::test_run_started& e) {
                send_message(r, "testSuiteStarted", {{"name", make_suite_name(e.name, e.filters)}});
            },
            [&](const snitch::event::test_run_ended& e) {
                send_message(
                    r, "testSuiteFinished", {{"name", make_suite_name(e.name, e.filters)}});
            },
            [&](const snitch::event::test_case_started& e) {
                send_message(r, "testStarted", {{"name", make_full_name(e.id)}});
            },
            [&](const snitch::event::test_case_ended& e) {
#    if SNITCH_WITH_TIMINGS
                send_message(
                    r, "testFinished",
                    {{"name", make_full_name(e.id)}, {"duration", make_duration(e.duration)}});
#    else
                send_message(r, "testFinished", {{"name", make_full_name(e.id)}});
#    endif
            },
            [&](const snitch::event::section_started&) {},
            [&](const snitch::event::section_ended&) {},
            [&](const snitch::event::test_case_skipped& e) {
                send_message(
                    r, "testIgnored",
                    {{"name", make_full_name(e.id)},
                     {"message", assertion{e.location, e.sections, e.captures, e.message}}});
            },
            [&](const snitch::event::assertion_failed& e) {
                send_message(
                    r, e.expected || e.allowed ? "testStdOut" : "testFailed",
                    {{"name", make_full_name(e.id)},
                     {e.expected || e.allowed ? "out" : "message",
                      assertion{e.location, e.sections, e.captures, e.data}}});
            },
            [&](const snitch::event::assertion_succeeded& e) {
                send_message(
                    r, "testStdOut",
                    {{"name", make_full_name(e.id)},
                     {"out", assertion{e.location, e.sections, e.captures, e.data}}});
            },
            [&](const snitch::event::list_test_run_started&) {},
            [&](const snitch::event::list_test_run_ended&) {},
            [&](const snitch::event::test_case_listed& e) { r.print(make_full_name(e.id), "\n"); }},
        event);
}
} // namespace snitch::reporter::teamcity

SNITCH_REGISTER_REPORTER_CALLBACKS(
    "teamcity",
    &snitch::reporter::teamcity::initialize,
    {},
    &snitch::reporter::teamcity::report,
    {});

#endif
