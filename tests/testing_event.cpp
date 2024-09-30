// clang-format off
#include "testing.hpp"
#include "testing_event.hpp"
// clang-format on

#include <algorithm>

std::string_view append_to_pool(snitch::small_string_span pool, std::string_view msg) {
    const char* start = pool.end();
    if (!append(pool, msg)) {
        snitch::terminate_with(
            "message pool is full; increase size in mock_framework or event_catcher");
    }

    return std::string_view{start, msg.size()};
}

namespace {
template<typename U, typename T>
void copy_test_run_id(snitch::small_string_span pool, U& c, const T& e) {
    c.name = append_to_pool(pool, e.name);
    for (const auto& f : e.filters) {
        c.filters.push_back(append_to_pool(pool, f));
    }
}

template<typename U, typename T>
void copy_test_case_id(snitch::small_string_span pool, U& c, const T& e) {
    c.id.name = append_to_pool(pool, e.id.name);
    c.id.tags = append_to_pool(pool, e.id.tags);
    c.id.type = append_to_pool(pool, e.id.type);
}

template<typename U, typename T>
void copy_location(snitch::small_string_span pool, U& c, const T& e) {
    c.location.file = append_to_pool(pool, e.location.file);
    c.location.line = e.location.line;
}

template<typename U, typename T>
void copy_assertion_location(snitch::small_string_span pool, U& c, const T& e) {
    copy_location(pool, c, e);

    for (const auto& ec : e.captures) {
        c.captures.push_back(append_to_pool(pool, ec));
    }

    for (const auto& es : e.sections) {
        c.sections.grow(1);
        c.sections.back().id.name        = append_to_pool(pool, es.id.name);
        c.sections.back().id.description = append_to_pool(pool, es.id.description);
        copy_location(pool, c.sections.back(), es);
    }
}

template<typename U, typename T>
void copy_assertion_data(snitch::small_string_span pool, U& c, const T& e) {
    std::visit(
        snitch::overload{
            [&](std::string_view message) { c.data = append_to_pool(pool, message); },
            [&](const snitch::expression_info& exp) {
                c.data = snitch::expression_info{
                    .type     = append_to_pool(pool, exp.type),
                    .expected = append_to_pool(pool, exp.expected),
                    .actual   = append_to_pool(pool, exp.actual)};
            }},
        e.data);
}

template<typename T>
std::pair<std::optional<T>, std::size_t>
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
        return {std::nullopt, events.size()};
    } else {
        return {std::get<T>(*iter), iter - events.cbegin()};
    }
}

template<typename T>
std::size_t count_events(snitch::small_vector_span<const owning_event::data> events) {
    return std::count_if(events.cbegin(), events.cend(), [&](const owning_event::data& e) {
        return std::holds_alternative<T>(e);
    });
}
} // namespace

