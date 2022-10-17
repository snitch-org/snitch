#include "snatch/snatch.hpp"

#include <algorithm> // for std::sort
#include <cstdio> // for std::printf
#include <cstring> // for std::memcpy
#include <optional> // for std::optional

#if !defined(SNATCH_DEFINE_MAIN)
#    define SNATCH_DEFINE_MAIN 1
#endif
#if !defined(SNATCH_WITH_CXXOPTS)
#    define SNATCH_WITH_CXXOPTS 0
#endif

// Testing framework implementation utilities.
// -------------------------------------------

namespace { namespace color {
const char* error_start [[maybe_unused]]      = "\x1b[1;31m";
const char* warning_start [[maybe_unused]]    = "\x1b[1;33m";
const char* status_start [[maybe_unused]]     = "\x1b[1;36m";
const char* fail_start [[maybe_unused]]       = "\x1b[1;31m";
const char* skipped_start [[maybe_unused]]    = "\x1b[1;33m";
const char* pass_start [[maybe_unused]]       = "\x1b[1;32m";
const char* highlight1_start [[maybe_unused]] = "\x1b[1;35m";
const char* highlight2_start [[maybe_unused]] = "\x1b[1;36m";
const char* reset [[maybe_unused]]            = "\x1b[0m";
}} // namespace ::color

