#ifndef SNATCH_TEAMCITY_HPP
#define SNATCH_TEAMCITY_HPP

#include "snatch/snatch.hpp"

namespace snatch::teamcity {
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
        if (!append(name, id.name, "(\"", id.type, "\")")) {
            truncate_end(name);
        }
    } else {
        if (!append(name, id.name)) {
            truncate_end(name);
        }
    }

    escape(name);
    return name;
}

small_string<max_message_length>
make_full_message(const snatch::assertion_location& location, std::string_view message) noexcept {
    small_string<max_message_length> full_message;
    if (!append(full_message, location.file, ":", location.line, "\n", message)) {
        truncate_end(full_message);
    }

    escape(full_message);
    return full_message;
}

small_string<max_message_length> make_escaped(std::string_view string) noexcept {
    small_string<max_message_length> escaped_string;
    if (!append(escaped_string, string)) {
        truncate_end(escaped_string);
    }

    escape(escaped_string);
    return escaped_string;
}

void report(const registry& r, const snatch::event::data& event) noexcept {
    std::visit(
        snatch::overload{
            [&](const snatch::event::test_run_started& e) {
                send_message(r, "testSuiteStarted", {{"name", make_escaped(e.name)}});
            },
            [&](const snatch::event::test_run_ended& e) {
                send_message(r, "testSuiteFinished", {{"name", make_escaped(e.name)}});
            },
            [&](const snatch::event::test_case_started& e) {
                send_message(r, "testStarted", {{"name", make_full_name(e.id)}});
            },
            [&](const snatch::event::test_case_ended& e) {
                send_message(r, "testFinished", {{"name", make_full_name(e.id)}});
            },
            [&](const snatch::event::test_case_skipped& e) {
                send_message(
                    r, "testIgnored",
                    {{"name", make_full_name(e.id)},
                     {"message", make_full_message(e.location, e.message)}});
            },
            [&](const snatch::event::assertion_failed& e) {
                send_message(
                    r, "testFailed",
                    {{"name", make_full_name(e.id)},
                     {"message", make_full_message(e.location, e.message)}});
            }},
        event);
}
} // namespace snatch::teamcity

#endif
