// clang-format off
#include "testing.hpp"
#include "testing_event.hpp"
// clang-format on

#include <algorithm>

namespace {
template<typename U, typename T>
void copy_test_run_id(U& c, const T& e) {
    append_or_truncate(c.name, e.name);
    for (const auto& f : e.filters) {
        c.filters.grow(1);
        append_or_truncate(c.filters.back(), f);
    }
}

template<typename U, typename T>
void copy_test_case_id(U& c, const T& e) {
    append_or_truncate(c.id.name, e.id.name);
    append_or_truncate(c.id.tags, e.id.tags);
    append_or_truncate(c.id.type, e.id.type);
}

template<typename U, typename T>
void copy_location(U& c, const T& e) {
    append_or_truncate(c.location.file, e.location.file);
    c.location.line = e.location.line;
}

template<typename U, typename T>
void copy_assertion_location(U& c, const T& e) {
    copy_location(c, e);

    for (const auto& ec : e.captures) {
        c.captures.grow(1);
        append_or_truncate(c.captures.back(), ec);
    }

    for (const auto& es : e.sections) {
        c.sections.grow(1);
        append_or_truncate(c.sections.back().id.name, es.id.name);
        append_or_truncate(c.sections.back().id.description, es.id.description);
        copy_location(c.sections.back(), es);
    }
}

template<typename U, typename T>
void copy_assertion_data(U& c, const T& e) {
    std::visit(
        snitch::overload{
            [&](std::string_view message) {
                owning_event::string msg;
                append_or_truncate(msg, message);
                c.data = msg;
            },
            [&](const snitch::expression_info& exp) {
                owning_event::expression_info info;
                append_or_truncate(info.type, exp.type);
                append_or_truncate(info.expected, exp.expected);
                append_or_truncate(info.actual, exp.actual);
                c.data = info;
            }},
        e.data);
}

template<typename T>
std::optional<T>
get_nth_event(snitch::small_vector_span<const owning_event::data> events, std::size_t id) {
    auto iter  = events.cbegin();
    bool first = true;

    do {
        if (!first) {
            ++iter;
            --id;
        }

        first = false;

        iter = std::find_if(iter, events.cend(), [&](const owning_event::data& e) {
            return std::holds_alternative<T>(e);
        });
    } while (id != 0);

    if (iter == events.cend()) {
        return {};
    } else {
        return std::get<T>(*iter);
    }
}

template<typename T>
std::size_t count_events(snitch::small_vector_span<const owning_event::data> events) {
    return std::count_if(events.cbegin(), events.cend(), [&](const owning_event::data& e) {
        return std::holds_alternative<T>(e);
    });
}
} // namespace

