#include "snitch/snitch.hpp"

#include <algorithm> // for std::sort
#include <cstdio> // for std::printf, std::snprintf
#include <cstring> // for std::memcpy
#include <optional> // for std::optional

#if SNITCH_WITH_TIMINGS
#    include <chrono> // for measuring test time
#endif

// Testing framework implementation utilities.
// -------------------------------------------

namespace {
using namespace std::literals;
using color_t = const char*;

namespace color {
constexpr color_t error [[maybe_unused]]      = "\x1b[1;31m";
constexpr color_t warning [[maybe_unused]]    = "\x1b[1;33m";
constexpr color_t status [[maybe_unused]]     = "\x1b[1;36m";
constexpr color_t fail [[maybe_unused]]       = "\x1b[1;31m";
constexpr color_t skipped [[maybe_unused]]    = "\x1b[1;33m";
constexpr color_t pass [[maybe_unused]]       = "\x1b[1;32m";
constexpr color_t highlight1 [[maybe_unused]] = "\x1b[1;35m";
constexpr color_t highlight2 [[maybe_unused]] = "\x1b[1;36m";
constexpr color_t reset [[maybe_unused]]      = "\x1b[0m";
} // namespace color

template<typename T>
struct colored {
    const T& value;
    color_t  color_start;
    color_t  color_end;
};

template<typename T>
colored<T> make_colored(const T& t, bool with_color, color_t start) {
    return {t, with_color ? start : "", with_color ? color::reset : ""};
}

thread_local snitch::impl::test_state* thread_current_test = nullptr;
} // namespace

namespace {
using snitch::small_string_span;

template<typename T>
constexpr const char* get_format_code() noexcept {
    if constexpr (std::is_same_v<T, const void*>) {
        return "%p";
    } else if constexpr (std::is_same_v<T, std::size_t>) {
        return "%zu";
    } else if constexpr (std::is_same_v<T, std::ptrdiff_t>) {
        return "%td";
    } else if constexpr (std::is_same_v<T, float>) {
        return "%f";
    } else if constexpr (std::is_same_v<T, double>) {
        return "%lf";
    } else {
        static_assert(!std::is_same_v<T, T>, "unsupported type");
    }
}

template<typename T>
bool append_fmt(small_string_span ss, T value) noexcept {
    if (ss.available() <= 1) {
        // snprintf will always print a null-terminating character,
        // so abort early if only space for one or zero character, as
        // this would clobber the original string.
        return false;
    }

    // Calculate required length.
    const int return_code = std::snprintf(nullptr, 0, get_format_code<T>(), value);
    if (return_code < 0) {
        return false;
    }

    // 'return_code' holds the number of characters that are required,
    // excluding the null-terminating character, which always gets appended,
    // so we need to +1.
    const std::size_t length    = static_cast<std::size_t>(return_code) + 1;
    const bool        could_fit = length <= ss.available();

    const std::size_t offset     = ss.size();
    const std::size_t prev_space = ss.available();
    ss.resize(std::min(ss.size() + length, ss.capacity()));
    std::snprintf(ss.begin() + offset, prev_space, get_format_code<T>(), value);

    // Pop the null-terminating character, always printed unfortunately.
    ss.pop_back();

    return could_fit;
}
} // namespace

