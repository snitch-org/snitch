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
    append_or_truncate(c.message, e.message);
    for (const auto& ec : e.captures) {
        c.captures.push_back(ec);
    }
    for (const auto& es : e.sections) {
        c.sections.push_back(es.name);
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
                c.test_case_success         = s.success;
                c.test_case_assertion_count = s.assertion_count;
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
                c.test_run_success         = s.success;
                c.test_run_run_count       = s.run_count;
                c.test_run_fail_count      = s.fail_count;
                c.test_run_skip_count      = s.skip_count;
                c.test_run_assertion_count = s.assertion_count;
                return c;
            },
            [](const snitch::event::test_case_skipped& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_skipped;
                copy_full_location(c, s);
                return c;
            },
            [](const auto&) -> event_deep_copy { snitch::terminate_with("event not handled"); }},
        e);
}

void mock_framework::report(const snitch::registry&, const snitch::event::data& e) noexcept {
    events.push_back(deep_copy(e));
}

void mock_framework::print(std::string_view msg) noexcept {
    if (!append(messages, msg)) {
        snitch::terminate_with("not enough space in message buffer");
    }
}

void mock_framework::setup_reporter() {
    registry.report_callback = {*this, snitch::constant<&mock_framework::report>{}};
    registry.print_callback  = {};
}

void mock_framework::setup_print() {
    registry.with_color = false;
    registry.verbose    = snitch::registry::verbosity::high;

    registry.report_callback = {};
    registry.print_callback  = {*this, snitch::constant<&mock_framework::print>{}};
}

void mock_framework::setup_reporter_and_print() {
    registry.with_color = false;
    registry.verbose    = snitch::registry::verbosity::high;

    registry.report_callback = {*this, snitch::constant<&mock_framework::report>{}};
    registry.print_callback  = {*this, snitch::constant<&mock_framework::print>{}};
}

void mock_framework::run_test() {
    registry.run(test_case);
}

std::optional<event_deep_copy> mock_framework::get_failure_event(std::size_t id) const {
    return get_event(events, event_deep_copy::type::assertion_failed, id);
}

std::optional<event_deep_copy> mock_framework::get_skip_event() const {
    return get_event(events, event_deep_copy::type::test_case_skipped, 0u);
}

std::size_t mock_framework::get_num_registered_tests() const {
    return registry.end() - registry.begin();
}

std::size_t mock_framework::get_num_runs() const {
    return count_events(events, event_deep_copy::type::test_case_ended);
}

std::size_t mock_framework::get_num_failures() const {
    return count_events(events, event_deep_copy::type::assertion_failed);
}

std::size_t mock_framework::get_num_skips() const {
    return count_events(events, event_deep_copy::type::test_case_skipped);
}
