// clang-format off
#include "testing.hpp"
#include "testing_event.hpp"
// clang-format on

#include <algorithm>

namespace {
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

std::optional<event_deep_copy>
get_event(const std::vector<event_deep_copy>& events, event_deep_copy::type type, std::size_t id) {
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

std::size_t count_events(const std::vector<event_deep_copy>& events, event_deep_copy::type type) {
    return std::count_if(events.cbegin(), events.cend(), [&](const event_deep_copy& e) {
        return e.event_type == type;
    });
}
} // namespace

event_deep_copy deep_copy(const snatch::event::data& e) {
    return std::visit(
        snatch::overload{
            [](const snatch::event::assertion_failed& a) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::assertion_failed;
                copy_test_case_id(c, a);
                copy_full_location(c, a);
                return c;
            },
            [](const snatch::event::test_case_started& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_started;
                copy_test_case_id(c, s);
                return c;
            },
            [](const snatch::event::test_case_ended& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_ended;
                copy_test_case_id(c, s);
                return c;
            },
            [](const snatch::event::test_case_skipped& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_skipped;
                copy_full_location(c, s);
                return c;
            },
            [](const auto&) -> event_deep_copy { snatch::terminate_with("event not handled"); }},
        e);
}

mock_framework::mock_framework() {
    registry.report_callback = {*this, snatch::constant<&mock_framework::report>{}};
}

void mock_framework::report(const snatch::registry&, const snatch::event::data& e) noexcept {
    events.push_back(deep_copy(e));
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

std::size_t mock_framework::get_num_runs() const {
    return count_events(events, event_deep_copy::type::test_case_ended);
}

std::size_t mock_framework::get_num_failures() const {
    return count_events(events, event_deep_copy::type::assertion_failed);
}

std::size_t mock_framework::get_num_skips() const {
    return count_events(events, event_deep_copy::type::test_case_skipped);
}