owning_event::data deep_copy(const snitch::event::data& e) {
    return std::visit(
        snitch::overload{
            [](const snitch::event::assertion_failed& a) -> owning_event::data {
                owning_event::assertion_failed c;
                copy_test_case_id(c, a);
                copy_assertion_location(c, a);
                copy_assertion_data(c, a);
                c.allowed  = a.allowed;
                c.expected = a.expected;
                return c;
            },
            [](const snitch::event::assertion_succeeded& a) -> owning_event::data {
                owning_event::assertion_succeeded c;
                copy_test_case_id(c, a);
                copy_assertion_location(c, a);
                copy_assertion_data(c, a);
                return c;
            },
            [](const snitch::event::test_case_started& s) -> owning_event::data {
                owning_event::test_case_started c;
                copy_test_case_id(c, s);
                copy_location(c, s);
                return c;
            },
            [](const snitch::event::test_case_ended& s) -> owning_event::data {
                owning_event::test_case_ended c;
                copy_test_case_id(c, s);
                copy_location(c, s);
                c.assertion_count                 = s.assertion_count;
                c.assertion_failure_count         = s.assertion_failure_count;
                c.allowed_assertion_failure_count = s.allowed_assertion_failure_count;
                c.state                           = s.state;
#if SNITCH_WITH_TIMINGS
                c.duration = s.duration;
#endif
                c.failure_expected = s.failure_expected;
                c.failure_allowed  = s.failure_allowed;
                return c;
            },
            [](const snitch::event::test_run_started& s) -> owning_event::data {
                owning_event::test_run_started c;
                copy_test_run_id(c, s);
                return c;
            },
            [](const snitch::event::test_run_ended& s) -> owning_event::data {
                owning_event::test_run_ended c;
                copy_test_run_id(c, s);
                c.run_count                       = s.run_count;
                c.fail_count                      = s.fail_count;
                c.allowed_fail_count              = s.allowed_fail_count;
                c.skip_count                      = s.skip_count;
                c.assertion_count                 = s.assertion_count;
                c.assertion_failure_count         = s.assertion_failure_count;
                c.allowed_assertion_failure_count = s.allowed_assertion_failure_count;
#if SNITCH_WITH_TIMINGS
                c.duration = s.duration;
#endif
                c.success = s.success;
                return c;
            },
            [](const snitch::event::test_case_skipped& s) -> owning_event::data {
                owning_event::test_case_skipped c;
                copy_test_case_id(c, s);
                copy_assertion_location(c, s);
                append_or_truncate(c.message, s.message);
                return c;
            },
            [](const auto&) -> owning_event::data { snitch::terminate_with("event not handled"); }},
        e);
}

std::optional<owning_event::test_id> get_test_id(const owning_event::data& e) noexcept {
    return std::visit(
        [](const auto& a) -> std::optional<owning_event::test_id> {
            using event_type = std::decay_t<decltype(a)>;
            if constexpr (requires(const event_type& t) {
                              { t.id } -> snitch::convertible_to<owning_event::test_id>;
                          }) {
                return a.id;
            } else {
                return {};
            }
        },
        e);
}

std::optional<owning_event::source_location> get_location(const owning_event::data& e) noexcept {
    return std::visit(
        [](const auto& a) -> std::optional<owning_event::source_location> {
            using event_type = std::decay_t<decltype(a)>;
            if constexpr (requires(const event_type& t) {
                              {
                                  t.location
                                  } -> snitch::convertible_to<owning_event::source_location>;
                          }) {
                return a.location;
            } else {
                return {};
            }
        },
        e);
}

mock_framework::mock_framework() noexcept {
    registry.add_reporter(
        "console", &snitch::reporter::console::initialize, &snitch::reporter::console::configure,
        &snitch::reporter::console::report, {});

    registry.print_callback = [](std::string_view msg) noexcept {
        snitch::cli::console_print(msg);
    };
}

void mock_framework::report(const snitch::registry&, const snitch::event::data& e) noexcept {
    if (!catch_success && std::holds_alternative<snitch::event::assertion_succeeded>(e)) {
        return;
    }

    events.push_back(deep_copy(e));
}

void mock_framework::print(std::string_view) noexcept {}

void mock_framework::setup_reporter() {
    registry.verbose         = snitch::registry::verbosity::full;
    registry.report_callback = {*this, snitch::constant<&mock_framework::report>{}};
}

void mock_framework::run_test() {
    registry.run(test_case);
}

std::optional<owning_event::assertion_failed>
mock_framework::get_failure_event(std::size_t id) const {
    return get_nth_event<owning_event::assertion_failed>(events, id);
}

std::optional<owning_event::assertion_succeeded>
mock_framework::get_success_event(std::size_t id) const {
    return get_nth_event<owning_event::assertion_succeeded>(events, id);
}

std::optional<owning_event::test_case_skipped> mock_framework::get_skip_event() const {
    return get_nth_event<owning_event::test_case_skipped>(events, 0u);
}

std::size_t mock_framework::get_num_registered_tests() const {
    return registry.test_cases().size();
}

std::size_t mock_framework::get_num_runs() const {
    return count_events<owning_event::test_case_ended>(events);
}