namespace {
using namespace snatch;
using namespace snatch::impl;

template<typename F>
bool run_tests(registry& r, F&& predicate) noexcept {
    bool        success         = true;
    std::size_t run_count       = 0;
    std::size_t fail_count      = 0;
    std::size_t assertion_count = 0;

    for (test_case& t : r) {
        if (!predicate(t)) {
            continue;
        }

        r.run(t);

        ++run_count;
        assertion_count += t.tests;
        if (t.state == test_state::failed) {
            ++fail_count;
            success = false;
        }
    }

    std::printf("==========================================\n");
    if (success) {
        std::printf(
            "%ssuccess:%s all tests passed (%zu test cases, %zu assertions)\n", color::pass_start,
            color::reset, run_count, assertion_count);
    } else {
        std::printf(
            "%serror:%s some tests failed (%zu out of %zu test cases, %zu assertions)\n",
            color::fail_start, color::reset, fail_count, run_count, assertion_count);
    }

    return success;
}

template<typename F>
void list_tests(const registry& r, F&& predicate) noexcept {
    for (const test_case& t : r) {
        if (!predicate(t)) {
            continue;
        }

        if (!t.type.empty()) {
            std::printf(
                "%.*s [%.*s]\n", static_cast<int>(t.name.length()), t.name.data(),
                static_cast<int>(t.type.length()), t.type.data());
        } else {
            std::printf("%.*s\n", static_cast<int>(t.name.length()), t.name.data());
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
make_full_name(small_string<max_test_name_length>& buffer, const test_case& t) noexcept {
    buffer.clear();
    if (t.type.length() != 0) {
        if (!append(buffer, t.name, " [", t.type, "]")) {
            return {};
        }
    } else {
        if (!append(buffer, t.name)) {
            return {};
        }
    }

    return buffer.str();
}

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

// Testing framework implementation.
// ---------------------------------

namespace snatch::impl {
[[noreturn]] void terminate_with(std::string_view msg) noexcept {
    std::printf(
        "terminate called with message: %.*s\n", static_cast<int>(msg.length()), msg.data());
    std::terminate();
}

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

} // namespace snatch::impl

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

namespace snatch {
void registry::register_test(
    std::string_view name, std::string_view tags, std::string_view type, test_ptr func) noexcept {

    if (test_list.size() == test_list.capacity()) {
        std::printf(
            "%serror:%s max number of test cases reached; "
            "please increase 'SNATCH_MAX_TEST_CASES' (currently %zu)\n.",
            color::error_start, color::reset, max_test_cases);
        std::terminate();
    }

    test_list.push_back(test_case{name, tags, type, func});

    small_string<max_test_name_length> buffer;
    if (make_full_name(buffer, test_list.back()).empty()) {
        std::printf(
            "%serror:%s max length of test name reached; "
            "please increase 'SNATCH_MAX_TEST_NAME_LENGTH' (currently %zu)\n.",
            color::error_start, color::reset, max_test_name_length);
        std::terminate();
    }
}

void registry::print_location(
    const test_case& current_case, const char* filename, int line_number) const noexcept {

    std::printf(
        "running test case \"%s%.*s%s\"\n"
        "          at %s:%d\n",
        color::highlight1_start, static_cast<int>(current_case.name.length()),
        current_case.name.data(), color::reset, filename, line_number);

    if (!current_case.type.empty()) {
        std::printf(
            "          for type %s%.*s%s\n", color::highlight1_start,
            static_cast<int>(current_case.type.length()), current_case.type.data(), color::reset);
    }
}

void registry::print_failure() const noexcept {
    std::printf("%sfailed:%s ", color::fail_start, color::reset);
}
void registry::print_skip() const noexcept {
    std::printf("%sskipped:%s ", color::skipped_start, color::reset);
}

void registry::print_details(std::string_view message) const noexcept {
    std::printf(
        "          %s%.*s%s\n", color::highlight2_start, static_cast<int>(message.length()),
        message.data(), color::reset);
}

void registry::print_details_expr(
    std::string_view check, std::string_view exp_str, const expression& exp) const noexcept {
    std::printf(
        "          %s%.*s(%.*s)%s", color::highlight2_start, static_cast<int>(check.length()),
        check.data(), static_cast<int>(exp_str.length()), exp_str.data(), color::reset);
    if (!exp.failed) {
        auto str = exp.data.str();
        std::printf(
            ", got %s%.*s%s\n", color::highlight2_start, static_cast<int>(str.length()), str.data(),
            color::reset);
    } else {
        std::printf("\n");
    }
}

void registry::run(test_case& t) noexcept {
    if (verbose) {
        std::printf("%sstarting:%s %s", color::status_start, color::reset, color::highlight1_start);
        if (!t.type.empty()) {
            std::printf(
                "%.*s [%.*s]", static_cast<int>(t.name.length()), t.name.data(),
                static_cast<int>(t.type.length()), t.type.data());
        } else {
            std::printf("%.*s", static_cast<int>(t.name.length()), t.name.data());
        }
        std::printf("%s.\n", color::reset);
    }

    t.tests = 0;
    t.state = test_state::success;

    try {
        t.func(t);
    } catch (const test_state& s) {
        t.state = s;
    } catch (const std::exception& e) {
        print_failure();
        print_location(t, __FILE__, __LINE__);
        print_details("unhandled std::exception caught; message:");
        print_details(e.what());
        t.state = test_state::failed;
    } catch (...) {
        print_failure();
        print_location(t, __FILE__, __LINE__);
        print_details("unhandled unknown exception caught");
        t.state = test_state::failed;
    }

    if (verbose) {
        std::printf("%sfinished:%s %s", color::status_start, color::reset, color::highlight1_start);
        if (!t.type.empty()) {
            std::printf(
                "%.*s [%.*s]", static_cast<int>(t.name.length()), t.name.data(),
                static_cast<int>(t.type.length()), t.type.data());
        } else {
            std::printf("%.*s", static_cast<int>(t.name.length()), t.name.data());
        }
        std::printf("%s.\n", color::reset);
    }
}

void registry::set_state(test_case& t, test_state s) noexcept {
    if (static_cast<std::underlying_type_t<test_state>>(t.state) <
        static_cast<std::underlying_type_t<test_state>>(s)) {
        t.state = s;
    }
}

bool registry::run_all_tests() noexcept {
    return run_tests(*this, [](const test_case&) { return true; });
}

bool registry::run_tests_matching_name(std::string_view name) noexcept {
    small_string<max_test_name_length> buffer;
    return run_tests(*this, [&](const test_case& t) {
        std::string_view v = make_full_name(buffer, t);

        // TODO: use regex here?
        return v.find(name) != t.name.npos;
    });
}

bool registry::run_tests_with_tag(std::string_view tag) noexcept {
    return run_tests(*this, [&](const test_case& t) {
        bool selected = false;
        for_each_tag(t.tags, [&](std::string_view v) {
            if (v == tag) {
                selected = true;
            }
        });
        return selected;
    });
}

void registry::list_all_tags() const noexcept {
    impl::small_vector<std::string_view, max_unique_tags> tags;
    for (const auto& t : test_list) {
        for_each_tag(t.tags, [&](std::string_view v) {
            if (std::find(tags.begin(), tags.end(), v) == tags.end()) {
                if (tags.size() == tags.capacity()) {
                    std::printf(
                        "%serror:%s max number of tags reached; "
                        "please increase 'SNATCH_MAX_UNIQUE_TAGS' (currently %zu)\n.",
                        color::error_start, color::reset, max_unique_tags);
                    std::terminate();
                }

                tags.push_back(v);
            }
        });
    }

    std::sort(tags.begin(), tags.end());

    for (auto c : tags) {
        std::printf("%.*s\n", static_cast<int>(c.length()), c.data());
    }
}

void registry::list_all_tests() const noexcept {
    list_tests(*this, [](const test_case&) { return true; });
}

void registry::list_tests_with_tag(std::string_view tag) const noexcept {
    list_tests(*this, [&](const test_case& t) {
        bool selected = false;
        for_each_tag(t.tags, [&](std::string_view v) {
            if (v == tag) {
                selected = true;
            }
        });
        return selected;
    });
}

impl::test_case* registry::begin() noexcept {
    return test_list.begin();
}

impl::test_case* registry::end() noexcept {
    return test_list.end();
}

const impl::test_case* registry::begin() const noexcept {
    return test_list.begin();
}

const impl::test_case* registry::end() const noexcept {
    return test_list.end();
}

constinit registry tests;
} // namespace snatch

#if SNATCH_DEFINE_MAIN

// Main entry point utilities.
// ---------------------------

namespace {
using namespace std::literals;

constexpr std::size_t max_command_line_args = 1024;
constexpr std::size_t max_arg_names         = 2;

enum class argument_type { optional, mandatory };

struct expected_argument {
    small_vector<std::string_view, max_arg_names> names;
    std::optional<std::string_view>               value_name;
    std::string_view                              description;
    argument_type                                 type = argument_type::optional;
};

using expected_arguments = small_vector<expected_argument, max_command_line_args>;

struct argument {
    std::string_view                name;
    std::optional<std::string_view> value_name;
    std::optional<std::string_view> value;
};

using arguments = small_vector<argument, max_command_line_args>;

std::optional<arguments>
parse_arguments(int argc, char* argv[], const expected_arguments& expected) noexcept {
    std::optional<arguments> ret(std::in_place);
    auto&                    args = *ret;
    bool                     bad  = false;

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
                    printf(
                        "%serror:%s duplicate command line argument '%.*s'\n", color::error_start,
                        color::reset, static_cast<int>(arg.size()), arg.data());
                    bad = true;
                    break;
                }

                expected_found[arg_index] = true;

                if (e.value_name) {
                    if (argi + 1 == argc) {
                        printf(
                            "%serror:%s missing value '<%.*s>' for command line argument '%.*s'\n",
                            color::error_start, color::reset,
                            static_cast<int>(e.value_name->size()), e.value_name->data(),
                            static_cast<int>(arg.size()), arg.data());
                        bad = true;
                        break;
                    }

                    argi += 1;
                    args.push_back(
                        argument{e.names.back(), e.value_name, {std::string_view(argv[argi])}});
                } else {
                    args.push_back(argument{e.names.back(), {}, {}});
                }

                break;
            }

            if (!found) {
                printf(
                    "%swarning:%s unknown command line argument '%.*s'\n", color::warning_start,
                    color::reset, static_cast<int>(arg.size()), arg.data());
            }
        } else {
            bool found = false;
            for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
                const auto& e = expected[arg_index];

                if (!e.names.empty() || expected_found[arg_index]) {
                    continue;
                }

                found = true;

                args.push_back(argument{""sv, e.value_name, {arg}});
                expected_found[arg_index] = true;
                break;
            }

            if (!found) {
                printf(
                    "%serror:%s too many positional arguments\n", color::error_start, color::reset);
                bad = true;
            }
        }
    }

