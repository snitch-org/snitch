#include "snitch/snitch_registry.hpp"

#include <algorithm> // for std::sort
#include <optional> // for std::optional

#if SNITCH_WITH_TIMINGS
#    include <chrono> // for measuring test time
#endif

// Testing framework implementation.
// ---------------------------------

namespace snitch::impl {
namespace {
using namespace std::literals;

// Requires: s contains a well-formed list of tags.
template<typename F>
void for_each_raw_tag(std::string_view s, F&& callback) {
    if (s.empty()) {
        return;
    }

    if (s.find_first_of("[") == std::string_view::npos ||
        s.find_first_of("]") == std::string_view::npos) {
        assertion_failed("incorrectly formatted tag; please use \"[tag1][tag2][...]\"");
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

// Requires: s contains a well-formed list of tags, each of length <= max_tag_length.
template<typename F>
void for_each_tag(std::string_view s, F&& callback) {
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
                assertion_failed("tag is too long");
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

template<typename F>
void list_tests(const registry& r, F&& predicate) noexcept {
    small_string<max_test_name_length> buffer;
    for (const test_case& t : r) {
        if (!predicate(t)) {
            continue;
        }

        cli::print(make_full_name(buffer, t.id), "\n");
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

small_vector<std::string_view, max_captures>
make_capture_buffer(const capture_state& captures) noexcept {
    small_vector<std::string_view, max_captures> captures_buffer;
    for (const auto& c : captures) {
        captures_buffer.push_back(c);
    }

    return captures_buffer;
}
} // namespace

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
} // namespace snitch::impl

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
    impl::for_each_tag(tags, [&](const impl::tags::parsed_tag& v) {
        if (auto* vs = std::get_if<std::string_view>(&v); vs != nullptr && is_match(*vs, filter)) {
            match = true;
        }
    });

    return match ? match_action : no_match_action;
}

filter_result
is_filter_match_id(std::string_view name, std::string_view tags, std::string_view filter) noexcept {
    if (filter.starts_with('[') || filter.starts_with("~[")) {
        return is_filter_match_tags(tags, filter);
    } else {
        return is_filter_match_name(name, filter);
    }
}
} // namespace snitch

namespace snitch::impl {
namespace {
void print_location(
    const registry&           r,
    const test_id&            id,
    const section_info&       sections,
    const capture_info&       captures,
    const assertion_location& location) noexcept {

    r.print("running test case \"", make_colored(id.name, r.with_color, color::highlight1), "\"\n");

    for (auto& section : sections) {
        r.print(
            "          in section \"", make_colored(section.name, r.with_color, color::highlight1),
            "\"\n");
    }

    r.print("          at ", location.file, ":", location.line, "\n");

    if (!id.type.empty()) {
        r.print(
            "          for type ", make_colored(id.type, r.with_color, color::highlight1), "\n");
    }

    for (auto& capture : captures) {
        r.print("          with ", make_colored(capture, r.with_color, color::highlight1), "\n");
    }
}

struct default_reporter_functor {
    const registry& r;

    void operator()(const snitch::event::test_run_started& e) const noexcept {
        r.print(
            make_colored("starting ", r.with_color, color::highlight2),
            make_colored(e.name, r.with_color, color::highlight1),
            make_colored(" with ", r.with_color, color::highlight2),
            make_colored("snitch v" SNITCH_FULL_VERSION "\n", r.with_color, color::highlight1));
        r.print("==========================================\n");
    }

    void operator()(const snitch::event::test_run_ended& e) const noexcept {
        r.print("==========================================\n");

        if (e.success) {
            r.print(
                make_colored("success:", r.with_color, color::pass), " all tests passed (",
                e.run_count, " test cases, ", e.assertion_count, " assertions");
        } else {
            r.print(
                make_colored("error:", r.with_color, color::fail), " some tests failed (",
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
    }

    void operator()(const snitch::event::test_case_started& e) const noexcept {
        small_string<max_test_name_length> full_name;
        make_full_name(full_name, e.id);

        r.print(
            make_colored("starting:", r.with_color, color::status), " ",
            make_colored(full_name, r.with_color, color::highlight1), " at ", e.location.file, ":",
            e.location.line);
        r.print("\n");
    }

    void operator()(const snitch::event::test_case_ended& e) const noexcept {
        small_string<max_test_name_length> full_name;
        make_full_name(full_name, e.id);

#if SNITCH_WITH_TIMINGS
        r.print(
            make_colored("finished:", r.with_color, color::status), " ",
            make_colored(full_name, r.with_color, color::highlight1), " (", e.duration, "s)");
#else
        r.print(
            make_colored("finished:", r.with_color, color::status), " ",
            make_colored(full_name, r.with_color, color::highlight1));
#endif
        r.print("\n");
    }

    void operator()(const snitch::event::test_case_skipped& e) const noexcept {
        r.print(make_colored("skipped: ", r.with_color, color::skipped));
        print_location(r, e.id, e.sections, e.captures, e.location);
        r.print("          ", make_colored(e.message, r.with_color, color::highlight2));
        r.print("\n");
    }

    void operator()(const snitch::event::assertion_failed& e) const noexcept {
        if (e.expected) {
            r.print(make_colored("expected failure: ", r.with_color, color::pass));
        } else {
            r.print(make_colored("failed: ", r.with_color, color::fail));
        }
        print_location(r, e.id, e.sections, e.captures, e.location);
        r.print("          ", make_colored(e.message, r.with_color, color::highlight2));
        r.print("\n");
    }

    void operator()(const snitch::event::assertion_succeeded& e) const noexcept {
        r.print(make_colored("passed: ", r.with_color, color::pass));
        print_location(r, e.id, e.sections, e.captures, e.location);
        r.print("          ", make_colored(e.message, r.with_color, color::highlight2));
        r.print("\n");
    }
};

void parse_colour_mode_option(registry& reg, std::string_view color_option) {
    if (color_option == "ansi") {
        reg.with_color = true;
    } else if (color_option == "none") {
        reg.with_color = false;
    } else if (color_option == "default") {
        // Nothing to do.
    } else {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", reg.with_color, color::warning),
            " unknown color directive; please use one of ansi|default|none\n");
    }
}

void parse_color_option(registry& reg, std::string_view color_option) {
    if (color_option == "always") {
        reg.with_color = true;
    } else if (color_option == "never") {
        reg.with_color = false;
    } else {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", reg.with_color, color::warning),
            " unknown color directive; please use one of always|never\n");
    }
}
} // namespace

void default_reporter(const registry& r, const event::data& event) noexcept {
    std::visit(default_reporter_functor{r}, event);
}

void initialize_default_reporter(registry&) noexcept {}

bool configure_default_reporter(
    registry& r, std::string_view option, std::string_view value) noexcept {

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

void finish_default_reporter(registry&) noexcept {}
} // namespace snitch::impl

namespace snitch {
std::string_view registry::add_reporter(
    std::string_view                                 name,
    const std::optional<initialize_report_function>& initialize,
    const std::optional<configure_report_function>&  configure,
    const report_function&                           report,
    const std::optional<finish_report_function>&     finish) {

    if (registered_reporters.available() == 0u) {
        using namespace snitch::impl;
        print(
            make_colored("error:", with_color, color::fail),
            " max number of reporters reached; "
            "please increase 'SNITCH_MAX_REGISTERED_REPORTERS' (currently ",
            max_registered_reporters, ")\n.");
        assertion_failed("max number of reporters reached");
    }

    if (name.find("::") != std::string_view::npos) {
        using namespace snitch::impl;
        print(
            make_colored("error:", with_color, color::fail),
            " reporter name cannot contains '::' (trying to register ", name, ")\n.");
        assertion_failed("invalid reporter name");
    }

    registered_reporters.push_back(registered_reporter{
        name, initialize.value_or([](registry&) noexcept {}),
        configure.value_or(
            [](registry&, std::string_view, std::string_view) noexcept { return false; }),
        report, finish.value_or([](registry&) noexcept {})});

    return name;
}

const char* registry::add(const test_id& id, const source_location& location, impl::test_ptr func) {
    if (test_list.available() == 0u) {
        using namespace snitch::impl;
        print(
            make_colored("error:", with_color, color::fail),
            " max number of test cases reached; "
            "please increase 'SNITCH_MAX_TEST_CASES' (currently ",
            max_test_cases, ")\n.");
        assertion_failed("max number of test cases reached");
    }

    test_list.push_back(impl::test_case{id, location, func});

    small_string<max_test_name_length> buffer;
    if (impl::make_full_name(buffer, test_list.back().id).empty()) {
        using namespace snitch::impl;
        print(
            make_colored("error:", with_color, color::fail),
            " max length of test name reached; "
            "please increase 'SNITCH_MAX_TEST_NAME_LENGTH' (currently ",
            max_test_name_length, ")\n.");
        assertion_failed("test case name exceeds max length");
    }

    return id.name.data();
}

void registry::report_assertion(
    bool                      success,
    impl::test_state&         state,
    const assertion_location& location,
    std::string_view          message) const noexcept {
    if (state.test.state == impl::test_case_state::skipped) {
        return;
    }

    ++state.asserts;

    if (!success && !state.may_fail) {
        impl::set_state(state.test, impl::test_case_state::failed);
    }

    const auto captures_buffer = impl::make_capture_buffer(state.captures);

    if (success) {
        if (verbose >= registry::verbosity::full) {
            report_callback(
                *this, event::assertion_succeeded{
                           state.test.id, state.sections.current_section, captures_buffer.span(),
                           location, message});
        }
    } else {
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message, state.should_fail, state.may_fail});
    }
}

void registry::report_assertion(
    bool                      success,
    impl::test_state&         state,
    const assertion_location& location,
    std::string_view          message1,
    std::string_view          message2) const noexcept {
    if (state.test.state == impl::test_case_state::skipped) {
        return;
    }

    ++state.asserts;

    if (!success && !state.may_fail) {
        impl::set_state(state.test, impl::test_case_state::failed);
    }

    const auto captures_buffer = impl::make_capture_buffer(state.captures);

    small_string<max_message_length> message;
    append_or_truncate(message, message1, message2);

    if (success) {
        if (verbose >= registry::verbosity::full) {
            report_callback(
                *this, event::assertion_succeeded{
                           state.test.id, state.sections.current_section, captures_buffer.span(),
                           location, message});
        }
    } else {
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message, state.should_fail, state.may_fail});
    }
}

void registry::report_assertion(
    bool                      success,
    impl::test_state&         state,
    const assertion_location& location,
    const impl::expression&   exp) const noexcept {
    if (state.test.state == impl::test_case_state::skipped) {
        return;
    }

    ++state.asserts;

    if (!success && !state.may_fail) {
        impl::set_state(state.test, impl::test_case_state::failed);
    }

    const auto captures_buffer = impl::make_capture_buffer(state.captures);

    small_string<max_message_length> message_buffer;
    std::string_view                 message;
    if (!exp.actual.empty()) {
        append_or_truncate(message_buffer, exp.expected, ", got ", exp.actual);
        message = message_buffer.str();
    } else {
        message = exp.expected;
    }

    if (success) {
        if (verbose >= registry::verbosity::full) {
            report_callback(
                *this, event::assertion_succeeded{
                           state.test.id, state.sections.current_section, captures_buffer.span(),
                           location, message});
        }
    } else {
        report_callback(
            *this, event::assertion_failed{
                       state.test.id, state.sections.current_section, captures_buffer.span(),
                       location, message, state.should_fail, state.may_fail});
    }
}

void registry::report_skipped(
    impl::test_state&         state,
    const assertion_location& location,
    std::string_view          message) const noexcept {
    impl::set_state(state.test, impl::test_case_state::skipped);

    const auto captures_buffer = impl::make_capture_buffer(state.captures);

    report_callback(
        *this, event::test_case_skipped{
                   state.test.id, state.sections.current_section, captures_buffer.span(), location,
                   message});
}

impl::test_state registry::run(impl::test_case& test) noexcept {
    if (verbose >= registry::verbosity::high) {
        report_callback(*this, event::test_case_started{test.id, test.location});
    }