namespace snitch {
bool append(small_string_span ss, std::string_view str) noexcept {
    if (str.empty()) {
        return true;
    }

    const bool        could_fit  = str.size() <= ss.available();
    const std::size_t copy_count = std::min(str.size(), ss.available());

    const std::size_t offset = ss.size();
    ss.grow(copy_count);
    std::memmove(ss.begin() + offset, str.data(), copy_count);

    return could_fit;
}

bool append(small_string_span ss, const void* ptr) noexcept {
    return append_fmt(ss, ptr);
}

bool append(small_string_span ss, std::nullptr_t) noexcept {
    return append(ss, "nullptr");
}

bool append(small_string_span ss, std::size_t i) noexcept {
    return append_fmt(ss, i);
}

bool append(small_string_span ss, std::ptrdiff_t i) noexcept {
    return append_fmt(ss, i);
}

bool append(small_string_span ss, float f) noexcept {
    return append_fmt(ss, f);
}

bool append(small_string_span ss, double d) noexcept {
    return append_fmt(ss, d);
}

bool append(small_string_span ss, bool value) noexcept {
    return append(ss, value ? "true" : "false");
}

void truncate_end(small_string_span ss) noexcept {
    std::size_t num_dots     = 3;
    std::size_t final_length = std::min(ss.capacity(), ss.size() + num_dots);
    std::size_t offset       = final_length >= num_dots ? final_length - num_dots : 0;
    num_dots                 = final_length - offset;

    ss.resize(final_length);
    std::memcpy(ss.begin() + offset, "...", num_dots);
}

bool replace_all(
    small_string_span string, std::string_view pattern, std::string_view replacement) noexcept {

    if (replacement.size() == pattern.size()) {
        std::string_view sv(string.begin(), string.size());
        auto             pos = sv.find(pattern);

        while (pos != sv.npos) {
            // Replace pattern by replacement
            std::memcpy(string.data() + pos, replacement.data(), replacement.size());
            pos += replacement.size();

            // Find next occurrence
            pos = sv.find(pattern, pos);
        }

        return true;
    } else if (replacement.size() < pattern.size()) {
        const std::size_t char_diff = pattern.size() - replacement.size();
        std::string_view  sv(string.begin(), string.size());
        auto              pos = sv.find(pattern);

        while (pos != sv.npos) {
            // Shift data after the replacement to the left to fill the gap
            std::rotate(string.begin() + pos, string.begin() + pos + char_diff, string.end());
            string.resize(string.size() - char_diff);

            // Replace pattern by replacement
            std::memcpy(string.data() + pos, replacement.data(), replacement.size());
            pos += replacement.size();

            // Find next occurrence
            sv  = {string.begin(), string.size()};
            pos = sv.find(pattern, pos);
        }

        return true;
    } else {
        const std::size_t char_diff = replacement.size() - pattern.size();
        std::string_view  sv(string.begin(), string.size());
        auto              pos      = sv.find(pattern);
        bool              overflow = false;

        while (pos != sv.npos) {
            // Shift data after the pattern to the right to make room for the replacement
            const std::size_t char_growth = std::min(char_diff, string.available());
            if (char_growth != char_diff) {
                overflow = true;
            }
            string.grow(char_growth);

            if (char_diff <= string.size() && string.size() - char_diff > pos) {
                std::rotate(string.begin() + pos, string.end() - char_diff, string.end());
            }

            // Replace pattern by replacement
            const std::size_t max_chars = std::min(replacement.size(), string.size() - pos);
            std::memcpy(string.data() + pos, replacement.data(), max_chars);
            pos += max_chars;

            // Find next occurrence
            sv  = {string.begin(), string.size()};
            pos = sv.find(pattern, pos);
        }

        return !overflow;
    }
}

bool is_match(std::string_view string, std::string_view regex) noexcept {
    // An empty regex matches any string; early exit.
    // An empty string matches an empty regex (exit here) or any regex containing
    // only wildcards (exit later).
    if (regex.empty()) {
        return true;
    }

    const std::size_t regex_size  = regex.size();
    const std::size_t string_size = string.size();

    // Iterate characters of the regex string and exit at first non-match.
    std::size_t js = 0;
    for (std::size_t jr = 0; jr < regex_size; ++jr, ++js) {
        bool escaped = false;
        if (regex[jr] == '\\') {
            // Escaped character, look ahead ignoring special characters.
            ++jr;
            if (jr >= regex_size) {
                // Nothing left to escape; the regex is ill-formed.
                return false;
            }

            escaped = true;
        }

        if (!escaped && regex[jr] == '*') {
            // Wildcard is found; if this is the last character of the regex
            // then any further content will be a match; early exit.
            if (jr == regex_size - 1) {
                return true;
            }

            // Discard what has already been matched.
            regex = regex.substr(jr + 1);

            // If there are no more characters in the string after discarding, then we only match if
            // the regex contains only wildcards from there on.
            const std::size_t remaining = string_size >= js ? string_size - js : 0u;
            if (remaining == 0u) {
                return regex.find_first_not_of('*') == regex.npos;
            }

            // Otherwise, we loop over all remaining characters of the string and look
            // for a match when starting from each of them.
            for (std::size_t o = 0; o < remaining; ++o) {
                if (is_match(string.substr(js + o), regex)) {
                    return true;
                }
            }

            return false;
        } else if (js >= string_size || regex[jr] != string[js]) {
            // Regular character is found; not a match if not an exact match in the string.
            return false;
        }
    }

    // We have finished reading the regex string and did not find either a definite non-match
    // or a definite match. This means we did not have any wildcard left, hence that we need
    // an exact match. Therefore, only match if the string size is the same as the regex.
    return js == string_size;
}
} // namespace snitch

namespace snitch::impl {
void stdout_print(std::string_view message) noexcept {
    // TODO: replace this with std::print?
    std::printf("%.*s", static_cast<int>(message.length()), message.data());
}

test_state& get_current_test() noexcept {
    test_state* current = thread_current_test;
    if (current == nullptr) {
        terminate_with("no test case is currently running on this thread");
    }

    return *current;
}

test_state* try_get_current_test() noexcept {
    return thread_current_test;
}

void set_current_test(test_state* current) noexcept {
    thread_current_test = current;
}

} // namespace snitch::impl

namespace snitch::cli {
small_function<void(std::string_view) noexcept> console_print = &snitch::impl::stdout_print;
} // namespace snitch::cli

namespace {
using snitch::max_message_length;
using snitch::small_string;

template<typename T>
bool append(small_string_span ss, const colored<T>& colored_value) noexcept {
    return append(ss, colored_value.color_start, colored_value.value, colored_value.color_end);
}

template<typename... Args>
void console_print(Args&&... args) noexcept {
    small_string<max_message_length> message;
    append_or_truncate(message, std::forward<Args>(args)...);
    snitch::cli::console_print(message);
}

bool is_at_least(snitch::registry::verbosity verbose, snitch::registry::verbosity required) {
    using underlying_type = std::underlying_type_t<snitch::registry::verbosity>;
    return static_cast<underlying_type>(verbose) >= static_cast<underlying_type>(required);
}

void trim(std::string_view& str, std::string_view patterns) noexcept {
    std::size_t start = str.find_first_not_of(patterns);
    if (start == str.npos)
        return;

    str.remove_prefix(start);

    std::size_t end = str.find_last_not_of(patterns);
    if (end != str.npos)
        str.remove_suffix(str.size() - end - 1);
}
} // namespace

namespace snitch {
[[noreturn]] void terminate_with(std::string_view msg) noexcept {
    impl::stdout_print("terminate called with message: ");
    impl::stdout_print(msg);
    impl::stdout_print("\n");

    std::terminate();
}
} // namespace snitch

// Sections implementation.
// ------------------------

