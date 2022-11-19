#include "testing.hpp"
#include "testing_event.hpp"

#include <algorithm>

using namespace std::literals;

std::optional<event_deep_copy> get_failure_event(const std::vector<event_deep_copy>& events) {
    auto iter = std::find_if(events.cbegin(), events.cend(), [](const event_deep_copy& e) {
        return e.event_type == event_deep_copy::type::assertion_failed;
    });

    if (iter == events.cend()) {
        return {};
    } else {
        return *iter;
    }
}

#define CHECK_CAPTURE(CAPTURED_VALUE)                                                              \
    do {                                                                                           \
        auto failure = get_failure_event(events);                                                  \
        REQUIRE(failure.has_value());                                                              \
        REQUIRE(failure.value().captures.size() == 1u);                                            \
        REQUIRE(failure.value().captures[0] == CAPTURED_VALUE##sv);                                \
    } while (0)

TEST_CASE("capture", "[test macros]") {
    snatch::registry mock_registry;

    snatch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snatch::impl::test_state::not_run};

    std::vector<event_deep_copy> events;
    auto report = [&](const snatch::registry&, const snatch::event::data& e) noexcept {
        events.push_back(deep_copy(e));
    };

    mock_registry.report_callback = report;

    SECTION("literal int") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_CAPTURE(1);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURE("1 := 1");
    }
};