    for (std::size_t arg_index = 0; arg_index < expected.size(); ++arg_index) {
        const auto& e = expected[arg_index];
        if (e.type == argument_type::mandatory && !expected_found[arg_index]) {
            if (e.names.empty()) {
                printf(
                    "%serror:%s missing positional argument '<%.*s>'\n", color::error_start,
                    color::reset, static_cast<int>(e.value_name->size()), e.value_name->data());
            } else {
                printf(
                    "%serror:%s missing option '<%.*s>'\n", color::error_start, color::reset,
                    static_cast<int>(e.names.back().size()), e.names.back().data());
            }
            bad = true;
        }
    }

    if (bad) {
        ret.reset();
    }

    return ret;
}

void print_help(
    std::string_view          program_name,
    std::string_view          program_description,
    const expected_arguments& expected) {

    // Print program desription
    printf(
        "%s%.*s%s\n", color::highlight2_start, static_cast<int>(program_description.size()),
        program_description.data(), color::reset);

    // Print command line usage example
    printf("%sUsage:%s\n", color::pass_start, color::reset);
    printf("  %.*s", static_cast<int>(program_name.size()), program_name.data());
    if (std::any_of(expected.cbegin(), expected.cend(), [](auto& e) { return !e.names.empty(); })) {
        printf(" [options...]");
    }
    for (const auto& e : expected) {
        if (e.names.empty()) {
            if (e.type == argument_type::mandatory) {
                printf(" <%.*s>", static_cast<int>(e.value_name->size()), e.value_name->data());
            } else {
                printf(" [<%.*s>]", static_cast<int>(e.value_name->size()), e.value_name->data());
            }
        }
    }
    printf("\n\n");

    // List arguments
    for (const auto& e : expected) {
        printf("  ");
        printf("%s", color::highlight1_start);
        if (!e.names.empty()) {
            if (e.names[0].starts_with("--")) {
                printf("    ");
            }

            printf("%.*s", static_cast<int>(e.names[0].size()), e.names[0].data());
            if (e.names.size() == 2) {
                printf(", %.*s", static_cast<int>(e.names[1].size()), e.names[1].data());
            }

            if (e.value_name) {
                printf(" <%.*s>", static_cast<int>(e.value_name->size()), e.value_name->data());
            }
        } else {
            printf("<%.*s>", static_cast<int>(e.value_name->size()), e.value_name->data());
        }
        printf("%s", color::reset);
        printf(" %.*s\n", static_cast<int>(e.description.size()), e.description.data());
    }
}