namespace snitch::impl {
section_entry_checker::~section_entry_checker() noexcept {
    if (entered) {
        if (state.sections.levels.size() == state.sections.depth) {
            state.sections.leaf_executed = true;
        } else {
            auto& child = state.sections.levels[state.sections.depth];
            if (child.previous_section_id == child.max_section_id) {
                state.sections.levels.pop_back();
            }
        }

        state.sections.current_section.pop_back();
    }

    --state.sections.depth;
}

section_entry_checker::operator bool() noexcept {
    ++state.sections.depth;

    if (state.sections.depth > state.sections.levels.size()) {
        if (state.sections.depth > max_nested_sections) {
            state.reg.print(
                make_colored("error:", state.reg.with_color, color::fail),
                " max number of nested sections reached; "
                "please increase 'SNITCH_MAX_NESTED_SECTIONS' (currently ",
                max_nested_sections, ")\n.");
            std::terminate();
        }

        state.sections.levels.push_back({});
    }

    auto& level = state.sections.levels[state.sections.depth - 1];

    ++level.current_section_id;
    if (level.max_section_id < level.current_section_id) {
        level.max_section_id = level.current_section_id;
    }

    if (!state.sections.leaf_executed &&
        (level.previous_section_id + 1 == level.current_section_id ||
         (level.previous_section_id == level.current_section_id &&
          state.sections.levels.size() > state.sections.depth))) {

        level.previous_section_id = level.current_section_id;
        state.sections.current_section.push_back(section);
        entered = true;
        return true;
    }

    return false;
}
} // namespace snitch::impl

// Captures implementation.
// ------------------------

namespace snitch::impl {
std::string_view extract_next_name(std::string_view& names) noexcept {
    std::string_view result;

    auto pos = names.find_first_of(",()\"\"''");

    bool in_string = false;
    bool in_char   = false;
    int  parens    = 0;
    while (pos != names.npos) {
        switch (names[pos]) {
        case '"':
            if (!in_char) {
                in_string = !in_string;
            }
            break;
        case '\'':
            if (!in_string) {
                in_char = !in_char;
            }
            break;
        case '(':
            if (!in_string && !in_char) {
                ++parens;
            }
            break;
        case ')':
            if (!in_string && !in_char) {
                --parens;
            }
            break;
        case ',':
            if (!in_string && !in_char && parens == 0) {
                result = names.substr(0, pos);
                trim(result, " \t\n\r");
                names.remove_prefix(pos + 1);
                return result;
            }
            break;
        }

        pos = names.find_first_of(",()\"\"''", pos + 1);
    }

    std::swap(result, names);
    trim(result, " \t\n\r");
    return result;
}

small_string<max_capture_length>& add_capture(test_state& state) noexcept {
    if (state.captures.available() == 0) {
        state.reg.print(
            make_colored("error:", state.reg.with_color, color::fail),
            " max number of captures reached; "
            "please increase 'SNITCH_MAX_CAPTURES' (currently ",
            max_captures, ")\n.");
        std::terminate();
    }

    state.captures.grow(1);
    state.captures.back().clear();
    return state.captures.back();
}
} // namespace snitch::impl

// Matcher implementation.
// -----------------------

namespace snitch::matchers {
contains_substring::contains_substring(std::string_view pattern) noexcept :
    substring_pattern(pattern) {}

bool contains_substring::match(std::string_view message) const noexcept {
    return message.find(substring_pattern) != message.npos;
}

small_string<max_message_length>
contains_substring::describe_match(std::string_view message, match_status status) const noexcept {
    small_string<max_message_length> description_buffer;
    append_or_truncate(
        description_buffer, (status == match_status::matched ? "found" : "could not find"), " '",
        substring_pattern, "' in '", message, "'");
    return description_buffer;
}

with_what_contains::with_what_contains(std::string_view pattern) noexcept :
    contains_substring(pattern) {}
} // namespace snitch::matchers

// Testing framework implementation.
// ---------------------------------

namespace {
using namespace snitch;
using namespace snitch::impl;

template<typename F>
void for_each_raw_tag(std::string_view s, F&& callback) noexcept {
    if (s.empty()) {
        return;
    }

    if (s.find_first_of("[") == std::string_view::npos ||
        s.find_first_of("]") == std::string_view::npos) {
        terminate_with("incorrectly formatted tag; please use \"[tag1][tag2][...]\"");
    }

    std::string_view delim    = "][";
    std::size_t      pos      = s.find(delim);
    std::size_t      last_pos = 0u;

    while (pos != std::string_view::npos) {
        std::size_t cur_size = pos - last_pos;
        if (cur_size != 0) {
            callback(s.substr(last_pos, cur_size + 1));
        }
        last_pos = pos + 1;
        pos      = s.find(delim, last_pos);
    }

    callback(s.substr(last_pos));
}

namespace tags {
struct ignored {};
struct may_fail {};
struct should_fail {};

using parsed_tag = std::variant<std::string_view, ignored, may_fail, should_fail>;
} // namespace tags

template<typename F>
void for_each_tag(std::string_view s, F&& callback) noexcept {
    small_string<max_tag_length> buffer;

    for_each_raw_tag(s, [&](std::string_view t) {
        // Look for "ignore" tags, which is either "[.]"
        // or a tag starting with ".", like "[.integration]".
        if (t == "[.]"sv) {
            // This is a pure "ignore" tag, add this to the list of special tags.
            callback(tags::parsed_tag{tags::ignored{}});
        } else if (t.starts_with("[."sv)) {
            // This is a combined "ignore" + normal tag, add the "ignore" to the list of special
            // tags, and continue with the normal tag.
            callback(tags::parsed_tag{tags::ignored{}});
            callback(tags::parsed_tag{std::string_view("[.]")});

            buffer.clear();
            if (!append(buffer, "[", t.substr(2u))) {
                terminate_with("tag is too long");
            }

            t = buffer;
        }

        if (t == "[!mayfail]") {
            callback(tags::parsed_tag{tags::may_fail{}});
        }

        if (t == "[!shouldfail]") {
            callback(tags::parsed_tag{tags::should_fail{}});
        }

        callback(tags::parsed_tag(t));
    });
}

std::string_view
make_full_name(small_string<max_test_name_length>& buffer, const test_id& id) noexcept {
    buffer.clear();
    if (id.type.length() != 0) {
        if (!append(buffer, id.name, " <", id.type, ">")) {
            return {};
        }
    } else {
        if (!append(buffer, id.name)) {
            return {};
        }
    }

    return buffer.str();
}

template<typename F>
void list_tests(const registry& r, F&& predicate) noexcept {
    small_string<max_test_name_length> buffer;
    for (const test_case& t : r) {
        if (!predicate(t)) {
            continue;
        }

        r.print(make_full_name(buffer, t.id), "\n");
    }
}

void set_state(test_case& t, impl::test_case_state s) noexcept {
    if (static_cast<std::underlying_type_t<impl::test_case_state>>(t.state) <
        static_cast<std::underlying_type_t<impl::test_case_state>>(s)) {
        t.state = s;
    }
}

snitch::test_case_state convert_to_public_state(impl::test_case_state s) noexcept {
    switch (s) {
    case impl::test_case_state::success: return snitch::test_case_state::success;
    case impl::test_case_state::failed: return snitch::test_case_state::failed;
    case impl::test_case_state::skipped: return snitch::test_case_state::skipped;
    default: terminate_with("test case state cannot be exposed to the public");
    }
}

small_vector<std::string_view, max_captures> make_capture_buffer(const capture_state& captures) {
    small_vector<std::string_view, max_captures> captures_buffer;
    for (const auto& c : captures) {
        captures_buffer.push_back(c);
    }

    return captures_buffer;
}
} // namespace

