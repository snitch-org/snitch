#ifndef SNITCH_TEAMCITY_HPP
#define SNITCH_TEAMCITY_HPP

#include "snitch/snitch.hpp"

namespace snitch::teamcity {
struct key_value {
    std::string_view key;
    std::string_view value;
};

void escape(small_string_span string) noexcept {
    if (!replace_all(string, "|", "||") || !replace_all(string, "'", "|'") ||
        !replace_all(string, "\n", "|n") || !replace_all(string, "\r", "|r") ||
        !replace_all(string, "[", "|[") || !replace_all(string, "]", "|]")) {
        truncate_end(string);
    }
}

void send_message(
    const registry& r, std::string_view message, std::initializer_list<key_value> args) noexcept {
    constexpr std::string_view teamcity_header = "##teamCity[";
    constexpr std::string_view teamcity_footer = "]\n";

    r.print(teamcity_header, message);
    for (const auto& arg : args) {
        r.print(" ", arg.key, "='", arg.value, "'");
    }
    r.print(teamcity_footer);
}

small_string<max_test_name_length> make_full_name(const test_id& id) noexcept {
    small_string<max_test_name_length> name;
    if (id.type.length() != 0) {
        append_or_truncate(name, id.name, "(\"", id.type, "\")");
    } else {
        append_or_truncate(name, id.name);
    }

    escape(name);
    return name;
}

small_string<max_message_length> make_full_message(
    const snitch::assertion_location& location,
    const snitch::section_info&       sections,
    const snitch::capture_info&       captures,
    std::string_view                  message) noexcept {

    small_string<max_message_length> full_message;
    append_or_truncate(full_message, location.file, ":", location.line, "\n");
    for (const auto& s : sections) {
        append_or_truncate(full_message, s.name, "\n");
    }
    for (const auto& c : captures) {
        append_or_truncate(full_message, c, "\n");
    }

    append_or_truncate(full_message, "  ", message);
    escape(full_message);
    return full_message;
}

small_string<max_message_length> make_escaped(std::string_view string) noexcept {
    small_string<max_message_length> escaped_string;
    append_or_truncate(escaped_string, string);
    escape(escaped_string);
    return escaped_string;
}

constexpr std::size_t max_duration_length = 32;

small_string<max_duration_length> make_duration(float duration) noexcept {
    small_string<max_duration_length> string;
    append_or_truncate(string, static_cast<std::size_t>(duration * 1e6));
    return string;
}

void report(const registry& r, const snitch::event::data& event) noexcept {
    std::visit(
        snitch::overload{
            [&](const snitch::event::test_run_started& e) {
                send_message(r, "testSuiteStarted", {{"name", make_escaped(e.name)}});
            },
            [&](const snitch::event::test_run_ended& e) {
                send_message(r, "testSuiteFinished", {{"name", make_escaped(e.name)}});
            },
            [&](const snitch::event::test_case_started& e) {
                send_message(r, "testStarted", {{"name", make_full_name(e.id)}});
            },
            [&](const snitch::event::test_case_ended& e) {
#if SNITCH_WITH_TIMINGS
                send_message(
                    r, "testFinished",
                    {{"name", make_full_name(e.id)}, {"duration", make_duration(e.duration)}});
#else
                send_message(r, "testFinished", {{"name", make_full_name(e.id)}});
#endif
            },
            [&](const snitch::event::test_case_skipped& e) {
                send_message(
                    r, "testIgnored",
                    {{"name", make_full_name(e.id)},
                     {"message",
                      make_full_message(e.location, e.sections, e.captures, e.message)}});
            },
            [&](const snitch::event::assertion_failed& e) {
                send_message(
                    r, "testFailed",
                    {{"name", make_full_name(e.id)},
                     {"message",
                      make_full_message(e.location, e.sections, e.captures, e.message)}});
            }},
        event);
}
} // namespace snitch::teamcity

#endif
