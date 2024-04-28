#include "snitch/snitch_config.hpp"

#if defined(SNITCH_WITH_CATCH2_XML_REPORTER) || defined(SNITCH_WITH_ALL_REPORTERS)

#    include "snitch/snitch_macros_reporter.hpp"
#    include "snitch/snitch_registry.hpp"
#    include "snitch/snitch_reporter_catch2_xml.hpp"
#    include "snitch/snitch_string.hpp"
#    include "snitch/snitch_string_utility.hpp"
#    include "snitch/snitch_test_data.hpp"

#    include <initializer_list>

namespace snitch::reporter::catch2_xml {
namespace {
struct key_value {
    std::string_view key;
    std::string_view value;
};

bool escape(small_string_span string) noexcept {
    return escape_all_or_truncate(string, "&", "&amp;") &&
           escape_all_or_truncate(string, "\"", "&quot;") &&
           escape_all_or_truncate(string, "'", "&apos;") &&
           escape_all_or_truncate(string, "<", "&lt;") &&
           escape_all_or_truncate(string, ">", "&gt;");
}

small_string<max_message_length> make_escaped(std::string_view string) noexcept {
    small_string<max_message_length> escaped_string;
    append_or_truncate(escaped_string, string);
    escape(escaped_string);
    return escaped_string;
}

small_string<max_test_name_length> make_full_name(const test_id& id) noexcept {
    small_string<max_test_name_length> name;
    snitch::impl::make_full_name(name, id);
    escape(name);
    return name;
}

small_string<max_message_length> make_filters(const filter_info& filters) noexcept {
    small_string<max_message_length> filter_string;

    bool first = true;
    for (const auto& filter : filters) {
        append_or_truncate(filter_string, (first ? "\"" : " \""), filter, "\"");
        first = false;
    }

    escape(filter_string);
    return filter_string;
}

constexpr std::size_t max_number_length = 32;

template<typename T>
small_string<max_number_length> make_string(T number) noexcept {
    small_string<max_number_length> string;
    append_or_truncate(string, number);
    return string;
}

std::string_view get_indent(const reporter& rep) noexcept {
    constexpr std::string_view spaces            = "                ";
    constexpr std::size_t      spaces_per_indent = 2;
    return spaces.substr(0, std::min(spaces.size(), spaces_per_indent * rep.indent_level));
}

void close(reporter& rep, const registry& r, std::string_view name) noexcept {
    --rep.indent_level;
    r.print(get_indent(rep), "</", name, ">\n");
}

void print(const reporter& rep, const registry& r, std::string_view data) noexcept {
    r.print(get_indent(rep), data, "\n");
}

void open(
    reporter&                        rep,
    const registry&                  r,
    std::string_view                 name,
    std::initializer_list<key_value> args = {}) noexcept {

    r.print(get_indent(rep), "<", name);
    for (const auto& arg : args) {
        r.print(" ", arg.key, "=\"", arg.value, "\"");
    }
    r.print(">\n");
    ++rep.indent_level;
}

void node(
    const reporter&                  rep,
    const registry&                  r,
    std::string_view                 name,
    std::initializer_list<key_value> args = {}) noexcept {

    r.print(get_indent(rep), "<", name);
    for (const auto& arg : args) {
        r.print(" ", arg.key, "=\"", arg.value, "\"");
    }
    r.print("/>\n");
}

void open_close(
    const reporter&  rep,
    const registry&  r,
    std::string_view name,
    std::string_view content) noexcept {

    if (content.empty()) {
        node(rep, r, name);
    } else {
        r.print(get_indent(rep), "<", name, ">", content, "</", name, ">\n");
    }
}

template<typename T>
void report_assertion(reporter& rep, const registry& r, const T& e, bool success) noexcept {
    for (const auto& c : e.captures) {
        open(rep, r, "Info");
        print(rep, r, make_escaped(c));
        close(rep, r, "Info");
    }

    std::visit(
        overload{
            [&](std::string_view message) {
                open(
                    rep, r, success ? "Success" : "Failure",
                    {{"filename", make_escaped(e.location.file)},
                     {"line", make_string(e.location.line)}});
                print(rep, r, make_escaped(message));
                close(rep, r, success ? "Success" : "Failure");
            },
            [&](const snitch::expression_info& exp) {
                open(
                    rep, r, "Expression",
                    {{"success", success ? "true" : "false"},
                     {"type", exp.type},
                     {"filename", make_escaped(e.location.file)},
                     {"line", make_string(e.location.line)}});

                open(rep, r, "Original");
                print(rep, r, make_escaped(exp.expected));
                close(rep, r, "Original");

                open(rep, r, "Expanded");
                if (!exp.actual.empty()) {
                    print(rep, r, make_escaped(exp.actual));
                } else {
                    print(rep, r, make_escaped(exp.expected));
                }
                close(rep, r, "Expanded");

                close(rep, r, "Expression");
            }},
        e.data);
}
} // namespace

reporter::reporter(registry& r) noexcept {
    // The XML reporter needs test_case_started and test_case_ended events, which are only
    // printed on verbosity 'high', so ensure the requested verbosity is at least as much.
    r.verbose = r.verbose < registry::verbosity::high ? registry::verbosity::high : r.verbose;
}

bool reporter::configure(registry&, std::string_view, std::string_view) noexcept {
    // No configuration
    return false;
}

void reporter::report(const registry& r, const snitch::event::data& event) noexcept {
    std::visit(
        snitch::overload{
            [&](const snitch::event::test_run_started& e) {
                print(*this, r, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
                // TODO: missing rng-seed
                open(
                    *this, r, "Catch2TestRun",
                    {{"name", make_escaped(e.name)},
                     {"rng-seed", "0"},
                     {"xml-format-version", "2"},
                     {"catch2-version", SNITCH_FULL_VERSION ".snitch"},
                     {"filters", make_filters(e.filters)}});
            },
            [&](const snitch::event::test_run_ended& e) {
                node(
                    *this, r, "OverallResults",
                    {{"successes", make_string(
                                       e.assertion_count - e.assertion_failure_count -
                                       e.allowed_assertion_failure_count)},
                     {"failures", make_string(e.assertion_failure_count)},
                     {"expectedFailures", make_string(e.allowed_assertion_failure_count)},
                     {"skips", make_string(e.skip_count)}});

                node(
                    *this, r, "OverallResultsCases",
                    {{"successes", make_string(e.run_count - e.fail_count - e.allowed_fail_count)},
                     {"failures", make_string(e.fail_count)},
                     {"expectedFailures", make_string(e.allowed_fail_count)},
                     {"skips", make_string(e.skip_count)}});

                close(*this, r, "Catch2TestRun");
            },
            [&](const snitch::event::test_case_started& e) {
                open(
                    *this, r, "TestCase",
                    {{"name", make_full_name(e.id)},
                     {"tags", make_escaped(e.id.tags)},
                     {"filename", make_escaped(e.location.file)},
                     {"line", make_string(e.location.line)}});
            },
            [&](const snitch::event::test_case_ended& e) {
#    if SNITCH_WITH_TIMINGS
                node(
                    *this, r, "OverallResult",
                    {{"success", e.state == test_case_state::failed ? "false" : "true"},
                     {"skips", e.state == test_case_state::skipped ? "1" : "0"},
                     {"durationInSeconds", make_string(e.duration)}});
#    else
                node(
                    *this, r, "OverallResult",
                    {{"success", e.state == test_case_state::failed ? "false" : "true"},
                     {"skips", e.state == test_case_state::skipped ? "1" : "0"}});
#    endif
                close(*this, r, "TestCase");
            },
            [&](const snitch::event::section_started& e) {
                open(
                    *this, r, "Section",
                    {{"name", make_escaped(e.id.name)},
                     {"filename", make_escaped(e.location.file)},
                     {"line", make_string(e.location.line)}});
            },
            [&](const snitch::event::section_ended& e) {
                node(
                    *this, r, "OverallResults",
                    {{"successes", make_string(
                                       e.assertion_count - e.assertion_failure_count -
                                       e.allowed_assertion_failure_count)},
                     {"failures", make_string(e.assertion_failure_count)},
                     {"expectedFailures", make_string(e.allowed_assertion_failure_count)},
                     {"skipped", e.skipped?"true":"false"}
#    if SNITCH_WITH_TIMINGS
                     ,
                     {"durationInSeconds", make_string(e.duration)}
#    endif
                    });
                close(*this, r, "Section");
            },
            [&](const snitch::event::test_case_skipped& e) {
                open(
                    *this, r, "Skip",
                    {{"filename", make_escaped(e.location.file)},
                     {"line", make_string(e.location.line)}});
                print(*this, r, e.message);
                close(*this, r, "Skip");
            },
            [&](const snitch::event::assertion_failed& e) { report_assertion(*this, r, e, false); },
            [&](const snitch::event::assertion_succeeded& e) {
                report_assertion(*this, r, e, true);
            },
            [&](const snitch::event::list_test_run_started&) {
                print(*this, r, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                open(*this, r, "MatchingTests");
            },
            [&](const snitch::event::list_test_run_ended&) { close(*this, r, "MatchingTests"); },
            [&](const snitch::event::test_case_listed& e) {
                open(*this, r, "TestCase");
                open_close(*this, r, "Name", make_full_name(e.id));
                open_close(*this, r, "ClassName", make_escaped(e.id.fixture));
                open_close(*this, r, "Tags", make_escaped(e.id.tags));
                open(*this, r, "SourceInfo");
                open_close(*this, r, "File", make_escaped(e.location.file));
                open_close(*this, r, "Line", make_string(e.location.line));
                close(*this, r, "SourceInfo");
                close(*this, r, "TestCase");
            }},
        event);
}
} // namespace snitch::reporter::catch2_xml

SNITCH_REGISTER_REPORTER("xml", snitch::reporter::catch2_xml::reporter);

#endif