namespace snitch {
filter_result is_filter_match_name(std::string_view name, std::string_view filter) noexcept {
    filter_result match_action    = filter_result::included;
    filter_result no_match_action = filter_result::not_included;
    if (filter.starts_with('~')) {
        filter          = filter.substr(1);
        match_action    = filter_result::excluded;
        no_match_action = filter_result::not_excluded;
    }

    return is_match(name, filter) ? match_action : no_match_action;
}

filter_result is_filter_match_tags(std::string_view tags, std::string_view filter) noexcept {
    filter_result match_action    = filter_result::included;
    filter_result no_match_action = filter_result::not_included;
    if (filter.starts_with('~')) {
        filter          = filter.substr(1);
        match_action    = filter_result::excluded;
        no_match_action = filter_result::not_excluded;
    }

    bool match = false;
    for_each_tag(tags, [&](const tags::parsed_tag& v) {
        if (auto* vs = std::get_if<std::string_view>(&v); vs != nullptr && is_match(*vs, filter)) {
            match = true;
        }
    });

    return match ? match_action : no_match_action;
}

filter_result is_filter_match_id(const test_id& id, std::string_view filter) noexcept {
    if (filter.starts_with('[') || filter.starts_with("~[")) {
        return is_filter_match_tags(id.tags, filter);
    } else {
        return is_filter_match_name(id.name, filter);
    }
}

const char* registry::add(const test_id& id, test_ptr func) noexcept {
    if (test_list.size() == test_list.capacity()) {
        print(
            make_colored("error:", with_color, color::fail),
            " max number of test cases reached; "
            "please increase 'SNITCH_MAX_TEST_CASES' (currently ",
            max_test_cases, ")\n.");
        std::terminate();
    }

    test_list.push_back(test_case{id, func});

    small_string<max_test_name_length> buffer;
    if (make_full_name(buffer, test_list.back().id).empty()) {
        print(
            make_colored("error:", with_color, color::fail),
            " max length of test name reached; "
            "please increase 'SNITCH_MAX_TEST_NAME_LENGTH' (currently ",
            max_test_name_length, ")\n.");
        std::terminate();
    }

    return id.name.data();
}

void registry::print_location(
    const impl::test_case&     current_case,
    const impl::section_state& sections,
    const impl::capture_state& captures,
    const assertion_location&  location) const noexcept {

    print(
        "running test case \"", make_colored(current_case.id.name, with_color, color::highlight1),
        "\"\n");

    for (auto& section : sections.current_section) {
        print(
            "          in section \"", make_colored(section.name, with_color, color::highlight1),
            "\"\n");
    }

    print("          at ", location.file, ":", location.line, "\n");

    if (!current_case.id.type.empty()) {
        print(
            "          for type ",
            make_colored(current_case.id.type, with_color, color::highlight1), "\n");
    }

    for (auto& capture : captures) {
        print("          with ", make_colored(capture, with_color, color::highlight1), "\n");
    }
}

void registry::print_failure() const noexcept {
    print(make_colored("failed: ", with_color, color::fail));
}

void registry::print_expected_failure() const noexcept {
    print(make_colored("expected failure: ", with_color, color::pass));
}

void registry::print_skip() const noexcept {
    print(make_colored("skipped: ", with_color, color::skipped));
}

void registry::print_details(std::string_view message) const noexcept {
    print("          ", make_colored(message, with_color, color::highlight2), "\n");
}

void registry::print_details_expr(const expression& exp) const noexcept {
    print("          ", make_colored(exp.expected, with_color, color::highlight2));

    if (!exp.actual.empty()) {
        print(", got ", make_colored(exp.actual, with_color, color::highlight2));
    }

    print("\n");
}

void registry::report_failure(
    impl::test_state&         state,
    const assertion_location& location,
    std::string_view          message) const noexcept {

    if (!state.may_fail) {
        set_state(state.test, impl::test_case_state::failed);
    }

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message, state.should_fail, state.may_fail});
    } else {
        if (state.should_fail) {
            print_expected_failure();
        } else {
            print_failure();
        }
        print_location(state.test, state.sections, state.captures, location);
        print_details(message);
    }
}