    test.state = impl::test_case_state::success;

    // Fetch special tags for this test case.
    bool may_fail    = false;
    bool should_fail = false;
    impl::for_each_tag(test.id.tags, [&](const impl::tags::parsed_tag& v) {
        if (std::holds_alternative<impl::tags::may_fail>(v)) {
            may_fail = true;
        } else if (std::holds_alternative<impl::tags::should_fail>(v)) {
            should_fail = true;
        }
    });

    impl::test_state state{
        .reg = *this, .test = test, .may_fail = may_fail, .should_fail = should_fail};

    // Store previously running test, to restore it later.
    // This should always be a null pointer, except when testing snitch itself.
    impl::test_state* previous_run = impl::try_get_current_test();
    impl::set_current_test(&state);

#if SNITCH_WITH_TIMINGS
    using clock     = std::chrono::steady_clock;
    auto time_start = clock::now();
#endif

    do {
        // Reset section state.
        state.sections.leaf_executed = false;
        for (std::size_t i = 0; i < state.sections.levels.size(); ++i) {
            state.sections.levels[i].current_section_id = 0;
        }

        // Run the test case.
#if SNITCH_WITH_EXCEPTIONS
        try {
            test.func();
        } catch (const impl::abort_exception&) {
            // Test aborted, assume its state was already set accordingly.
        } catch (const std::exception& e) {
            report_assertion(
                false, state, test.location,
                "unhandled std::exception caught; message: ", e.what());
            --state.asserts; // this doesn't count as a user assert, undo the increment
        } catch (...) {
            report_assertion(false, state, test.location, "unhandled unknown exception caught");
            --state.asserts; // this doesn't count as a user assert, undo the increment
        }
#else
        test.func();
#endif

        if (state.sections.levels.size() == 1) {
            // This test case contained sections; check if there are any more left to evaluate.
            auto& child = state.sections.levels[0];
            if (child.previous_section_id == child.max_section_id) {
                // No more; clear the section state.
                state.sections.levels.clear();
                state.sections.current_section.clear();
            }
        }
    } while (!state.sections.levels.empty() && state.test.state != impl::test_case_state::skipped);