std::optional<argument> get_option(const arguments& args, std::string_view name) {
    std::optional<argument> ret;

    auto iter =
        std::find_if(args.cbegin(), args.cend(), [&](const auto& arg) { return arg.name == name; });

    if (iter != args.cend()) {
        ret = *iter;
    }

    return ret;
}

std::optional<argument> get_positional_argument(const arguments& args, std::string_view name) {
    std::optional<argument> ret;

    auto iter = std::find_if(args.cbegin(), args.cend(), [&](const auto& arg) {
        return arg.name.empty() && arg.value_name == name;
    });

    if (iter != args.cend()) {
        ret = *iter;
    }

    return ret;
}
} // namespace

// Main entry point.
// -----------------

int main(int argc, char* argv[]) {
    // clang-format off
    const expected_arguments expected = {
        {{"-l", "--list-tests"},    {},             "List tests by name"},
        {{"--list-tags"},           {},             "List tags by name"},
        {{"--list-tests-with-tag"}, {"tag"},        "List tests by name with a given tag"},
        {{"-t", "--tags"},          {},             "Use tags for filtering, not name"},
        {{"-v", "--verbose"},       {},             "Enable detailed output"},
        {{"-h", "--help"},          {},             "Print help"},
        {{},                        {"test regex"}, "A regex to select which test cases (or tags) to run"}};
    // clang-format on

    std::optional<arguments> ret_args = parse_arguments(argc, argv, expected);
    if (!ret_args) {
        printf("\n");
        print_help(argv[0], "Snatch test runner"sv, expected);
        return 1;
    }

    arguments& args = *ret_args;

    if (get_option(args, "--help")) {
        print_help(argv[0], "Snatch test runner"sv, expected);
        return 0;
    }

    if (get_option(args, "--list-tests")) {
        snatch::tests.list_all_tests();
        return 0;
    }

    if (auto opt = get_option(args, "--list-tests-with-tag")) {
        snatch::tests.list_tests_with_tag(*opt->value);
        return 0;
    }

    if (get_option(args, "--list-tags")) {
        snatch::tests.list_all_tags();
        return 0;
    }

    if (get_option(args, "--verbose")) {
        snatch::tests.verbose = true;
    }

    bool success = true;
    if (auto opt = get_positional_argument(args, "test regex")) {
        if (get_option(args, "--tags")) {
            success = snatch::tests.run_tests_with_tag(*opt->value);
        } else {
            success = snatch::tests.run_tests_matching_name(*opt->value);
        }
    } else {
        success = snatch::tests.run_all_tests();
    }

    return success ? 0 : 1;
}

#endif