void registry::report_failure(
    impl::test_state&         state,
    const assertion_location& location,
    std::string_view          message1,
    std::string_view          message2) const noexcept {

    if (!state.may_fail) {
        set_state(state.test, impl::test_case_state::failed);
    }

    small_string<max_message_length> message;
    append_or_truncate(message, message1, message2);

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message, state.should_fail, state.may_fail});
    } else {
        if (state.should_fail) {
            print_expected_failure();
        } else {
            print_failure();
        }
        print_location(state.test, state.sections, state.captures, location);
        print_details(message);
    }
}

void registry::report_failure(
    impl::test_state&         state,
    const assertion_location& location,
    const impl::expression&   exp) const noexcept {

    if (!state.may_fail) {
        set_state(state.test, impl::test_case_state::failed);
    }

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        if (!exp.actual.empty()) {
            small_string<max_message_length> message;
            append_or_truncate(message, exp.expected, ", got ", exp.actual);
            report_callback(
                *this, event::assertion_failed{
                           state.test.id, state.sections.current_section, captures_buffer.span(),
                           location, message, state.should_fail, state.may_fail});
        } else {
            report_callback(
                *this, event::assertion_failed{
                           state.test.id, state.sections.current_section, captures_buffer.span(),
                           location, exp.expected, state.should_fail, state.may_fail});
        }
    } else {
        if (state.should_fail) {
            print_expected_failure();
        } else {
            print_failure();
        }
        print_location(state.test, state.sections, state.captures, location);
        print_details_expr(exp);
    }
}

void registry::report_skipped(
    impl::test_state&         state,
    const assertion_location& location,
    std::string_view          message) const noexcept {

    set_state(state.test, impl::test_case_state::skipped);

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        report_callback(
            *this, event::test_case_skipped{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message});
    } else {
        print_skip();
        print_location(state.test, state.sections, state.captures, location);
        print_details(message);
    }
}