    if (state.should_fail) {
        if (state.test.state == impl::test_case_state::success) {
            state.should_fail = false;
            report_assertion(
                false, state, {"<snitch internal>", 0}, "expected test to fail, but it passed");
            --state.asserts; // this doesn't count as a user assert, undo the increment
            state.should_fail = true;
        } else if (state.test.state == impl::test_case_state::failed) {
            state.test.state = impl::test_case_state::success;
        }
    }

#if SNITCH_WITH_TIMINGS
    auto time_end  = clock::now();
    state.duration = std::chrono::duration<float>(time_end - time_start).count();
#endif

    if (verbose >= registry::verbosity::high) {
#if SNITCH_WITH_TIMINGS
        report_callback(
            *this, event::test_case_ended{
                       .id              = test.id,
                       .location        = test.location,
                       .assertion_count = state.asserts,
                       .state           = impl::convert_to_public_state(state.test.state),
                       .duration        = state.duration});
#else
        report_callback(
            *this, event::test_case_ended{
                       .id              = test.id,
                       .location        = test.location,
                       .assertion_count = state.asserts,
                       .state           = impl::convert_to_public_state(state.test.state)});
#endif
    }

    impl::set_current_test(previous_run);

    return state;
}

bool registry::run_selected_tests(
    std::string_view                                     run_name,
    const small_function<bool(const test_id&) noexcept>& predicate) noexcept {
    if (verbose >= registry::verbosity::normal) {
        report_callback(*this, event::test_run_started{run_name});
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

    for (impl::test_case& t : *this) {
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

    if (verbose >= registry::verbosity::normal) {
#if SNITCH_WITH_TIMINGS
        report_callback(
            *this, event::test_run_ended{
                       .name            = run_name,
                       .run_count       = run_count,
                       .fail_count      = fail_count,
                       .skip_count      = skip_count,
                       .assertion_count = assertion_count,
                       .duration        = duration,
                       .success         = success,
                   });
#else
        report_callback(
            *this, event::test_run_ended{
                       .name            = run_name,
                       .run_count       = run_count,
                       .fail_count      = fail_count,
                       .skip_count      = skip_count,
                       .assertion_count = assertion_count,
                       .success         = success});
#endif
    }

    return success;
}

bool registry::run_tests(std::string_view run_name) noexcept {
    const auto filter = [](const test_id& id) {
        bool selected = true;
        impl::for_each_tag(id.tags, [&](const impl::tags::parsed_tag& s) {
            if (std::holds_alternative<impl::tags::ignored>(s)) {
                selected = false;
            }
        });

        return selected;
    };

    return run_selected_tests(run_name, filter);
}

bool registry::run_tests(const cli::input& args) noexcept {
    if (get_option(args, "--help")) {
        cli::print("\n");
        cli::print_help(args.executable);
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

    if (get_option(args, "--list-reporters")) {
        list_all_reporters();
        return true;
    }

    bool success = false;
    if (get_positional_argument(args, "test regex").has_value()) {
        small_string<max_test_name_length> buffer;

        const auto filter = [&](const test_id& id) noexcept {
            std::optional<bool> selected;

            const auto callback = [&](std::string_view filter) noexcept {
                switch (is_filter_match_id(impl::make_full_name(buffer, id), id.tags, filter)) {
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

        success = run_selected_tests(args.executable, filter);
    } else {
        success = run_tests(args.executable);
    }

    finish_callback(*this);

    return success;
}

namespace impl {
void parse_reporter(
    registry&                                                   r,
    small_vector_span<const std::optional<registered_reporter>> reporters,
    std::string_view                                            arg) noexcept {

    if (arg.empty() || arg[0] == ':') {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", r.with_color, color::warning), " invalid reporter '", arg,
            "', using default\n");
        return;
    }

    // Isolate reporter name and options
    std::string_view reporter_name = arg;
    std::string_view options;
    if (auto option_pos = reporter_name.find("::"); option_pos != std::string_view::npos) {
        options       = reporter_name.substr(option_pos);
        reporter_name = reporter_name.substr(0, option_pos);
    }

    // Locate reporter
    auto iter = std::find_if(reporters.begin(), reporters.end(), [&](const auto& reporter) {
        return reporter->name == reporter_name;
    });

    if (iter == reporters.end()) {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", r.with_color, color::warning), " unknown reporter '",
            reporter_name, "', using default\n");
        cli::print(make_colored("note:", r.with_color, color::status), " available reporters:\n");
        for (const auto& reporter : reporters) {
            cli::print(
                make_colored("note:", r.with_color, color::status), "  ", reporter->name, "\n");
        }
        return;
    }

    // Initialise reporter now, so we can configure it.
    (*iter)->initialize(r);

    // Configure reporter
    auto option_pos = options.find("::");
    while (option_pos != std::string_view::npos) {
        option_pos = options.find("::", 2);
        if (option_pos != std::string_view::npos) {
            options = options.substr(option_pos);
        }

        std::string_view option = options.substr(2, option_pos);

        auto equal_pos = option.find("=");
        if (equal_pos == std::string_view::npos || equal_pos == 0) {
            using namespace snitch::impl;
            cli::print(
                make_colored("warning:", r.with_color, color::warning),
                " badly formatted reporter option '", option, "'; expected 'key=value'\n");
            continue;
        }

        std::string_view option_name  = option.substr(0, equal_pos);
        std::string_view option_value = option.substr(equal_pos + 1);

        if (!(*iter)->configure(r, option_name, option_value)) {
            using namespace snitch::impl;
            cli::print(
                make_colored("warning:", r.with_color, color::warning),
                " unknown reporter option '", option_name, "'\n");
        }
    }

    // Register reporter callbacks
    r.report_callback = (*iter)->callback;
    r.finish_callback = (*iter)->finish;
}
} // namespace impl

void registry::configure(const cli::input& args) noexcept {
    if (auto opt = get_option(args, "--colour-mode")) {
        impl::parse_colour_mode_option(*this, *opt->value);
    }

    if (auto opt = get_option(args, "--color")) {
        impl::parse_color_option(*this, *opt->value);
    }

    if (auto opt = get_option(args, "--verbosity")) {
        if (*opt->value == "quiet") {
            verbose = snitch::registry::verbosity::quiet;
        } else if (*opt->value == "normal") {
            verbose = snitch::registry::verbosity::normal;
        } else if (*opt->value == "high") {
            verbose = snitch::registry::verbosity::high;
        } else if (*opt->value == "full") {
            verbose = snitch::registry::verbosity::full;
        } else {
            using namespace snitch::impl;
            cli::print(
                make_colored("warning:", with_color, color::warning),
                " unknown verbosity level; please use one of quiet|normal|high|full\n");
        }
    }

    if (auto opt = get_option(args, "--reporter")) {
        impl::parse_reporter(*this, registered_reporters, *opt->value);
    }
}

void registry::list_all_tags() const {
    small_vector<std::string_view, max_unique_tags> tags;
    for (const auto& t : test_list) {
        impl::for_each_tag(t.id.tags, [&](const impl::tags::parsed_tag& v) {
            if (auto* vs = std::get_if<std::string_view>(&v); vs != nullptr) {
                if (std::find(tags.begin(), tags.end(), *vs) == tags.end()) {
                    if (tags.size() == tags.capacity()) {
                        using namespace snitch::impl;
                        cli::print(
                            make_colored("error:", with_color, color::fail),
                            " max number of tags reached; "
                            "please increase 'SNITCH_MAX_UNIQUE_TAGS' (currently ",
                            max_unique_tags, ")\n.");
                        assertion_failed("max number of unique tags reached");
                    }

                    tags.push_back(*vs);
                }
            }
        });
    }

    std::sort(tags.begin(), tags.end());

    for (const auto& t : tags) {
        cli::print("[", t, "]\n");
    }
}

void registry::list_all_tests() const noexcept {
    impl::list_tests(*this, [](const impl::test_case&) { return true; });
}

void registry::list_tests_with_tag(std::string_view tag) const noexcept {
    impl::list_tests(*this, [&](const impl::test_case& t) {
        const auto result = is_filter_match_tags(t.id.tags, tag);
        return result == filter_result::included || result == filter_result::not_excluded;
    });
}

void registry::list_all_reporters() const noexcept {
    for (const auto& r : registered_reporters) {
        cli::print(r->name, "\n");
    }
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

constinit registry tests = []() {
    registry r;
    r.with_color = SNITCH_DEFAULT_WITH_COLOR == 1;
    return r;
}();
} // namespace snitch
