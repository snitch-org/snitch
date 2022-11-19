// clang-format off
#include "testing.hpp"
#include "testing_event.hpp"
// clang-format on

#include <algorithm>

event_deep_copy deep_copy(const snatch::event::data& e) {
    return std::visit(
        snatch::overload{
            [](const snatch::event::assertion_failed& a) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::assertion_failed;
                append_or_truncate(c.test_id_name, a.id.name);
                append_or_truncate(c.test_id_tags, a.id.tags);
                append_or_truncate(c.test_id_type, a.id.type);
                append_or_truncate(c.location_file, a.location.file);
                c.location_line = a.location.line;
                append_or_truncate(c.message, a.message);
                for (const auto& ac : a.captures) {
                    c.captures.push_back(ac);
                }
                for (const auto& as : a.sections) {
                    c.sections.push_back(as.name);
                }
                return c;
            },
            [](const snatch::event::test_case_started& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_started;
                append_or_truncate(c.test_id_name, s.id.name);
                append_or_truncate(c.test_id_tags, s.id.tags);
                append_or_truncate(c.test_id_type, s.id.type);
                return c;
            },
            [](const snatch::event::test_case_ended& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_ended;
                append_or_truncate(c.test_id_name, s.id.name);
                append_or_truncate(c.test_id_tags, s.id.tags);
                append_or_truncate(c.test_id_type, s.id.type);
                return c;
            },
            [](const auto&) -> event_deep_copy { snatch::terminate_with("event not handled"); }},
        e);
}

std::optional<event_deep_copy>
get_failure_event(const std::vector<event_deep_copy>& events, std::size_t id) {
    auto iter  = events.cbegin();
    bool first = true;

    do {
        if (!first) {
            ++iter;
            --id;
        }

        first = false;

        iter = std::find_if(iter, events.cend(), [](const event_deep_copy& e) {
            return e.event_type == event_deep_copy::type::assertion_failed;
        });
    } while (id != 0);

    if (iter == events.cend()) {
        return {};
    } else {
        return *iter;
    }
}

std::size_t get_num_runs(const std::vector<event_deep_copy>& events) {
    return std::count_if(events.cbegin(), events.cend(), [](const event_deep_copy& e) {
        return e.event_type == event_deep_copy::type::test_case_ended;
    });
}

std::size_t get_num_failures(const std::vector<event_deep_copy>& events) {
    return std::count_if(events.cbegin(), events.cend(), [](const event_deep_copy& e) {
        return e.event_type == event_deep_copy::type::assertion_failed;
    });
}

void pop_first_run(std::vector<event_deep_copy>& events) {
    auto iter = std::find_if(events.cbegin(), events.cend(), [](const event_deep_copy& e) {
        return e.event_type == event_deep_copy::type::test_case_ended;
    });

    events.erase(events.begin(), iter);
}

void pop_first_failure(std::vector<event_deep_copy>& events) {
    auto iter = std::find_if(events.cbegin(), events.cend(), [](const event_deep_copy& e) {
        return e.event_type == event_deep_copy::type::assertion_failed;
    });

    if (iter != events.end()) {
        events.erase(iter);
    }
}
