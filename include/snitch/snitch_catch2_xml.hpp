#ifndef SNITCH_XML_HPP
#define SNITCH_XML_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_registry.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"
#include "snitch/snitch_test_data.hpp"

#include <cstddef>
#include <initializer_list>
#include <string_view>

namespace snitch::catch2_xml {
struct key_value {
    std::string_view key;
    std::string_view value;
};

void escape(small_string_span string) noexcept {
    if (!replace_all(string, "&", "&amp;") || !replace_all(string, "\"", "&quot;") ||
        !replace_all(string, "'", "&apos;") || !replace_all(string, "<", "&lt;") ||
        !replace_all(string, ">", "&gt;")) {
        truncate_end(string);
    }
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

constexpr std::size_t max_number_length = 32;

template<typename T>
small_string<max_number_length> make_string(T number) noexcept {
    small_string<max_number_length> string;
    append_or_truncate(string, number);
    return string;
}

struct reporter {
    std::size_t indent_level = 0;

    std::string_view indent() noexcept {
        constexpr std::string_view spaces            = "                ";
        constexpr std::size_t      spaces_per_indent = 2;
        return spaces.substr(0, spaces_per_indent * indent_level);
    }

    void close(const registry& r, std::string_view node) noexcept {
        --indent_level;
        r.print(indent(), "</", node, ">\n");
    }

    void
    open(const registry& r, std::string_view node, std::initializer_list<key_value> args) noexcept {
        r.print(indent(), "<", node);
        for (const auto& arg : args) {
            r.print(" ", arg.key, "=\"", arg.value, "\"");
        }
        r.print(">\n");
        ++indent_level;
    }

    void
    node(const registry& r, std::string_view node, std::initializer_list<key_value> args) noexcept {
        r.print(indent(), "<", node);
        for (const auto& arg : args) {
            r.print(" ", arg.key, "=\"", arg.value, "\"");
        }
        r.print("/>\n");
    }

    explicit reporter(registry& r) noexcept {
        // The XML reporter needs test_case_started and test_case_ended events, which are only
        // printed on verbosity 'high', so ensure the requested verbosity is at least as much.
        r.verbose = r.verbose < registry::verbosity::high ? registry::verbosity::high : r.verbose;
    }

    bool configure(registry&, std::string_view, std::string_view) noexcept {
        // No configuration
        return false;
    }

    template<typename T>
    void report_assertion(const registry&, const T&) noexcept {
        // TODO
    }

    void report(const registry& r, const snitch::event::data& event) noexcept {
        std::visit(
            snitch::overload{
                [&](const snitch::event::test_run_started& e) {
                    r.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
                    open(
                        r, "Catch2TestRun",
                        {{"name", make_escaped(e.name)}, {"xml-format-version", "2"}});
                },
                [&](const snitch::event::test_run_ended& e) {
                    // TODO: missing failures and expectedFailures attribs
                    node(
                        r, "OverallResults",
                        {{"successes", make_string(e.assertion_count)},
                         {"failures", "0"},
                         {"expectedFailures", "0"}});

                    // TODO: missing expectedFailures attrib
                    node(
                        r, "OverallResultsCases",
                        {{"successes", make_string(e.run_count - e.fail_count)},
                         {"failures", make_string(e.fail_count)},
                         {"expectedFailures", "0"}});

                    close(r, "Catch2TestRun");
                },
                [&](const snitch::event::test_case_started& e) {
                    open(
                        r, "TestCase",
                        {{"name", make_full_name(e.id)},
                         {"tags", make_escaped(e.id.tags)},
                         {"filename", make_escaped(e.location.file)},
                         {"line", make_string(e.location.line)}});
                },
                [&](const snitch::event::test_case_ended& e) {
#if SNITCH_WITH_TIMINGS
                    node(
                        r, "OverallResult",
                        {{"success", e.state == test_case_state::failed ? "false" : "true"},
                         {"durationInSeconds", make_string(e.duration)}});
#else
                    node(
                        r, "OverallResult",
                        {{"success", e.state == test_case_state::failed ? "false" : "true"}});
#endif
                    close(r, "TestCase");
                },
                [&](const snitch::event::test_case_skipped&) {
                    // Nothing to do; this gets reported as "success".
                },
                [&](const snitch::event::assertion_failed& e) { report_assertion(r, e); },
                [&](const snitch::event::assertion_succeeded& e) { report_assertion(r, e); }},
            event);
    }
};
} // namespace snitch::catch2_xml

SNITCH_REGISTER_REPORTER("xml", snitch::catch2_xml::reporter);
#endif
