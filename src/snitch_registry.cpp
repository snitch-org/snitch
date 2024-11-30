#include "snitch/snitch_registry.hpp"

#include "snitch/snitch_time.hpp"

#include <algorithm> // for std::sort
#include <optional> // for std::optional

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
struct hidden {};
struct may_fail {};
struct should_fail {};

using parsed_tag = std::variant<std::string_view, hidden, may_fail, should_fail>;
} // namespace tags

// Requires: s contains a well-formed list of tags, each of length <= max_tag_length.
template<typename F>
void for_each_tag(std::string_view s, F&& callback) {
    small_string<max_tag_length> buffer;

    for_each_raw_tag(s, [&](std::string_view t) {
        // Look for "hidden" tags, which is either "[.]"
        // or a tag starting with ".", like "[.integration]".
        if (t == "[.]"sv) {
            // This is a pure "hidden" tag, add this to the list of special tags.
            callback(tags::parsed_tag{tags::hidden{}});
        } else if (t.starts_with("[."sv)) {
            // This is a combined "hidden" + normal tag, add the "hidden" to the list of special
            // tags, and continue with the normal tag.
            callback(tags::parsed_tag{tags::hidden{}});
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

// Requires: s contains a well-formed list of tags, each of length <= max_tag_length.
bool has_hidden_tag(std::string_view tags) {
    bool hidden = false;
    impl::for_each_tag(tags, [&](const impl::tags::parsed_tag& s) {
        if (std::holds_alternative<impl::tags::hidden>(s)) {
            hidden = true;
        }
    });

    return hidden;
}

template<typename F>
void list_tests(const registry& r, F&& predicate) noexcept {
    r.report_callback(r, event::list_test_run_started{});

    for (const test_case& t : r.test_cases()) {
        if (!predicate(t.id)) {
            continue;
        }

        r.report_callback(r, event::test_case_listed{t.id, t.location});
    }

    r.report_callback(r, event::list_test_run_ended{});
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
    case impl::test_case_state::allowed_fail: return snitch::test_case_state::allowed_fail;
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
filter_result filter_result_and(filter_result first, filter_result second) noexcept {
    // AND favours exclusion over inclusion, then explicit over implicit.
    if (!first.included && second.included) {
        return first;
    } else if (first.included && !second.included) {
        return second;
    } else if (!first.implicit) {
        return first;
    } else {
        return second;
    }
}

filter_result filter_result_or(filter_result first, filter_result second) noexcept {
    // OR favours inclusion over exclusion, then explicit over implicit.
    if (first.included && !second.included) {
        return first;
    } else if (!first.included && second.included) {
        return second;
    } else if (!first.implicit) {
        return first;
    } else {
        return second;
    }
}

filter_result is_filter_match_name(std::string_view name, std::string_view filter) noexcept {
    filter_result match_action    = {.included = true, .implicit = false};
    filter_result no_match_action = {.included = false, .implicit = true};
    if (filter.starts_with('~')) {
        filter = filter.substr(1);
        std::swap(match_action.included, no_match_action.included);
    }

    return is_match(name, filter) ? match_action : no_match_action;
}

filter_result is_filter_match_tags_single(std::string_view tags, std::string_view filter) noexcept {
    filter_result match_action    = {.included = true, .implicit = false};
    filter_result no_match_action = {.included = false, .implicit = true};
    if (filter.starts_with('~')) {
        filter = filter.substr(1);
        std::swap(match_action.included, no_match_action.included);
    }

    bool match = false;
    impl::for_each_tag(tags, [&](const impl::tags::parsed_tag& v) {
        if (auto* vs = std::get_if<std::string_view>(&v); vs != nullptr) {
            if (is_match(*vs, filter)) {
                match = true;
            }
        }
    });

    return match ? match_action : no_match_action;
}

filter_result is_filter_match_tags(std::string_view tags, std::string_view filter) noexcept {
    // Start with no result.
    std::optional<filter_result> result;

    // Evaluate each tag filter (one after the other, e.g. "[tag1][tag2]").
    std::size_t end_pos = 0;
    do {
        end_pos = find_first_not_escaped(filter, ']');
        if (end_pos != std::string_view::npos) {
            ++end_pos;
        }

        const filter_result sub_result =
            is_filter_match_tags_single(tags, filter.substr(0, end_pos));

        if (!result.has_value()) {
            // The first filter initialises the result.
            result = sub_result;
        } else {
            // Subsequent filters are combined with the current result using AND.
            result = filter_result_and(*result, sub_result);
        }

        if (!result->included) {
            // Optimisation; we can short-circuit at the first exclusion.
            // It does not matter if it is implicit or explicit, they are treated the same.
            break;
        }

        if (end_pos != std::string_view::npos) {
            filter.remove_prefix(end_pos);
        }
    } while (end_pos != std::string_view::npos && !filter.empty());

    return *result;
}

filter_result is_filter_match_id_single(
    std::string_view name, std::string_view tags, std::string_view filter) noexcept {

    if (filter.starts_with('[') || filter.starts_with("~[")) {
        return is_filter_match_tags(tags, filter);
    } else {
        return is_filter_match_name(name, filter);
    }
}

filter_result
is_filter_match_id(std::string_view name, std::string_view tags, std::string_view filter) noexcept {
    // Start with no result.
    std::optional<filter_result> result;

    // Evaluate each filter (comma-separated).
    std::size_t comma_pos = 0;
    do {
        comma_pos = find_first_not_escaped(filter, ',');

        const filter_result sub_result =
            is_filter_match_id_single(name, tags, filter.substr(0, comma_pos));

        if (!result.has_value()) {
            // The first filter initialises the result.
            result = sub_result;
        } else {
            // Subsequent filters are combined with the current result using OR.
            result = filter_result_or(*result, sub_result);
        }

        if (result->included && !result->implicit) {
            // Optimisation; we can short-circuit at the first explicit inclusion.
            // We can't short-circuit on implicit inclusion, because there could still be an
            // explicit inclusion coming, and we want to know (for hidden tests).
            break;
        }

        if (comma_pos != std::string_view::npos) {
            filter.remove_prefix(comma_pos + 1);
        }
    } while (comma_pos != std::string_view::npos);

    return *result;
}
} // namespace snitch

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
            max_registered_reporters, ").\n");
        assertion_failed("max number of reporters reached");
    }

    if (name.find("::") != std::string_view::npos) {
        using namespace snitch::impl;
        print(
            make_colored("error:", with_color, color::fail),
            " reporter name cannot contains '::' (trying to register '", name, "')\n.");
        assertion_failed("invalid reporter name");
    }

    registered_reporters.push_back(registered_reporter{
        name, initialize.value_or([](registry&) noexcept {}),
        configure.value_or(
            [](registry&, std::string_view, std::string_view) noexcept { return false; }),
        report, finish.value_or([](registry&) noexcept {})});

    return name;
}

void registry::destroy_reporter(registry&) noexcept {
    reporter_storage.reset();
}

void registry::report_default(const registry&, const event::data& e) noexcept {
    using default_reporter = reporter::console::reporter;

    if (reporter_storage.type() != type_id<default_reporter>()) {
        reporter_storage.emplace<default_reporter>(*this);
    }

    reporter_storage.get<default_reporter>().report(*this, e);
}

const char*
registry::add_impl(const test_id& id, const source_location& location, impl::test_ptr func) {
    if (test_list.available() == 0u) {
        using namespace snitch::impl;
        print(
            make_colored("error:", with_color, color::fail),
            " max number of test cases reached; "
            "please increase 'SNITCH_MAX_TEST_CASES' (currently ",
            max_test_cases, ").\n");
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

const char*
registry::add(const impl::name_and_tags& id, const source_location& location, impl::test_ptr func) {
    return add_impl({.name = id.name, .tags = id.tags}, location, func);
}

const char* registry::add_fixture(
    const impl::fixture_name_and_tags& id, const source_location& location, impl::test_ptr func) {

    return add_impl({.name = id.name, .tags = id.tags, .fixture = id.fixture}, location, func);
}

namespace {
void register_assertion(bool success, impl::test_state& state) {
    if (!success) {
        if (state.may_fail || state.should_fail) {
            ++state.asserts;
            ++state.allowed_failures;

            for (auto& section : state.info.sections.current_section) {
                ++section.assertion_count;
                ++section.allowed_assertion_failure_count;
            }

#if SNITCH_WITH_EXCEPTIONS
            if (state.held_info.has_value()) {
                for (auto& section : state.held_info.value().sections.current_section) {
                    ++section.assertion_count;
                    ++section.allowed_assertion_failure_count;
                }
            }
#endif

            impl::set_state(state.test, impl::test_case_state::allowed_fail);
        } else {
            ++state.asserts;
            ++state.failures;

            for (auto& section : state.info.sections.current_section) {
                ++section.assertion_count;
                ++section.assertion_failure_count;
            }

#if SNITCH_WITH_EXCEPTIONS
            if (state.held_info.has_value()) {
                for (auto& section : state.held_info.value().sections.current_section) {
                    ++section.assertion_count;
                    ++section.assertion_failure_count;
                }
            }
#endif

            impl::set_state(state.test, impl::test_case_state::failed);
        }
    } else {
        ++state.asserts;

        for (auto& section : state.info.sections.current_section) {
            ++section.assertion_count;
        }

#if SNITCH_WITH_EXCEPTIONS
        if (state.held_info.has_value()) {
            for (auto& section : state.held_info.value().sections.current_section) {
                ++section.assertion_count;
            }
        }
#endif
    }
}

void report_assertion_impl(
    const registry& r, bool success, impl::test_state& state, const assertion_data& data) noexcept {

    if (state.test.state == impl::test_case_state::skipped) {
        return;
    }

    register_assertion(success, state);

#if SNITCH_WITH_EXCEPTIONS
    const bool use_held_info = (state.unhandled_exception || std::uncaught_exceptions() > 0) &&
                               state.held_info.has_value();

    const auto captures_buffer = impl::make_capture_buffer(
        use_held_info ? state.held_info.value().captures : state.info.captures);

    const auto& current_section = use_held_info ? state.held_info.value().sections.current_section
                                                : state.info.sections.current_section;

    const auto& last_location =
        use_held_info ? state.held_info.value().locations.back() : state.info.locations.back();

    const auto location =
        state.in_check
            ? assertion_location{last_location.file, last_location.line, location_type::exact}
            : last_location;
#else
    const auto  captures_buffer = impl::make_capture_buffer(state.info.captures);
    const auto& current_section = state.info.sections.current_section;
    const auto& last_location   = state.info.locations.back();
    const auto  location =
        assertion_location{last_location.file, last_location.line, location_type::exact};
#endif

    if (success) {
        if (r.verbose >= registry::verbosity::full) {
            r.report_callback(
                r, event::assertion_succeeded{
                       state.test.id, current_section, captures_buffer.span(), location, data});
        }
    } else {
        r.report_callback(
            r, event::assertion_failed{
                   state.test.id, current_section, captures_buffer.span(), location, data,
                   state.should_fail, state.may_fail});
    }
}
} // namespace

void registry::report_assertion(bool success, std::string_view message) noexcept {
    impl::test_state& state = impl::get_current_test();
    report_assertion_impl(state.reg, success, state, message);
}

void registry::report_assertion(
    bool success, std::string_view message1, std::string_view message2) noexcept {

    impl::test_state& state = impl::get_current_test();
    if (state.test.state == impl::test_case_state::skipped) {
        return;
    }

    small_string<max_message_length> message;
    append_or_truncate(message, message1, message2);
    report_assertion_impl(state.reg, success, state, message);
}

void registry::report_assertion(bool success, const impl::expression& exp) noexcept {
    impl::test_state& state = impl::get_current_test();
    if (state.test.state == impl::test_case_state::skipped) {
        return;
    }

    report_assertion_impl(
        state.reg, success, state, expression_info{exp.type, exp.expected, exp.actual});
}

void registry::report_skipped(std::string_view message) noexcept {
    impl::test_state& state = impl::get_current_test();
    impl::set_state(state.test, impl::test_case_state::skipped);

    const auto  captures_buffer = impl::make_capture_buffer(state.info.captures);
    const auto& location        = state.info.locations.back();

    state.reg.report_callback(
        state.reg, event::test_case_skipped{
                       state.test.id,
                       state.info.sections.current_section,
                       captures_buffer.span(),
                       {location.file, location.line, location_type::exact},
                       message});
}

void registry::report_section_started(const section& sec) noexcept {
    const impl::test_state& state = impl::get_current_test();

    if (state.reg.verbose < registry::verbosity::high) {
        return;
    }

    state.reg.report_callback(state.reg, event::section_started{sec.id, sec.location});
}

void registry::report_section_ended(const section& sec) noexcept {
    const impl::test_state& state = impl::get_current_test();

    if (state.reg.verbose < registry::verbosity::high) {
        return;
    }

    const bool skipped = state.test.state == impl::test_case_state::skipped;

#if SNITCH_WITH_TIMINGS
    const auto duration = get_duration_in_seconds(sec.start_time, get_current_time());
    state.reg.report_callback(
        state.reg, event::section_ended{
                       .id                              = sec.id,
                       .location                        = sec.location,
                       .skipped                         = skipped,
                       .assertion_count                 = sec.assertion_count,
                       .assertion_failure_count         = sec.assertion_failure_count,
                       .allowed_assertion_failure_count = sec.allowed_assertion_failure_count,
                       .duration                        = duration});
#else
    state.reg.report_callback(
        state.reg, event::section_ended{
                       .id                              = sec.id,
                       .location                        = sec.location,
                       .skipped                         = skipped,
                       .assertion_count                 = sec.assertion_count,
                       .assertion_failure_count         = sec.assertion_failure_count,
                       .allowed_assertion_failure_count = sec.allowed_assertion_failure_count});
#endif
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

    state.info.locations.push_back(
        {test.location.file, test.location.line, location_type::test_case_scope});

    // Store previously running test, to restore it later.
    // This should always be a null pointer, except when testing snitch itself.
    impl::test_state* previous_run = impl::try_get_current_test();
    impl::set_current_test(&state);

#if SNITCH_WITH_TIMINGS
    const auto time_start = get_current_time();
#endif

#if SNITCH_WITH_EXCEPTIONS
    try {
#endif

        do {
            // Reset section state.
            state.info.sections.leaf_executed = false;
            for (std::size_t i = 0; i < state.info.sections.levels.size(); ++i) {
                state.info.sections.levels[i].current_section_id = 0;
            }

            // Run the test case.
            test.func();

            if (state.info.sections.levels.size() == 1) {
                // This test case contained sections; check if there are any more left to evaluate.
                auto& child = state.info.sections.levels[0];
                if (child.previous_section_id == child.max_section_id) {
                    // No more; clear the section state.
                    state.info.sections.levels.clear();
                    state.info.sections.current_section.clear();
                }
            }
        } while (!state.info.sections.levels.empty() &&
                 state.test.state != impl::test_case_state::skipped);

#if SNITCH_WITH_EXCEPTIONS
        state.in_check = true;
        report_assertion(true, "no exception caught");
        state.in_check = false;
    } catch (const impl::abort_exception&) {
        // Test aborted, assume its state was already set accordingly.
        state.unhandled_exception = true;
    } catch (const std::exception& e) {
        state.unhandled_exception = true;
        report_assertion(false, "unexpected std::exception caught; message: ", e.what());
    } catch (...) {
        state.unhandled_exception = true;
        report_assertion(false, "unexpected unknown exception caught");
    }

    if (state.unhandled_exception) {
        notify_exception_handled();
    }

    state.unhandled_exception = false;
#endif

    if (state.should_fail) {
        state.should_fail = false;
        state.in_check    = true;
        report_assertion(
            state.test.state == impl::test_case_state::allowed_fail, "expected test to fail");
        state.in_check    = false;
        state.should_fail = true;
    }

#if SNITCH_WITH_TIMINGS
    state.duration = get_duration_in_seconds(time_start, get_current_time());
#endif

    if (verbose >= registry::verbosity::high) {
#if SNITCH_WITH_TIMINGS
        report_callback(
            *this, event::test_case_ended{
                       .id                              = test.id,
                       .location                        = test.location,
                       .assertion_count                 = state.asserts,
                       .assertion_failure_count         = state.failures,
                       .allowed_assertion_failure_count = state.allowed_failures,
                       .state    = impl::convert_to_public_state(state.test.state),
                       .duration = state.duration});
#else
        report_callback(
            *this, event::test_case_ended{
                       .id                              = test.id,
                       .location                        = test.location,
                       .assertion_count                 = state.asserts,
                       .assertion_failure_count         = state.failures,
                       .allowed_assertion_failure_count = state.allowed_failures,
                       .state = impl::convert_to_public_state(state.test.state)});
#endif
    }

    impl::set_current_test(previous_run);

    return state;
}

bool registry::run_selected_tests(
    std::string_view                                   run_name,
    const filter_info&                                 filter_strings,
    const function_ref<bool(const test_id&) noexcept>& predicate) noexcept {

    if (verbose >= registry::verbosity::normal) {
        report_callback(
            *this, event::test_run_started{.name = run_name, .filters = filter_strings});
    }

    bool        success                         = true;
    std::size_t run_count                       = 0;
    std::size_t fail_count                      = 0;
    std::size_t allowed_fail_count              = 0;
    std::size_t skip_count                      = 0;
    std::size_t assertion_count                 = 0;
    std::size_t assertion_failure_count         = 0;
    std::size_t allowed_assertion_failure_count = 0;

#if SNITCH_WITH_TIMINGS
    const auto time_start = get_current_time();
#endif

    for (impl::test_case& t : this->test_cases()) {
        if (!predicate(t.id)) {
            continue;
        }

        auto state = run(t);

        ++run_count;
        assertion_count += state.asserts;
        assertion_failure_count += state.failures;
        allowed_assertion_failure_count += state.allowed_failures;

        switch (t.state) {
        case impl::test_case_state::success: {
            // Nothing to do
            break;
        }
        case impl::test_case_state::allowed_fail: {
            ++allowed_fail_count;
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
    const float duration = get_duration_in_seconds(time_start, get_current_time());
#endif

    if (verbose >= registry::verbosity::normal) {
#if SNITCH_WITH_TIMINGS
        report_callback(
            *this, event::test_run_ended{
                       .name                            = run_name,
                       .filters                         = filter_strings,
                       .run_count                       = run_count,
                       .fail_count                      = fail_count,
                       .allowed_fail_count              = allowed_fail_count,
                       .skip_count                      = skip_count,
                       .assertion_count                 = assertion_count,
                       .assertion_failure_count         = assertion_failure_count,
                       .allowed_assertion_failure_count = allowed_assertion_failure_count,
                       .duration                        = duration,
                       .success                         = success,
                   });
#else
        report_callback(
            *this, event::test_run_ended{
                       .name                            = run_name,
                       .filters                         = filter_strings,
                       .run_count                       = run_count,
                       .fail_count                      = fail_count,
                       .allowed_fail_count              = allowed_fail_count,
                       .skip_count                      = skip_count,
                       .assertion_count                 = assertion_count,
                       .assertion_failure_count         = assertion_failure_count,
                       .allowed_assertion_failure_count = allowed_assertion_failure_count,
                       .success                         = success});
#endif
    }

    return success;
}

bool registry::run_tests(std::string_view run_name) noexcept {
    // The default run simply filters out the hidden tests.
    const auto filter = [](const test_id& id) { return !impl::has_hidden_tag(id.tags); };

    const small_vector<std::string_view, 1> filter_strings = {};
    return run_selected_tests(run_name, filter_strings, filter);
}

namespace {
bool run_tests_impl(registry& r, const cli::input& args) noexcept {
    if (get_option(args, "--help")) {
        cli::print_help(args.executable, {.with_color = r.with_color});
        return true;
    }

    if (auto opt = get_option(args, "--list-tests-with-tag")) {
        r.list_tests_with_tag(*opt->value);
        return true;
    }

    if (get_option(args, "--list-tags")) {
        r.list_all_tags();
        return true;
    }

    if (get_option(args, "--list-reporters")) {
        r.list_all_reporters();
        return true;
    }

    if (get_positional_argument(args, "test regex").has_value()) {
        // Gather all filters in a local array (for faster iteration and for event reporting).
        small_vector<std::string_view, max_command_line_args> filter_strings;
        const auto add_filter_string = [&](std::string_view filter) noexcept {
            filter_strings.push_back(filter);
        };
        for_each_positional_argument(args, "test regex", add_filter_string);

        // This buffer will be reused to evaluate the full name of each test.
        small_string<max_test_name_length> buffer;

        const auto filter = [&](const test_id& id) noexcept {
            // Start with no result.
            std::optional<filter_result> result;

            // Evaluate each filter (provided as separate command-line argument).
            for (const auto& filter : filter_strings) {
                const filter_result sub_result =
                    is_filter_match_id(impl::make_full_name(buffer, id), id.tags, filter);

                if (!result.has_value()) {
                    // The first filter initialises the result.
                    result = sub_result;
                } else {
                    // Subsequent filters are combined with the current result using AND.
                    result = filter_result_and(*result, sub_result);
                }

                if (!result->included) {
                    // Optimisation; we can short-circuit at the first exclusion.
                    // It does not matter if it is implicit or explicit, they are treated the same.
                    break;
                }
            }

            if (result->included) {
                if (!result->implicit) {
                    // Explicit inclusion always selects the test.
                    return true;
                } else {
                    // Implicit inclusion only selects non-hidden tests.
                    return !impl::has_hidden_tag(id.tags);
                }
            } else {
                // Exclusion always discards the test, regardless if it is explicit or implicit.
                return false;
            }
        };

        if (get_option(args, "--list-tests")) {
            impl::list_tests(r, filter);
            return true;
        } else {
            return r.run_selected_tests(args.executable, filter_strings, filter);
        }
    } else {
        if (get_option(args, "--list-tests")) {
            r.list_all_tests();
            return true;
        } else {
            return r.run_tests(args.executable);
        }
    }
}
} // namespace

bool registry::run_tests(const cli::input& args) noexcept {
    // Run tests.
    const bool success = run_tests_impl(*this, args);

    // Tell the current reporter we are done.
    finish_callback(*this);

    // Close the output file, if any.
    file_writer.reset();

    return success;
}

namespace impl {
void parse_reporter(
    registry&                                    r,
    small_vector_span<const registered_reporter> reporters,
    std::string_view                             arg) noexcept {

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
        return reporter.name == reporter_name;
    });

    if (iter == reporters.end()) {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", r.with_color, color::warning), " unknown reporter '",
            reporter_name, "', using default\n");
        cli::print(make_colored("note:", r.with_color, color::status), " available reporters:\n");
        for (const auto& reporter : reporters) {
            cli::print(
                make_colored("note:", r.with_color, color::status), "  ", reporter.name, "\n");
        }
        return;
    }

    // Initialise reporter now, so we can configure it.
    iter->initialize(r);

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

        if (!iter->configure(r, option_name, option_value)) {
            using namespace snitch::impl;
            cli::print(
                make_colored("warning:", r.with_color, color::warning),
                " unknown reporter option '", option_name, "'\n");
        }
    }

    // Register reporter callbacks
    r.report_callback = iter->callback;
    r.finish_callback = iter->finish;
}

bool parse_colour_mode_option(registry& reg, std::string_view color_option) noexcept {
    if (color_option == "ansi") {
        reg.with_color = true;
        return true;
    } else if (color_option == "none") {
        reg.with_color = false;
        return true;
    } else if (color_option == "default") {
        // Nothing to do.
        return false;
    } else {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", reg.with_color, color::warning),
            " unknown color directive; please use one of ansi|default|none\n");
        return false;
    }
}

bool parse_color_option(registry& reg, std::string_view color_option) noexcept {
    if (color_option == "always") {
        reg.with_color = true;
        return true;
    } else if (color_option == "never") {
        reg.with_color = false;
        return true;
    } else if (color_option == "default") {
        // Nothing to do.
        return false;
    } else {
        using namespace snitch::impl;
        cli::print(
            make_colored("warning:", reg.with_color, color::warning),
            " unknown color directive; please use one of always|default|never\n");
        return false;
    }
}
} // namespace impl

void registry::configure(const cli::input& args) {
    bool color_override = false;
    if (auto opt = get_option(args, "--colour-mode")) {
        color_override = impl::parse_colour_mode_option(*this, *opt->value);
    }

    if (auto opt = get_option(args, "--color")) {
        color_override = impl::parse_color_option(*this, *opt->value) || color_override;
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

    if (auto opt = get_option(args, "--out")) {
        file_writer = impl::file_writer{*opt->value};

        if (!color_override) {
            with_color = false;
        }

        print_callback = {*file_writer, snitch::constant<&impl::file_writer::write>{}};
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
                            max_unique_tags, ").\n");
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
    impl::list_tests(*this, [](const test_id&) { return true; });
}

void registry::list_tests_with_tag(std::string_view tag) const noexcept {
    impl::list_tests(*this, [&](const test_id& id) {
        const auto result = is_filter_match_tags(id.tags, tag);
        return result.included;
    });
}

void registry::list_all_reporters() const noexcept {
    for (const auto& r : registered_reporters) {
        cli::print(r.name, "\n");
    }
}

small_vector_span<impl::test_case> registry::test_cases() noexcept {
    return test_list;
}

small_vector_span<const impl::test_case> registry::test_cases() const noexcept {
    return test_list;
}

small_vector_span<registered_reporter> registry::reporters() noexcept {
    return registered_reporters;
}

small_vector_span<const registered_reporter> registry::reporters() const noexcept {
    return registered_reporters;
}

#if SNITCH_ENABLE
constinit registry tests;
#endif // SNITCH_ENABLE
} // namespace snitch