owning_event::data deep_copy(snitch::small_string_span pool, const snitch::event::data& e) {
    return std::visit(
        snitch::overload{
            [&](const snitch::event::assertion_failed& a) -> owning_event::data {
                owning_event::assertion_failed c;
                copy_test_case_id(pool, c, a);
                copy_assertion_location(pool, c, a);
                copy_assertion_data(pool, c, a);
                c.allowed  = a.allowed;
                c.expected = a.expected;
                return c;
            },
            [&](const snitch::event::assertion_succeeded& a) -> owning_event::data {
                owning_event::assertion_succeeded c;
                copy_test_case_id(pool, c, a);
                copy_assertion_location(pool, c, a);
                copy_assertion_data(pool, c, a);
                return c;
            },
            [&](const snitch::event::test_case_started& s) -> owning_event::data {
                owning_event::test_case_started c;
                copy_test_case_id(pool, c, s);
                copy_location(pool, c, s);
                return c;
            },
            [&](const snitch::event::test_case_ended& s) -> owning_event::data {
                owning_event::test_case_ended c;
                copy_test_case_id(pool, c, s);
                copy_location(pool, c, s);
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
            [&](const snitch::event::section_started& s) -> owning_event::data {
                owning_event::section_started c;
                copy_location(pool, c, s);
                c.id.name        = append_to_pool(pool, s.id.name);
                c.id.description = append_to_pool(pool, s.id.description);
                return c;
            },
            [&](const snitch::event::section_ended& s) -> owning_event::data {
                owning_event::section_ended c;
                copy_location(pool, c, s);
                c.id.name                         = append_to_pool(pool, s.id.name);
                c.id.description                  = append_to_pool(pool, s.id.description);
                c.skipped                         = s.skipped;
                c.assertion_count                 = s.assertion_count;
                c.assertion_failure_count         = s.assertion_failure_count;
                c.allowed_assertion_failure_count = s.allowed_assertion_failure_count;
#if SNITCH_WITH_TIMINGS
                c.duration = s.duration;
#endif
                return c;
            },
            [&](const snitch::event::test_run_started& s) -> owning_event::data {
                owning_event::test_run_started c;
                copy_test_run_id(pool, c, s);
                return c;
            },
            [&](const snitch::event::test_run_ended& s) -> owning_event::data {
                owning_event::test_run_ended c;
                copy_test_run_id(pool, c, s);
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
            [&](const snitch::event::test_case_skipped& s) -> owning_event::data {
                owning_event::test_case_skipped c;
                copy_test_case_id(pool, c, s);
                copy_assertion_location(pool, c, s);
                append_or_truncate(pool, c.message, s.message);
                return c;
            },
            [&](const snitch::event::list_test_run_started& s) -> owning_event::data {
                owning_event::list_test_run_started c;
                copy_test_run_id(pool, c, s);
                return c;
            },
            [&](const snitch::event::list_test_run_ended& s) -> owning_event::data {
                owning_event::list_test_run_started c;
                copy_test_run_id(pool, c, s);
                return c;
            },
            [&](const snitch::event::test_case_listed& s) -> owning_event::data {
                owning_event::test_case_listed c;
                copy_test_case_id(pool, c, s);
                return c;
            },
            [](const auto&) -> owning_event::data { snitch::terminate_with("event not handled"); }},
        e);
}

std::optional<owning_event::assertion_failed>
get_failure_event(snitch::small_vector_span<const owning_event::data> events, std::size_t id) {
    return get_nth_event<owning_event::assertion_failed>(events, id).first;
}

std::optional<owning_event::assertion_succeeded>
get_success_event(snitch::small_vector_span<const owning_event::data> events, std::size_t id) {
    return get_nth_event<owning_event::assertion_succeeded>(events, id).first;
}

std::optional<snitch::test_id> get_test_id(const owning_event::data& e) noexcept {
    return std::visit(
        [](const auto& a) -> std::optional<snitch::test_id> {
            using event_type = std::decay_t<decltype(a)>;
            if constexpr (requires(const event_type& t) {
                              { t.id } -> snitch::convertible_to<snitch::test_id>;
                          }) {
                return a.id;
            } else {
                return {};
            }
        },
        e);
}

std::optional<snitch::source_location> get_location(const owning_event::data& e) noexcept {
    return std::visit(
        [](const auto& a) -> std::optional<snitch::source_location> {
            using event_type = std::decay_t<decltype(a)>;
            if constexpr (requires(const event_type& t) {
                              { t.location } -> snitch::convertible_to<snitch::source_location>;
                          }) {
                return a.location;
            } else {
                return {};
            }
        },
        e);
}

mock_framework::mock_framework() noexcept {
    registry.add_reporter<snitch::reporter::console::reporter>("console");

    registry.print_callback = [](std::string_view msg) noexcept {
        snitch::cli::console_print(msg);
    };
}

void mock_framework::report(const snitch::registry&, const snitch::event::data& e) noexcept {
    if (!catch_success && std::holds_alternative<snitch::event::assertion_succeeded>(e)) {
        return;
    }

    events.push_back(deep_copy(string_pool, e));
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
    return get_nth_event<owning_event::assertion_failed>(events, id).first;
}

std::optional<owning_event::assertion_succeeded>
mock_framework::get_success_event(std::size_t id) const {
    return get_nth_event<owning_event::assertion_succeeded>(events, id).first;
}

bool mock_framework::check_balanced_section_events() const {
    bool                                                                  test_case_started = false;
    snitch::small_vector<snitch::section_id, snitch::max_nested_sections> sections;
    for (const auto& e : events) {
        bool good = std::visit(
            snitch::overload{
                [&](const owning_event::section_started& s) {
                    if (!test_case_started) {
                        return false;
                    }
                    sections.push_back(s.id);
                    return true;
                },
                [&](const owning_event::section_ended& s) {
                    if (!test_case_started) {
                        return false;
                    }
                    if (sections.empty()) {
                        return false;
                    }
                    if (sections.back().name != s.id.name ||
                        sections.back().description != s.id.description) {
                        return false;
                    }

                    sections.pop_back();
                    return true;
                },
                [&](const owning_event::test_case_started&) {
                    test_case_started = true;
                    return sections.empty();
                },
                [&](const owning_event::test_case_ended&) {
                    test_case_started = false;
                    return sections.empty();
                },
                [](const auto&) { return true; }},
            e);

        if (!good) {
            return false;
        }
    }

    return sections.empty();
}

snitch::small_vector<std::string_view, snitch::max_nested_sections>
mock_framework::get_sections_for_failure_event(std::size_t id) const {
    auto [event, pos] = get_nth_event<owning_event::assertion_failed>(events, id);

    snitch::small_vector<std::string_view, snitch::max_nested_sections> sections;
    for (std::size_t i = 0; i < pos; ++i) {
        std::visit(
            snitch::overload{
                [&](const owning_event::section_started& s) { sections.push_back(s.id.name); },
                [&](const owning_event::section_ended&) { sections.pop_back(); },
                [](const auto&) {}},
            events[i]);
    }

    return sections;
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

std::size_t mock_framework::get_num_listed_tests() const {
    return count_events<owning_event::test_case_listed>(events);
}

bool mock_framework::is_test_listed(const snitch::test_id& id) const {
    return std::find_if(events.cbegin(), events.cend(), [&](const owning_event::data& e) {
               if (auto* t = std::get_if<owning_event::test_case_listed>(&e)) {
                   return t->id.name == id.name && t->id.type == id.type && t->id.tags == id.tags &&
                          t->id.fixture == id.fixture;
               }
               return false;
           }) != events.cend();
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
                        [&](const snitch::expression_info& actual_expr,
                            const expr_data&               expected_expr) {
                            return actual_expr.type == expected_expr.type &&
                                   actual_expr.expected == expected_expr.expected &&
                                   actual_expr.actual == expected_expr.actual;
                        },
                        [&](const snitch::expression_info&, std::string_view) { return false; },
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
                        [&](const snitch::expression_info& actual_expr,
                            const expr_data&               expected_expr) {
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
                        [&](const snitch::expression_info&, std::string_view) {
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