std::size_t mock_framework::get_num_failures() const {
    return count_events<owning_event::assertion_failed>(events);
}

std::size_t mock_framework::get_num_successes() const {
    return count_events<owning_event::assertion_succeeded>(events);
}

std::size_t mock_framework::get_num_skips() const {
    return count_events<owning_event::test_case_skipped>(events);
}

snitch::matchers::has_expr_data::has_expr_data(std::string_view msg) : expected{msg} {}

snitch::matchers::has_expr_data::has_expr_data(
    std::string_view type, std::string_view expected, std::string_view actual) :
    expected{expr_data{type, expected, actual}} {}

bool snitch::matchers::has_expr_data::match(const owning_event::data& e) const noexcept {
    return std::visit(
        [&](const auto& a) {
            using event_type = std::decay_t<decltype(a)>;
            if constexpr (
                std::is_same_v<event_type, owning_event::assertion_succeeded> ||
                std::is_same_v<event_type, owning_event::assertion_failed>) {
                return std::visit(
                    snitch::overload{
                        [&](std::string_view actual_message, std::string_view expected_message) {
                            return actual_message == expected_message;
                        },
                        [&](const owning_event::expression_info& actual_expr,
                            const expr_data&                     expected_expr) {
                            return actual_expr.type == expected_expr.type &&
                                   actual_expr.expected == expected_expr.expected &&
                                   actual_expr.actual == expected_expr.actual;
                        },
                        [&](const owning_event::expression_info&, std::string_view) {
                            return false;
                        },
                        [&](std::string_view, const expr_data&) { return false; }},
                    a.data, expected);
            } else {
                return false;
            }
        },
        e);
}

snitch::small_string<snitch::max_message_length> snitch::matchers::has_expr_data::describe_match(
    const owning_event::data& e, match_status) const noexcept {

    snitch::small_string<snitch::max_message_length> msg;
    std::visit(
        [&](const auto& a) {
            using event_type = std::decay_t<decltype(a)>;
            if constexpr (
                std::is_same_v<event_type, owning_event::assertion_succeeded> ||
                std::is_same_v<event_type, owning_event::assertion_failed>) {
                std::visit(
                    snitch::overload{
                        [&](std::string_view actual_message, std::string_view expected_message) {
                            if (actual_message != expected_message) {
                                append_or_truncate(
                                    msg, "'", actual_message, "' != '", expected_message, "'");
                            } else {
                                append_or_truncate(msg, "'", expected_message, "'");
                            }
                        },
                        [&](const owning_event::expression_info& actual_expr,
                            const expr_data&                     expected_expr) {
                            if (actual_expr.type != expected_expr.type) {
                                append_or_truncate(
                                    msg, "'", actual_expr.type, "' != '", expected_expr.type, "'");
                            } else {
                                append_or_truncate(msg, "'", expected_expr.type, "'");
                            }
                            if (actual_expr.expected != expected_expr.expected) {
                                append_or_truncate(
                                    msg, " and '", actual_expr.expected, "' != '",
                                    expected_expr.expected, "'");
                            } else {
                                append_or_truncate(msg, " and '", expected_expr.expected, "'");
                            }
                            if (actual_expr.actual != expected_expr.actual) {
                                append_or_truncate(
                                    msg, " and '", actual_expr.actual, "' != '",
                                    expected_expr.actual, "'");
                            } else {
                                append_or_truncate(msg, " and '", expected_expr.actual, "'");
                            }
                        },
                        [&](const owning_event::expression_info&, std::string_view) {
                            append_or_truncate(msg, "expected message, got expression");
                        },
                        [&](std::string_view, const expr_data&) {
                            append_or_truncate(msg, "expected expression, got message");
                        }},
                    a.data, expected);
            } else {
                append_or_truncate(msg, "event is not an assertion event");
            }
        },
        e);

    return msg;
}
