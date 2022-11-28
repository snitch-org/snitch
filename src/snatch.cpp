#include "snatch/snatch.hpp"

#include <algorithm> // for std::sort
#include <cstdio> // for std::printf, std::snprintf
#include <cstring> // for std::memcpy
#include <optional> // for std::optional

#if SNATCH_WITH_TIMINGS
#    include <chrono> // for measuring test time
#endif

// Testing framework implementation utilities.
// -------------------------------------------

namespace {
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
} // namespace

namespace {
using snatch::small_string_span;

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
        static_assert(std::is_same_v<T, T>, "unsupported type");
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

namespace snatch {
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
} // namespace snatch

namespace snatch::impl {
void stdout_print(std::string_view message) noexcept {
    // TODO: replace this with std::print?
    std::printf("%.*s", static_cast<int>(message.length()), message.data());
}
} // namespace snatch::impl

namespace snatch::cli {
small_function<void(std::string_view) noexcept> console_print = &snatch::impl::stdout_print;
} // namespace snatch::cli

namespace {
using snatch::max_message_length;
using snatch::small_string;

template<typename T>
bool append(small_string_span ss, const colored<T>& colored_value) noexcept {
    return append(ss, colored_value.color_start, colored_value.value, colored_value.color_end);
}

template<typename... Args>
void console_print(Args&&... args) noexcept {
    small_string<max_message_length> message;
    append_or_truncate(message, std::forward<Args>(args)...);
    snatch::cli::console_print(message);
}

bool is_at_least(snatch::registry::verbosity verbose, snatch::registry::verbosity required) {
    using underlying_type = std::underlying_type_t<snatch::registry::verbosity>;
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

namespace snatch {
[[noreturn]] void terminate_with(std::string_view msg) noexcept {
    impl::stdout_print("terminate called with message: ");
    impl::stdout_print(msg);
    impl::stdout_print("\n");

    std::terminate();
}
} // namespace snatch

// Sections implementation.
// ------------------------

namespace snatch::impl {
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
                "please increase 'SNATCH_MAX_NESTED_SECTIONS' (currently ",
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
} // namespace snatch::impl

// Captures implementation.
// ------------------------

namespace snatch::impl {
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

small_string<max_capture_length>& add_capture(test_run& state) noexcept {
    if (state.captures.available() == 0) {
        state.reg.print(
            make_colored("error:", state.reg.with_color, color::fail),
            " max number of captures reached; "
            "please increase 'SNATCH_MAX_CAPTURES' (currently ",
            max_captures, ")\n.");
        std::terminate();
    }

    state.captures.grow(1);
    state.captures.back().clear();
    return state.captures.back();
}
} // namespace snatch::impl

// Matcher implementation.
// -----------------------

namespace snatch::matchers {
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
} // namespace snatch::matchers

// Testing framework implementation.
// ---------------------------------

namespace {
using namespace snatch;
using namespace snatch::impl;

template<typename F>
bool run_tests(registry& r, std::string_view run_name, F&& predicate) noexcept {
    if (!r.report_callback.empty()) {
        r.report_callback(r, event::test_run_started{run_name});
    } else if (is_at_least(r.verbose, registry::verbosity::normal)) {
        r.print(
            make_colored("starting tests with ", r.with_color, color::highlight2),
            make_colored("snatch v" SNATCH_FULL_VERSION "\n", r.with_color, color::highlight1));
        r.print("==========================================\n");
    }

    bool        success         = true;
    std::size_t run_count       = 0;
    std::size_t fail_count      = 0;
    std::size_t skip_count      = 0;
    std::size_t assertion_count = 0;

    for (test_case& t : r) {
        if (!predicate(t)) {
            continue;
        }

        auto state = r.run(t);

        ++run_count;
        assertion_count += state.asserts;
        if (t.state == test_state::failed) {
            ++fail_count;
            success = false;
        } else if (t.state == test_state::skipped) {
            ++skip_count;
        }
    }

    if (!r.report_callback.empty()) {
        r.report_callback(
            r, event::test_run_ended{
                   run_name, success, run_count, fail_count, skip_count, assertion_count});
    } else if (is_at_least(r.verbose, registry::verbosity::normal)) {
        r.print("==========================================\n");

        if (success) {
            r.print(
                make_colored("success:", r.with_color, color::pass), " all tests passed (",
                run_count, " test cases, ", assertion_count, " assertions");
        } else {
            r.print(
                make_colored("error:", r.with_color, color::fail), " some tests failed (",
                fail_count, " out of ", run_count, " test cases, ", assertion_count, " assertions");
        }

        if (skip_count > 0) {
            r.print(", ", skip_count, " test cases skipped");
        }

        r.print(")\n");
    }

    return success;
}

template<typename F>
void list_tests(const registry& r, F&& predicate) noexcept {
    for (const test_case& t : r) {
        if (!predicate(t)) {
            continue;
        }

        if (!t.id.type.empty()) {
            r.print(t.id.name, " [", t.id.type, "]\n");
        } else {
            r.print(t.id.name, "\n");
        }
    }
}

template<typename F>
void for_each_tag(std::string_view s, F&& callback) noexcept {
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

std::string_view
make_full_name(small_string<max_test_name_length>& buffer, const test_id& id) noexcept {
    buffer.clear();
    if (id.type.length() != 0) {
        if (!append(buffer, id.name, " [", id.type, "]")) {
            return {};
        }
    } else {
        if (!append(buffer, id.name)) {
            return {};
        }
    }

    return buffer.str();
}

void set_state(test_case& t, test_state s) noexcept {
    if (static_cast<std::underlying_type_t<test_state>>(t.state) <
        static_cast<std::underlying_type_t<test_state>>(s)) {
        t.state = s;
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

namespace snatch {
void registry::register_test(const test_id& id, test_ptr func) noexcept {
    if (test_list.size() == test_list.capacity()) {
        print(
            make_colored("error:", with_color, color::fail),
            " max number of test cases reached; "
            "please increase 'SNATCH_MAX_TEST_CASES' (currently ",
            max_test_cases, ")\n.");
        std::terminate();
    }

    test_list.push_back(test_case{id, func});

    small_string<max_test_name_length> buffer;
    if (make_full_name(buffer, test_list.back().id).empty()) {
        print(
            make_colored("error:", with_color, color::fail),
            " max length of test name reached; "
            "please increase 'SNATCH_MAX_TEST_NAME_LENGTH' (currently ",
            max_test_name_length, ")\n.");
        std::terminate();
    }
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
    impl::test_run&           state,
    const assertion_location& location,
    std::string_view          message) const noexcept {

    set_state(state.test, test_state::failed);

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message});
    } else {
        print_failure();
        print_location(state.test, state.sections, state.captures, location);
        print_details(message);
    }
}

void registry::report_failure(
    impl::test_run&           state,
    const assertion_location& location,
    std::string_view          message1,
    std::string_view          message2) const noexcept {

    set_state(state.test, test_state::failed);

    small_string<max_message_length> message;
    append_or_truncate(message, message1, message2);

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message});
    } else {
        print_failure();
        print_location(state.test, state.sections, state.captures, location);
        print_details(message);
    }
}

void registry::report_failure(
    impl::test_run&           state,
    const assertion_location& location,
    const impl::expression&   exp) const noexcept {

    set_state(state.test, test_state::failed);

    if (!report_callback.empty()) {
        const auto captures_buffer = make_capture_buffer(state.captures);
        if (!exp.actual.empty()) {
            small_string<max_message_length> message;
            append_or_truncate(message, exp.expected, ", got ", exp.actual);
            report_callback(
                *this, event::assertion_failed{
                           state.test.id, state.sections.current_section, captures_buffer.span(),
                           location, message});
        } else {
            report_callback(
                *this, event::assertion_failed{
                           state.test.id,
                           state.sections.current_section,
                           captures_buffer.span(),
                           location,
                           {exp.expected}});
        }
    } else {
        print_failure();
        print_location(state.test, state.sections, state.captures, location);
        print_details_expr(exp);
    }
}

void registry::report_skipped(
    impl::test_run&           state,
    const assertion_location& location,
    std::string_view          message) const noexcept {

    set_state(state.test, test_state::skipped);

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

test_run registry::run(test_case& test) noexcept {
    small_string<max_test_name_length> full_name;

    if (!report_callback.empty()) {
        report_callback(*this, event::test_case_started{test.id});
    } else if (is_at_least(verbose, verbosity::high)) {
        make_full_name(full_name, test.id);
        print(
            make_colored("starting:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), "\n");
    }

    test.state = test_state::success;

    test_run state {
        .reg = *this, .test = test, .sections = {}, .captures = {}, .asserts = 0,
#if SNATCH_WITH_TIMINGS
        .duration = 0.0f
#endif
    };

#if SNATCH_WITH_TIMINGS
    using clock     = std::chrono::high_resolution_clock;
    auto time_start = clock::now();
#endif

    do {
        for (std::size_t i = 0; i < state.sections.levels.size(); ++i) {
            state.sections.levels[i].current_section_id = 0;
        }

        state.sections.leaf_executed = false;

#if SNATCH_WITH_EXCEPTIONS
        try {
            test.func(state);
        } catch (const impl::abort_exception&) {
            // Test aborted, assume its state was already set accordingly.
        } catch (const std::exception& e) {
            report_failure(
                state, {__FILE__, __LINE__}, "unhandled std::exception caught; message:", e.what());
        } catch (...) {
            report_failure(state, {__FILE__, __LINE__}, "unhandled unknown exception caught");
        }
#else
        test.func(state);
#endif

        if (state.sections.levels.size() == 1) {
            auto& child = state.sections.levels[0];
            if (child.previous_section_id == child.max_section_id) {
                state.sections.levels.clear();
                state.sections.current_section.clear();
            }
        }
    } while (!state.sections.levels.empty());

#if SNATCH_WITH_TIMINGS
    auto time_end  = clock::now();
    state.duration = std::chrono::duration<float>(time_end - time_start).count();
#endif

    if (!report_callback.empty()) {
#if SNATCH_WITH_TIMINGS
        report_callback(*this, event::test_case_ended{test.id, state.duration});
#else
        report_callback(*this, event::test_case_ended{test.id});
#endif
    } else if (is_at_least(verbose, verbosity::high)) {
#if SNATCH_WITH_TIMINGS
        print(
            make_colored("finished:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), " (", state.duration, "s)\n");
#else
        print(
            make_colored("finished:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), "\n");
#endif
    }

    return state;
}

bool registry::run_all_tests(std::string_view run_name) noexcept {
    return ::run_tests(*this, run_name, [](const test_case&) { return true; });
}

bool registry::run_tests_matching_name(
    std::string_view run_name, std::string_view name_filter) noexcept {
    small_string<max_test_name_length> buffer;
    return ::run_tests(*this, run_name, [&](const test_case& t) {
        std::string_view v = make_full_name(buffer, t.id);

        // TODO: use regex here?
        return v.find(name_filter) != v.npos;
    });
}

bool registry::run_tests_with_tag(std::string_view run_name, std::string_view tag_filter) noexcept {
    return ::run_tests(*this, run_name, [&](const test_case& t) {
        bool selected = false;
        for_each_tag(t.id.tags, [&](std::string_view v) {
            if (v == tag_filter) {
                selected = true;
            }
        });
        return selected;
    });
}

void registry::list_all_tags() const noexcept {
    small_vector<std::string_view, max_unique_tags> tags;
    for (const auto& t : test_list) {
        for_each_tag(t.id.tags, [&](std::string_view v) {
            if (std::find(tags.begin(), tags.end(), v) == tags.end()) {
                if (tags.size() == tags.capacity()) {
                    print(
                        make_colored("error:", with_color, color::fail),
                        " max number of tags reached; "
                        "please increase 'SNATCH_MAX_UNIQUE_TAGS' (currently ",
                        max_unique_tags, ")\n.");
                    std::terminate();
                }

                tags.push_back(v);
            }
        });
    }

    std::sort(tags.begin(), tags.end());

    for (const auto& t : tags) {
        print(t, "\n");
    }
}

void registry::list_all_tests() const noexcept {
    list_tests(*this, [](const test_case&) { return true; });
}

void registry::list_tests_with_tag(std::string_view tag) const noexcept {
    list_tests(*this, [&](const test_case& t) {
        bool selected = false;
        for_each_tag(t.id.tags, [&](std::string_view v) {
            if (v == tag) {
                selected = true;
            }
        });
        return selected;
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
    r.with_color = SNATCH_DEFAULT_WITH_COLOR == 1;
    return r;
}();
} // namespace snatch

// Main entry point utilities.
// ---------------------------

namespace {
using namespace std::literals;

constexpr std::size_t max_arg_names = 2;

enum class argument_type { optional, mandatory };

struct expected_argument {
    small_vector<std::string_view, max_arg_names> names;
    std::optional<std::string_view>               value_name;
    std::string_view                              description;
    argument_type                                 type = argument_type::optional;
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
        if (!e.names.empty()) {
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
            if (!e.value_name.has_value()) {
                terminate_with("positional argument must have a value name");
            }
        }
    }

    // Parse
    for (int argi = 1; argi < argc; ++argi) {
        std::string_view arg(argv[argi]);
        if (arg.starts_with('-')) {
            bool found = false;
            for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
                const auto& e = expected[arg_index];

                if (e.names.empty()) {
                    continue;
                }

                if (std::find(e.names.cbegin(), e.names.cend(), arg) == e.names.cend()) {
                    continue;
                }

                found = true;

                if (expected_found[arg_index]) {
                    console_print(
                        make_colored("error:", settings.with_color, color::error),
                        " duplicate command line argument '", arg, "'\n");
                    bad = true;
                    break;
                }

                expected_found[arg_index] = true;

                if (e.value_name) {
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
                    args.push_back(cli::argument{e.names.back(), {}, {}});
                }

                break;
            }

            if (!found) {
                console_print(
                    make_colored("warning:", settings.with_color, color::warning),
                    " unknown command line argument '", arg, "'\n");
            }
        } else {
            bool found = false;
            for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
                const auto& e = expected[arg_index];

                if (!e.names.empty() || expected_found[arg_index]) {
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
        if (e.type == argument_type::mandatory && !expected_found[arg_index]) {
            if (e.names.empty()) {
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

    // Print program desription
    console_print(make_colored(program_description, settings.with_color, color::highlight2), "\n");

    // Print command line usage example
    console_print(make_colored("Usage:", settings.with_color, color::pass), "\n");
    console_print("  ", program_name);
    if (std::any_of(expected.cbegin(), expected.cend(), [](auto& e) { return !e.names.empty(); })) {
        console_print(" [options...]");
    }

    for (const auto& e : expected) {
        if (e.names.empty()) {
            if (e.type == argument_type::mandatory) {
                console_print(" <", *e.value_name, ">");
            } else {
                console_print(" [<", *e.value_name, ">]");
            }
        }
    }

    console_print("\n\n");

    // List arguments
    small_string<max_message_length> heading;
    for (const auto& e : expected) {
        heading.clear();

        bool success = true;
        if (!e.names.empty()) {
            if (e.names[0].starts_with("--")) {
                success = success && append(heading, "    ");
            }

            success = success && append(heading, e.names[0]);

            if (e.names.size() == 2) {
                success = success && append(heading, ", ", e.names[1]);
            }

            if (e.value_name) {
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
const expected_arguments expected_args = {
    {{"-l", "--list-tests"},    {},                    "List tests by name"},
    {{"--list-tags"},           {},                    "List tags by name"},
    {{"--list-tests-with-tag"}, {"[tag]"},             "List tests by name with a given tag"},
    {{"-t", "--tags"},          {},                    "Use tags for filtering, not name"},
    {{"-v", "--verbosity"},     {"quiet|normal|high"}, "Define how much gets sent to the standard output"},
    {{"--color"},               {"always|never"},      "Enable/disable color in output"},
    {{"-h", "--help"},          {},                    "Print help"},
    {{},                        {"test regex"},        "A regex to select which test cases (or tags) to run"}};
// clang-format on

constexpr bool with_color_default = SNATCH_DEFAULT_WITH_COLOR == 1;

constexpr const char* program_description = "Test runner (snatch v" SNATCH_FULL_VERSION ")";
} // namespace

namespace snatch::cli {
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
        return arg.name.empty() && arg.value_name == name;
    });

    if (iter != args.arguments.cend()) {
        ret = *iter;
    }

    return ret;
}
} // namespace snatch::cli

namespace snatch {
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
            verbose = snatch::registry::verbosity::quiet;
        } else if (*opt->value == "normal") {
            verbose = snatch::registry::verbosity::normal;
        } else if (*opt->value == "high") {
            verbose = snatch::registry::verbosity::high;
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

    if (auto opt = get_positional_argument(args, "test regex")) {
        if (get_option(args, "--tags")) {
            return run_tests_with_tag(args.executable, *opt->value);
        } else {
            return run_tests_matching_name(args.executable, *opt->value);
        }
    } else {
        return run_all_tests(args.executable);
    }
}
} // namespace snatch

#if SNATCH_DEFINE_MAIN

// Main entry point.
// -----------------

int main(int argc, char* argv[]) {
    std::optional<snatch::cli::input> args = snatch::cli::parse_arguments(argc, argv);
    if (!args) {
        return 1;
    }

    snatch::tests.configure(*args);

    return snatch::tests.run_tests(*args) ? 0 : 1;
}

#endif
