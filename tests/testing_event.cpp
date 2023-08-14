// clang-format off
#include "testing.hpp"
#include "testing_event.hpp"
// clang-format on

#include <algorithm>

namespace {
template<typename T>
void copy_test_run_id(event_deep_copy& c, const T& e) {
    append_or_truncate(c.test_run_name, e.name);
}

template<typename T>
void copy_test_case_id(event_deep_copy& c, const T& e) {
    append_or_truncate(c.test_id_name, e.id.name);
    append_or_truncate(c.test_id_tags, e.id.tags);
    append_or_truncate(c.test_id_type, e.id.type);
}

template<typename T>
void copy_full_location(event_deep_copy& c, const T& e) {
    append_or_truncate(c.location_file, e.location.file);
    c.location_line = e.location.line;

    if constexpr (std::is_same_v<T, snitch::event::test_case_skipped>) {
        append_or_truncate(c.expr_message, e.message);
    } else {
        std::visit(
            snitch::overload{
                [&](std::string_view message) { append_or_truncate(c.expr_message, message); },
                [&](const snitch::expression_info& exp) {
                    append_or_truncate(c.expr_type, exp.type);
                    append_or_truncate(c.expr_expected, exp.expected);
                    append_or_truncate(c.expr_actual, exp.actual);
                }},
            e.data);
    }

    for (const auto& ec : e.captures) {
        c.captures.push_back(ec);
    }
    for (const auto& es : e.sections) {
        c.sections.push_back(es.id.name);
    }
}

std::optional<event_deep_copy> get_event(
    snitch::small_vector_span<const event_deep_copy> events,
    event_deep_copy::type                            type,
    std::size_t                                      id) {
    auto iter  = events.cbegin();
    bool first = true;

    do {
        if (!first) {
            ++iter;
            --id;
        }

        first = false;

        iter = std::find_if(
            iter, events.cend(), [&](const event_deep_copy& e) { return e.event_type == type; });
    } while (id != 0);

    if (iter == events.cend()) {
        return {};
    } else {
        return *iter;
    }
}

std::size_t
count_events(snitch::small_vector_span<const event_deep_copy> events, event_deep_copy::type type) {
    return std::count_if(events.cbegin(), events.cend(), [&](const event_deep_copy& e) {
        return e.event_type == type;
    });
}
} // namespace

event_deep_copy deep_copy(const snitch::event::data& e) {
    return std::visit(
        snitch::overload{
            [](const snitch::event::assertion_failed& a) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::assertion_failed;
                copy_test_case_id(c, a);
                copy_full_location(c, a);
                return c;
            },
            [](const snitch::event::assertion_succeeded& a) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::assertion_succeeded;
                copy_test_case_id(c, a);
                copy_full_location(c, a);
                return c;
            },
            [](const snitch::event::test_case_started& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_started;
                copy_test_case_id(c, s);
                return c;
            },
            [](const snitch::event::test_case_ended& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_ended;
                copy_test_case_id(c, s);
                c.test_case_state                  = s.state;
                c.test_case_assertion_count        = s.assertion_count;
                c.test_case_failure_count          = s.assertion_failure_count;
                c.test_case_expected_failure_count = s.allowed_assertion_failure_count;
                return c;
            },
            [](const snitch::event::test_run_started& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_run_started;
                copy_test_run_id(c, s);
                return c;
            },
            [](const snitch::event::test_run_ended& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_run_ended;
                copy_test_run_id(c, s);
                c.test_run_success                         = s.success;
                c.test_run_run_count                       = s.run_count;
                c.test_run_fail_count                      = s.fail_count;
                c.test_run_allowed_fail_count              = s.allowed_fail_count;
                c.test_run_skip_count                      = s.skip_count;
                c.test_run_assertion_count                 = s.assertion_count;
                c.test_run_assertion_failure_count         = s.assertion_failure_count;
                c.test_run_allowed_assertion_failure_count = s.allowed_assertion_failure_count;
                return c;
            },
            [](const snitch::event::test_case_skipped& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_skipped;
                copy_test_case_id(c, s);
                copy_full_location(c, s);
                return c;
            },
            [](const auto&) -> event_deep_copy { snitch::terminate_with("event not handled"); }},
        e);
}

