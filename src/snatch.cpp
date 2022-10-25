#include "snatch/snatch.hpp"

#include <algorithm> // for std::sort
#include <cstdio> // for std::printf, std::snprintf
#include <cstring> // for std::memcpy
#include <optional> // for std::optional

#if !defined(SNATCH_DEFINE_MAIN)
#    define SNATCH_DEFINE_MAIN 1
#endif
#if !defined(SNATCH_WITH_COLOR)
#    define SNATCH_WITH_COLOR 1
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
    const int return_code = std::snprintf(ss.end(), 0, get_format_code<T>(), value);
    if (return_code < 0) {
        return false;
    }

    const std::size_t length    = static_cast<std::size_t>(return_code);
    const bool        could_fit = length <= ss.available();

    const std::size_t offset     = ss.size();
    const std::size_t prev_space = ss.available();
    ss.resize(std::min(ss.size() + length, ss.capacity()));
    std::snprintf(ss.begin() + offset, prev_space, get_format_code<T>(), value);

    return could_fit;
}
} // namespace

namespace snatch {
bool append(small_string_span ss, std::string_view str) noexcept {
    const bool        could_fit  = str.size() <= ss.available();
    const std::size_t copy_count = std::min(str.size(), ss.available());

    const std::size_t offset = ss.size();
    ss.grow(copy_count);
    std::memcpy(ss.begin() + offset, str.data(), copy_count);

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
        auto             pos = sv.find_first_of(pattern);

        while (pos != sv.npos) {
            // Replace pattern by replacement
            std::memcpy(string.begin() + pos, replacement.begin(), replacement.size());
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
            std::memcpy(string.begin() + pos, replacement.begin(), replacement.size());
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
            std::rotate(string.begin() + pos, string.end() - char_growth, string.end());

            // Replace pattern by replacement
            const std::size_t max_chars = pattern.size() + char_growth;
            std::memcpy(string.begin() + pos, replacement.begin(), max_chars);
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

namespace {
using snatch::max_message_length;
using snatch::small_string;
using snatch::impl::stdout_print;

template<typename T>
bool append(small_string_span ss, const colored<T>& colored_value) noexcept {
    return append(ss, colored_value.color_start, colored_value.value, colored_value.color_end);
}

template<typename... Args>
void console_print(Args&&... args) noexcept {
    small_string<max_message_length> message;
    if (!append(message, std::forward<Args>(args)...)) {
        truncate_end(message);
    }

    stdout_print(message);
}

bool is_at_least(snatch::registry::verbosity verbose, snatch::registry::verbosity required) {
    using underlying_type = std::underlying_type_t<snatch::registry::verbosity>;
    return static_cast<underlying_type>(verbose) >= static_cast<underlying_type>(required);
}
} // namespace

namespace snatch {
[[noreturn]] void terminate_with(std::string_view msg) noexcept {
    console_print("terminate called with message: ", msg, "\n");
    std::terminate();
}
} // namespace snatch

// Matcher implementation.
// -----------------------

namespace snatch::matchers {
contains_substring::contains_substring(std::string_view pattern) noexcept :
    substring_pattern(pattern) {}

bool contains_substring::match(std::string_view message) const noexcept {
    return message.find(substring_pattern) != message.npos;
}

std::string_view contains_substring::describe_fail(std::string_view message) const noexcept {
    description_buffer.clear();
    if (!append(
            description_buffer, "could not find '", substring_pattern, "' in '", message, "'")) {
        truncate_end(description_buffer);
    }

    return description_buffer.str();
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

        r.run(t);

        ++run_count;
        assertion_count += t.asserts;
        if (t.state == test_state::failed) {
            ++fail_count;
            success = false;
        } else if (t.state == test_state::skipped) {
            ++skip_count;
        }
    }

    if (!r.report_callback.empty()) {
        r.report_callback(r, event::test_run_ended{run_name});
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
            console_print(t.id.name, " [", t.id.type, "]\n");
        } else {
            console_print(t.id.name);
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
    const impl::test_case& current_case, const assertion_location& location) const noexcept {

    print(
        "running test case \"", make_colored(current_case.id.name, with_color, color::highlight1),
        "\"\n");
    print("          at ", location.file, ":", location.line, "\n");

    if (!current_case.id.type.empty()) {
        print(
            "          for type ",
            make_colored(current_case.id.type, with_color, color::highlight1), "\n");
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
    print("          ", make_colored(exp.content, with_color, color::highlight2));

    if (!exp.failed) {
        print(", got ", make_colored(exp.data, with_color, color::highlight2));
    }

    print("\n");
}

void registry::report_failure(
    impl::test_case&          t,
    const assertion_location& location,
    std::string_view          message) const noexcept {

    set_state(t, test_state::failed);

    if (!report_callback.empty()) {
        report_callback(*this, event::assertion_failed{t.id, location, message});
    } else {
        print_failure();
        print_location(t, location);
        print_details(message);
    }
}

void registry::report_failure(
    impl::test_case&          t,
    const assertion_location& location,
    std::string_view          message1,
    std::string_view          message2) const noexcept {

    set_state(t, test_state::failed);

    small_string<max_message_length> message;
    if (!append(message, message1, message2)) {
        truncate_end(message);
    }

    if (!report_callback.empty()) {
        report_callback(*this, event::assertion_failed{t.id, location, message});
    } else {
        print_failure();
        print_location(t, location);
        print_details(message);
    }
}

void registry::report_failure(
    impl::test_case&          t,
    const assertion_location& location,
    const impl::expression&   exp) const noexcept {

    set_state(t, test_state::failed);

    if (!report_callback.empty()) {
        if (!exp.failed) {
            small_string<max_message_length> message;
            if (!append(message, exp.content, ", got ", exp.data)) {
                truncate_end(message);
            }
            report_callback(*this, event::assertion_failed{t.id, location, message});
        } else {
            report_callback(*this, event::assertion_failed{t.id, location, {exp.content}});
        }
    } else {
        print_failure();
        print_location(t, location);
        print_details_expr(exp);
    }
}

void registry::report_skipped(
    impl::test_case&          t,
    const assertion_location& location,
    std::string_view          message) const noexcept {

    set_state(t, test_state::skipped);

    if (!report_callback.empty()) {
        report_callback(*this, event::test_case_skipped{t.id, location, message});
    } else {
        print_skip();
        print_location(t, location);
        print_details(message);
    }
}

void registry::run(test_case& t) noexcept {
    small_string<max_test_name_length> full_name;

    if (!report_callback.empty()) {
        report_callback(*this, event::test_case_started{t.id});
    } else if (is_at_least(verbose, verbosity::high)) {
        make_full_name(full_name, t.id);
        print(
            make_colored("starting:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), "\n");
    }

    t.asserts = 0;
    t.state   = test_state::success;

#if SNATCH_WITH_EXCEPTIONS
    try {
        t.func(t);
    } catch (const impl::abort_exception&) {
        // Test aborted, assume its state was already set accordingly.
    } catch (const std::exception& e) {
        report_failure(
            t, {__FILE__, __LINE__}, "unhandled std::exception caught; message:", e.what());
    } catch (...) {
        report_failure(t, {__FILE__, __LINE__}, "unhandled unknown exception caught");
    }
#else
    t.func(t);
#endif

    if (!report_callback.empty()) {
        report_callback(*this, event::test_case_ended{t.id});
    } else if (is_at_least(verbose, verbosity::high)) {
        print(
            make_colored("finished:", with_color, color::status), " ",
            make_colored(full_name, with_color, color::highlight1), "\n");
    }
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
    char*                     argv[],
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

std::optional<cli::argument> get_option(const cli::input& args, std::string_view name) {
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
get_positional_argument(const cli::input& args, std::string_view name) {
    std::optional<cli::argument> ret;

    auto iter = std::find_if(args.arguments.cbegin(), args.arguments.cend(), [&](const auto& arg) {
        return arg.name.empty() && arg.value_name == name;
    });

    if (iter != args.arguments.cend()) {
        ret = *iter;
    }

    return ret;
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

constexpr bool with_color_default = SNATCH_WITH_COLOR == 1;
} // namespace

namespace snatch::cli {
std::optional<cli::input> parse_arguments(int argc, char* argv[]) noexcept {
    std::optional<cli::input> ret_args =
        parse_arguments(argc, argv, expected_args, {.with_color = with_color_default});

    if (!ret_args) {
        console_print("\n");
        print_help(
            argv[0], "Snatch test runner"sv, expected_args, {.with_color = with_color_default});
    }

    return ret_args;
}
} // namespace snatch::cli

namespace snatch {
void registry::configure(const cli::input& args) noexcept {
    if (auto opt = get_option(args, "--color")) {
        if (*opt->value == "always") {
            snatch::tests.with_color = true;
        } else if (*opt->value == "never") {
            snatch::tests.with_color = false;
        } else {
            console_print(
                make_colored("warning:", snatch::tests.with_color, color::warning),
                "unknown color directive; please use one of always|never\n");
        }
    }

    if (auto opt = get_option(args, "--verbosity")) {
        if (*opt->value == "quiet") {
            snatch::tests.verbose = snatch::registry::verbosity::quiet;
        } else if (*opt->value == "normal") {
            snatch::tests.verbose = snatch::registry::verbosity::normal;
        } else if (*opt->value == "high") {
            snatch::tests.verbose = snatch::registry::verbosity::high;
        } else {
            console_print(
                make_colored("warning:", snatch::tests.with_color, color::warning),
                "unknown verbosity level; please use one of quiet|normal|high\n");
        }
    }
}

bool registry::run_tests(const cli::input& args) noexcept {
    if (get_option(args, "--help")) {
        console_print("\n");
        print_help(
            args.executable, "Snatch test runner"sv, expected_args,
            {.with_color = with_color_default});
        return true;
    }

    if (get_option(args, "--list-tests")) {
        snatch::tests.list_all_tests();
        return true;
    }

    if (auto opt = get_option(args, "--list-tests-with-tag")) {
        snatch::tests.list_tests_with_tag(*opt->value);
        return true;
    }

    if (get_option(args, "--list-tags")) {
        snatch::tests.list_all_tags();
        return true;
    }

    if (auto opt = get_positional_argument(args, "test regex")) {
        if (get_option(args, "--tags")) {
            return snatch::tests.run_tests_with_tag(args.executable, *opt->value);
        } else {
            return snatch::tests.run_tests_matching_name(args.executable, *opt->value);
        }
    } else {
        return snatch::tests.run_all_tests(args.executable);
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