test_state registry::run(test_case& test) noexcept {
    small_string<max_test_name_length> full_name;

    if (!report_callback.empty()) {
        report_callback(*this, event::test_case_started{test.id});
    } else if (is_at_least(verbose, verbosity::high)) {
        make_full_name(full_name, test.id);
        print(
            make_colored("starting:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), "\n");
    }

    test.state = impl::test_case_state::success;

    bool may_fail    = false;
    bool should_fail = false;
    for_each_tag(test.id.tags, [&](const tags::parsed_tag& v) {
        if (std::holds_alternative<tags::may_fail>(v)) {
            may_fail = true;
        } else if (std::holds_alternative<tags::should_fail>(v)) {
            should_fail = true;
        }
    });

    test_state state{.reg = *this, .test = test, .may_fail = may_fail, .should_fail = should_fail};

    // Store previously running test, to restore it later.
    // This should always be a null pointer, except when testing snitch itself.
    test_state* previous_run = thread_current_test;
    thread_current_test      = &state;

#if SNITCH_WITH_TIMINGS
    using clock     = std::chrono::steady_clock;
    auto time_start = clock::now();
#endif

    do {
        for (std::size_t i = 0; i < state.sections.levels.size(); ++i) {
            state.sections.levels[i].current_section_id = 0;
        }

        state.sections.leaf_executed = false;

#if SNITCH_WITH_EXCEPTIONS
        try {
            test.func();
        } catch (const impl::abort_exception&) {
            // Test aborted, assume its state was already set accordingly.
        } catch (const std::exception& e) {
            report_failure(
                state, {__FILE__, __LINE__}, "unhandled std::exception caught; message:", e.what());
        } catch (...) {
            report_failure(state, {__FILE__, __LINE__}, "unhandled unknown exception caught");
        }
#else
        test.func();
#endif

        if (state.sections.levels.size() == 1) {
            auto& child = state.sections.levels[0];
            if (child.previous_section_id == child.max_section_id) {
                state.sections.levels.clear();
                state.sections.current_section.clear();
            }
        }
    } while (!state.sections.levels.empty());

    if (state.should_fail) {
        if (state.test.state == impl::test_case_state::success) {
            state.should_fail = false;
            report_failure(state, {__FILE__, __LINE__}, "expected test to fail, but it passed");
            state.should_fail = true;
        } else if (state.test.state == impl::test_case_state::failed) {
            state.test.state = impl::test_case_state::success;
        }
    }

#if SNITCH_WITH_TIMINGS
    auto time_end  = clock::now();
    state.duration = std::chrono::duration<float>(time_end - time_start).count();
#endif

    if (!report_callback.empty()) {
#if SNITCH_WITH_TIMINGS
        report_callback(
            *this, event::test_case_ended{
                       .id              = test.id,
                       .state           = convert_to_public_state(state.test.state),
                       .assertion_count = state.asserts,
                       .duration        = state.duration});
#else
        report_callback(
            *this, event::test_case_ended{
                       .id              = test.id,
                       .state           = convert_to_public_state(state.test.state),
                       .assertion_count = state.asserts});
#endif
    } else if (is_at_least(verbose, verbosity::high)) {
#if SNITCH_WITH_TIMINGS
        print(
            make_colored("finished:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), " (", state.duration, "s)\n");
#else
        print(
            make_colored("finished:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), "\n");
#endif
    }

    thread_current_test = previous_run;

    return state;
}

bool registry::run_selected_tests(
    std::string_view                                     run_name,
    const small_function<bool(const test_id&) noexcept>& predicate) noexcept {

    if (!report_callback.empty()) {
        report_callback(*this, event::test_run_started{run_name});
    } else if (is_at_least(verbose, registry::verbosity::normal)) {
        print(
            make_colored("starting tests with ", with_color, color::highlight2),
            make_colored("snitch v" SNITCH_FULL_VERSION "\n", with_color, color::highlight1));
        print("==========================================\n");
    }

    bool        success         = true;
    std::size_t run_count       = 0;
    std::size_t fail_count      = 0;
    std::size_t skip_count      = 0;
    std::size_t assertion_count = 0;

#if SNITCH_WITH_TIMINGS
    using clock     = std::chrono::steady_clock;
    auto time_start = clock::now();
#endif

    for (test_case& t : *this) {
        if (!predicate(t.id)) {
            continue;
        }

        auto state = run(t);

        ++run_count;
        assertion_count += state.asserts;

        switch (t.state) {
        case impl::test_case_state::success: {
            // Nothing to do
            break;
        }
        case impl::test_case_state::failed: {
            ++fail_count;
            success = false;
            break;
        }
        case impl::test_case_state::skipped: {
            ++skip_count;
            break;
        }
        case impl::test_case_state::not_run: {
            // Unreachable
            break;
        }
        }
    }

#if SNITCH_WITH_TIMINGS
    auto  time_end = clock::now();
    float duration = std::chrono::duration<float>(time_end - time_start).count();
#endif

    if (!report_callback.empty()) {
#if SNITCH_WITH_TIMINGS
        report_callback(
            *this, event::test_run_ended{
                       .name            = run_name,
                       .success         = success,
                       .run_count       = run_count,
                       .fail_count      = fail_count,
                       .skip_count      = skip_count,
                       .assertion_count = assertion_count,
                       .duration        = duration});
#else
        report_callback(
            *this, event::test_run_ended{
                       .name            = run_name,
                       .success         = success,
                       .run_count       = run_count,
                       .fail_count      = fail_count,
                       .skip_count      = skip_count,
                       .assertion_count = assertion_count});
#endif
    } else if (is_at_least(verbose, registry::verbosity::normal)) {
        print("==========================================\n");

        if (success) {
            print(
                make_colored("success:", with_color, color::pass), " all tests passed (", run_count,
                " test cases, ", assertion_count, " assertions");
        } else {
            print(
                make_colored("error:", with_color, color::fail), " some tests failed (", fail_count,
                " out of ", run_count, " test cases, ", assertion_count, " assertions");
        }

        if (skip_count > 0) {
            print(", ", skip_count, " test cases skipped");
        }

#if SNITCH_WITH_TIMINGS
        print(", ", duration, " seconds");
#endif

        print(")\n");
    }

    return success;
}

bool registry::run_tests(std::string_view run_name) noexcept {
    const auto filter = [](const test_id& id) {
        bool selected = true;
        for_each_tag(id.tags, [&](const tags::parsed_tag& s) {
            if (std::holds_alternative<tags::ignored>(s)) {
                selected = false;
            }
        });

        return selected;
    };

    return run_selected_tests(run_name, filter);
}

void registry::list_all_tags() const noexcept {
    small_vector<std::string_view, max_unique_tags> tags;
    for (const auto& t : test_list) {
        for_each_tag(t.id.tags, [&](const tags::parsed_tag& v) {
            if (auto* vs = std::get_if<std::string_view>(&v); vs != nullptr) {
                if (std::find(tags.begin(), tags.end(), *vs) == tags.end()) {
                    if (tags.size() == tags.capacity()) {
                        print(
                            make_colored("error:", with_color, color::fail),
                            " max number of tags reached; "
                            "please increase 'SNITCH_MAX_UNIQUE_TAGS' (currently ",
                            max_unique_tags, ")\n.");
                        std::terminate();
                    }

                    tags.push_back(*vs);
                }
            }
        });
    }

    std::sort(tags.begin(), tags.end());

    for (const auto& t : tags) {
        print("[", t, "]\n");
    }
}

void registry::list_all_tests() const noexcept {
    list_tests(*this, [](const test_case&) { return true; });
}

void registry::list_tests_with_tag(std::string_view tag) const noexcept {
    list_tests(*this, [&](const test_case& t) {
        const auto result = is_filter_match_tags(t.id.tags, tag);
        return result == filter_result::included || result == filter_result::not_excluded;
    });
}

test_case* registry::begin() noexcept {
    return test_list.begin();
}

test_case* registry::end() noexcept {
    return test_list.end();
}

const test_case* registry::begin() const noexcept {
    return test_list.begin();
}

const test_case* registry::end() const noexcept {
    return test_list.end();
}

constinit registry tests = []() {
    registry r;
    r.with_color = SNITCH_DEFAULT_WITH_COLOR == 1;
    return r;
}();
} // namespace snitch

// Main entry point utilities.
// ---------------------------

namespace {
using namespace std::literals;

constexpr std::size_t max_arg_names = 2;

namespace argument_type {
enum type { optional = 0b00, mandatory = 0b01, repeatable = 0b10 };
}

struct expected_argument {
    small_vector<std::string_view, max_arg_names> names;
    std::optional<std::string_view>               value_name;
    std::string_view                              description;
    argument_type::type                           type = argument_type::optional;
};

using expected_arguments = small_vector<expected_argument, max_command_line_args>;

struct parser_settings {
    bool with_color = true;
};

std::string_view extract_executable(std::string_view path) {
    if (auto folder_end = path.find_last_of("\\/"); folder_end != path.npos) {
        path.remove_prefix(folder_end + 1);
    }
    if (auto extension_start = path.find_last_of('.'); extension_start != path.npos) {
        path.remove_suffix(path.size() - extension_start);
    }

    return path;
}

bool is_option(const expected_argument& e) {
    return !e.names.empty();
}

bool is_option(const cli::argument& a) {
    return !a.name.empty();
}

bool has_value(const expected_argument& e) {
    return e.value_name.has_value();
}

bool is_mandatory(const expected_argument& e) {
    return (e.type & argument_type::mandatory) != 0;
}

bool is_repeatable(const expected_argument& e) {
    return (e.type & argument_type::repeatable) != 0;
}

std::optional<cli::input> parse_arguments(
    int                       argc,
    const char* const         argv[],
    const expected_arguments& expected,
    const parser_settings&    settings = parser_settings{}) noexcept {

    std::optional<cli::input> ret(std::in_place);
    ret->executable = extract_executable(argv[0]);

    auto& args = ret->arguments;
    bool  bad  = false;

    // Check validity of inputs
    small_vector<bool, max_command_line_args> expected_found;
    for (const auto& e : expected) {
        expected_found.push_back(false);

        if (is_option(e)) {
            if (e.names.size() == 1) {
                if (!e.names[0].starts_with('-')) {
                    terminate_with("option name must start with '-' or '--'");
                }
            } else {
                if (!(e.names[0].starts_with('-') && e.names[1].starts_with("--"))) {
                    terminate_with("option names must be given with '-' first and '--' second");
                }
            }
        } else {
            if (!has_value(e)) {
                terminate_with("positional argument must have a value name");
            }
        }
    }

    // Parse
    for (int argi = 1; argi < argc; ++argi) {
        std::string_view arg(argv[argi]);

        if (arg.starts_with('-')) {
            // Options start with dashes.
            bool found = false;

            for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
                const auto& e = expected[arg_index];

                if (!is_option(e)) {
                    continue;
                }

                if (std::find(e.names.cbegin(), e.names.cend(), arg) == e.names.cend()) {
                    continue;
                }

                found = true;

                if (expected_found[arg_index] && !is_repeatable(e)) {
                    console_print(
                        make_colored("error:", settings.with_color, color::error),
                        " duplicate command line argument '", arg, "'\n");
                    bad = true;
                    break;
                }

                expected_found[arg_index] = true;

                if (has_value(e)) {
                    if (argi + 1 == argc) {
                        console_print(
                            make_colored("error:", settings.with_color, color::error),
                            " missing value '<", *e.value_name, ">' for command line argument '",
                            arg, "'\n");
                        bad = true;
                        break;
                    }

                    argi += 1;
                    args.push_back(cli::argument{
                        e.names.back(), e.value_name, {std::string_view(argv[argi])}});
                } else {
                    args.push_back(cli::argument{e.names.back()});
                }

                break;
            }

            if (!found) {
                console_print(
                    make_colored("warning:", settings.with_color, color::warning),
                    " unknown command line argument '", arg, "'\n");
            }
        } else {
            // If no dash, this is a positional argument.
            bool found = false;

            for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
                const auto& e = expected[arg_index];

                if (is_option(e)) {
                    continue;
                }

                if (expected_found[arg_index] && !is_repeatable(e)) {
                    continue;
                }

                found = true;

                args.push_back(cli::argument{""sv, e.value_name, {arg}});
                expected_found[arg_index] = true;
                break;
            }

            if (!found) {
                console_print(
                    make_colored("error:", settings.with_color, color::error),
                    " too many positional arguments\n");
                bad = true;
            }
        }
    }

    for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
        const auto& e = expected[arg_index];
        if (!expected_found[arg_index] && is_mandatory(e)) {
            if (!is_option(e)) {
                console_print(
                    make_colored("error:", settings.with_color, color::error),
                    " missing positional argument '<", *e.value_name, ">'\n");
            } else {
                console_print(
                    make_colored("error:", settings.with_color, color::error), " missing option '<",
                    e.names.back(), ">'\n");
            }
            bad = true;
        }
    }

    if (bad) {
        ret.reset();
    }

    return ret;
}

struct print_help_settings {
    bool with_color = true;
};

void print_help(
    std::string_view           program_name,
    std::string_view           program_description,
    const expected_arguments&  expected,
    const print_help_settings& settings = print_help_settings{}) {

    // Print program description
    console_print(make_colored(program_description, settings.with_color, color::highlight2), "\n");

    // Print command line usage example
    console_print(make_colored("Usage:", settings.with_color, color::pass), "\n");
    console_print("  ", program_name);
    if (std::any_of(expected.cbegin(), expected.cend(), [](auto& e) { return is_option(e); })) {
        console_print(" [options...]");
    }

    for (const auto& e : expected) {
        if (!is_option(e)) {
            if (!is_mandatory(e) && !is_repeatable(e)) {
                console_print(" [<", *e.value_name, ">]");
            } else if (is_mandatory(e) && !is_repeatable(e)) {
                console_print(" <", *e.value_name, ">");
            } else if (!is_mandatory(e) && is_repeatable(e)) {
                console_print(" [<", *e.value_name, ">...]");
            } else if (is_mandatory(e) && is_repeatable(e)) {
                console_print(" <", *e.value_name, ">...");
            } else {
                terminate_with("unhandled argument type");
            }
        }
    }

    console_print("\n\n");

    // List arguments
    small_string<max_message_length> heading;
    for (const auto& e : expected) {
        heading.clear();

        bool success = true;
        if (is_option(e)) {
            if (e.names[0].starts_with("--")) {
                success = success && append(heading, "    ");
            }

            success = success && append(heading, e.names[0]);

            if (e.names.size() == 2) {
                success = success && append(heading, ", ", e.names[1]);
            }

            if (has_value(e)) {
                success = success && append(heading, " <", *e.value_name, ">");
            }
        } else {
            success = success && append(heading, "<", *e.value_name, ">");
        }

        if (!success) {
            terminate_with("argument name is too long");
        }

        console_print(
            "  ", make_colored(heading, settings.with_color, color::highlight1), " ", e.description,
            "\n");
    }
}

// clang-format off
constexpr expected_arguments expected_args = {
    {{"-l", "--list-tests"},    {},                    "List tests by name"},
    {{"--list-tags"},           {},                    "List tags by name"},
    {{"--list-tests-with-tag"}, {"[tag]"},             "List tests by name with a given tag"},
    {{"-v", "--verbosity"},     {"quiet|normal|high"}, "Define how much gets sent to the standard output"},
    {{"--color"},               {"always|never"},      "Enable/disable color in output"},
    {{"-h", "--help"},          {},                    "Print help"},
    {{},                        {"test regex"},        "A regex to select which test cases to run", argument_type::repeatable}};
// clang-format on

constexpr bool with_color_default = SNITCH_DEFAULT_WITH_COLOR == 1;

constexpr const char* program_description = "Test runner (snitch v" SNITCH_FULL_VERSION ")";
} // namespace

namespace snitch::cli {
std::optional<cli::input> parse_arguments(int argc, const char* const argv[]) noexcept {
    std::optional<cli::input> ret_args =
        parse_arguments(argc, argv, expected_args, {.with_color = with_color_default});

    if (!ret_args) {
        console_print("\n");
        print_help(argv[0], program_description, expected_args, {.with_color = with_color_default});
    }

    return ret_args;
}

std::optional<cli::argument> get_option(const cli::input& args, std::string_view name) noexcept {
    std::optional<cli::argument> ret;

    auto iter = std::find_if(args.arguments.cbegin(), args.arguments.cend(), [&](const auto& arg) {
        return arg.name == name;
    });

    if (iter != args.arguments.cend()) {
        ret = *iter;
    }

    return ret;
}

std::optional<cli::argument>
get_positional_argument(const cli::input& args, std::string_view name) noexcept {
    std::optional<cli::argument> ret;

    auto iter = std::find_if(args.arguments.cbegin(), args.arguments.cend(), [&](const auto& arg) {
        return !is_option(arg) && arg.value_name == name;
    });

    if (iter != args.arguments.cend()) {
        ret = *iter;
    }

    return ret;
}

void for_each_positional_argument(
    const cli::input&                                      args,
    std::string_view                                       name,
    const small_function<void(std::string_view) noexcept>& callback) noexcept {

    auto iter = args.arguments.cbegin();
    while (iter != args.arguments.cend()) {
        iter = std::find_if(iter, args.arguments.cend(), [&](const auto& arg) {
            return !is_option(arg) && arg.value_name == name;
        });

        if (iter != args.arguments.cend()) {
            callback(*iter->value);
            ++iter;
        }
    }
}
} // namespace snitch::cli

namespace snitch {
void registry::configure(const cli::input& args) noexcept {
    if (auto opt = get_option(args, "--color")) {
        if (*opt->value == "always") {
            with_color = true;
        } else if (*opt->value == "never") {
            with_color = false;
        } else {
            print(
                make_colored("warning:", with_color, color::warning),
                "unknown color directive; please use one of always|never\n");
        }
    }

    if (auto opt = get_option(args, "--verbosity")) {
        if (*opt->value == "quiet") {
            verbose = snitch::registry::verbosity::quiet;
        } else if (*opt->value == "normal") {
            verbose = snitch::registry::verbosity::normal;
        } else if (*opt->value == "high") {
            verbose = snitch::registry::verbosity::high;
        } else {
            print(
                make_colored("warning:", with_color, color::warning),
                "unknown verbosity level; please use one of quiet|normal|high\n");
        }
    }
}

bool registry::run_tests(const cli::input& args) noexcept {
    if (get_option(args, "--help")) {
        console_print("\n");
        print_help(
            args.executable, program_description, expected_args,
            {.with_color = with_color_default});
        return true;
    }

    if (get_option(args, "--list-tests")) {
        list_all_tests();
        return true;
    }

    if (auto opt = get_option(args, "--list-tests-with-tag")) {
        list_tests_with_tag(*opt->value);
        return true;
    }

    if (get_option(args, "--list-tags")) {
        list_all_tags();
        return true;
    }

    if (get_positional_argument(args, "test regex").has_value()) {
        const auto filter = [&](const test_id& id) noexcept {
            std::optional<bool> selected;

            const auto callback = [&](std::string_view filter) noexcept {
                switch (is_filter_match_id(id, filter)) {
                case filter_result::included: selected = true; break;
                case filter_result::excluded: selected = false; break;
                case filter_result::not_included:
                    if (!selected.has_value()) {
                        selected = false;
                    }
                    break;
                case filter_result::not_excluded:
                    if (!selected.has_value()) {
                        selected = true;
                    }
                    break;
                }
            };

            for_each_positional_argument(args, "test regex", callback);

            return selected.value();
        };

        return run_selected_tests(args.executable, filter);
    } else {
        return run_tests(args.executable);
    }
}
} // namespace snitch

#if SNITCH_DEFINE_MAIN

// Main entry point.
// -----------------

int main(int argc, char* argv[]) {
    std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
    if (!args) {
        return 1;
    }

    snitch::tests.configure(*args);

    return snitch::tests.run_tests(*args) ? 0 : 1;
}

#endif