void mock_framework::report(const snitch::registry&, const snitch::event::data& e) noexcept {
    auto evt = deep_copy(e);
    if (!catch_success && evt.event_type == event_deep_copy::type::assertion_succeeded) {
        return;
    }

    events.push_back(evt);
}

void mock_framework::print(std::string_view msg) noexcept {
    if (!append(messages, msg)) {
        snitch::terminate_with("not enough space in message buffer");
    }
}

void mock_framework::setup_reporter() {
    registry.verbose = snitch::registry::verbosity::full;

    registry.report_callback = {*this, snitch::constant<&mock_framework::report>{}};
    registry.print_callback  = &snitch::impl::stdout_print;
}

void mock_framework::setup_print() {
    registry.with_color = false;
    registry.verbose    = snitch::registry::verbosity::full;

    registry.report_callback = &snitch::impl::default_reporter;
    registry.print_callback  = {*this, snitch::constant<&mock_framework::print>{}};
}

void mock_framework::setup_reporter_and_print() {
    registry.with_color = false;
    registry.verbose    = snitch::registry::verbosity::full;

    registry.report_callback = {*this, snitch::constant<&mock_framework::report>{}};
    registry.print_callback  = {*this, snitch::constant<&mock_framework::print>{}};
}

void mock_framework::run_test() {
    registry.run(test_case);
}

std::optional<event_deep_copy> mock_framework::get_failure_event(std::size_t id) const {
    return get_event(events, event_deep_copy::type::assertion_failed, id);
}

std::optional<event_deep_copy> mock_framework::get_success_event(std::size_t id) const {
    return get_event(events, event_deep_copy::type::assertion_succeeded, id);
}

std::optional<event_deep_copy> mock_framework::get_skip_event() const {
    return get_event(events, event_deep_copy::type::test_case_skipped, 0u);
}

std::size_t mock_framework::get_num_registered_tests() const {
    return registry.test_cases().size();
}

std::size_t mock_framework::get_num_runs() const {
    return count_events(events, event_deep_copy::type::test_case_ended);
}

std::size_t mock_framework::get_num_failures() const {
    return count_events(events, event_deep_copy::type::assertion_failed);
}

std::size_t mock_framework::get_num_successes() const {
    return count_events(events, event_deep_copy::type::assertion_succeeded);
}

std::size_t mock_framework::get_num_skips() const {
    return count_events(events, event_deep_copy::type::test_case_skipped);
}

snitch::matchers::has_expr_data::has_expr_data(std::string_view msg) : expected{msg} {}

snitch::matchers::has_expr_data::has_expr_data(
    std::string_view type, std::string_view expected, std::string_view actual) :
    expected{expr_data{type, expected, actual}} {}

bool snitch::matchers::has_expr_data::match(const event_deep_copy& e) const noexcept {
    return std::visit(
        snitch::overload{
            [&](std::string_view message) { return message == e.expr_message; },
            [&](const expr_data& expr) {
                return expr.type == e.expr_type && expr.expected == e.expr_expected &&
                       expr.actual == e.expr_actual;
            }},
        expected);
}

snitch::small_string<snitch::max_message_length> snitch::matchers::has_expr_data::describe_match(
    const event_deep_copy& e, match_status) const noexcept {
    return std::visit(
        snitch::overload{
            [&](std::string_view message) {
                snitch::small_string<snitch::max_message_length> msg;
                if (e.expr_message != message) {
                    append_or_truncate(msg, "'", e.expr_message, "' != '", message, "'");
                } else {
                    append_or_truncate(msg, "'", message, "'");
                }
                return msg;
            },
            [&](const expr_data& expr) {
                snitch::small_string<snitch::max_message_length> msg;
                if (e.expr_type != expr.type) {
                    append_or_truncate(msg, "'", e.expr_type, "' != '", expr.type, "'");
                } else {
                    append_or_truncate(msg, "'", expr.type, "'");
                }
                if (e.expr_expected != expr.expected) {
                    append_or_truncate(
                        msg, " and '", e.expr_expected, "' != '", expr.expected, "'");
                } else {
                    append_or_truncate(msg, " and '", expr.expected, "'");
                }
                if (e.expr_actual != expr.actual) {
                    append_or_truncate(msg, " and '", e.expr_actual, "' != '", expr.actual, "'");
                } else {
                    append_or_truncate(msg, " and '", expr.actual, "'");
                }
                return msg;
            }},
        expected);
}
