#include "testing.hpp"
#include "testing_event.hpp"

#include <algorithm>
#include <string>
#include <vector>

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

#define CHECK_CAPTURES(...)                                                                        \
    do {                                                                                           \
        auto failure = get_failure_event(events);                                                  \
        REQUIRE(failure.has_value());                                                              \
        const char* EXPECTED_CAPTURES[] = {__VA_ARGS__};                                           \
        REQUIRE(                                                                                   \
            failure.value().captures.size() == sizeof(EXPECTED_CAPTURES) / sizeof(const char*));   \
        std::size_t CAPTURE_INDEX = 0;                                                             \
        for (std::string_view CAPTURED_VALUE : EXPECTED_CAPTURES) {                                \
            CHECK(failure.value().captures[CAPTURE_INDEX] == CAPTURED_VALUE);                      \
            ++CAPTURE_INDEX;                                                                       \
        }                                                                                          \
    } while (0)

#define CHECK_NO_CAPTURE                                                                           \
    do {                                                                                           \
        auto failure = get_failure_event(events);                                                  \
        REQUIRE(failure.has_value());                                                              \
        CHECK(failure.value().captures.empty());                                                   \
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
        CHECK_CAPTURES("1 := 1");
    }

    SECTION("literal string") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_CAPTURE("hello");
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("\"hello\" := hello");
    }

    SECTION("variable int") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_CAPTURE(i);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("i := 1");
    }

    SECTION("variable string") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            std::string s = "hello";
            SNATCH_CAPTURE(s);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("s := hello");
    }

    SECTION("expression int") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_CAPTURE(2 * i + 1);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("2 * i + 1 := 3");
    }

    SECTION("expression string") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            std::string s = "hello";
            SNATCH_CAPTURE(s + ", 'world' (string),)(");
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("s + \", 'world' (string),)(\" := hello, 'world' (string),)(");
    }

    SECTION("expression function call & char") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            std::string s = "hel\"lo";
            SNATCH_CAPTURE(s.find_first_of('e'));
            SNATCH_CAPTURE(s.find_first_of('"'));
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("s.find_first_of('e') := 1", "s.find_first_of('\"') := 3");
    }

    SECTION("two variables") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            int j = 2;
            SNATCH_CAPTURE(i, j);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("i := 1", "j := 2");
    }

    SECTION("three variables different types") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int         i = 1;
            int         j = 2;
            std::string s = "hello";
            SNATCH_CAPTURE(i, j, s);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("i := 1", "j := 2", "s := hello");
    }

    SECTION("scoped out") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            {
                int i = 1;
                SNATCH_CAPTURE(i);
            }
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_NO_CAPTURE;
    }

    SECTION("scoped out multiple") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_CAPTURE(i);

            {
                int j = 2;
                SNATCH_CAPTURE(j);
            }

            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("i := 1");
    }
};

TEST_CASE("info", "[test macros]") {
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
            SNATCH_INFO(1);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("1");
    }

    SECTION("literal string") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_INFO("hello");
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("hello");
    }

    SECTION("variable int") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_INFO(i);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("1");
    }

    SECTION("variable string") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            std::string s = "hello";
            SNATCH_INFO(s);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("hello");
    }

    SECTION("expression int") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_INFO(2 * i + 1);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("3");
    }

    SECTION("expression string") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            std::string s = "hello";
            SNATCH_INFO(s + ", 'world'");
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("hello, 'world'");
    }

    SECTION("multiple") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int         i = 1;
            int         j = 2;
            std::string s = "hello";
            SNATCH_INFO(i, " and ", j);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("1 and 2");
    }

    SECTION("scoped out") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            {
                int i = 1;
                SNATCH_INFO(i);
            }
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_NO_CAPTURE;
    }

    SECTION("scoped out multiple") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_INFO(i);

            {
                int j = 2;
                SNATCH_INFO(j);
            }

            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("1");
    }

    SECTION("mixed with capture") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_INFO(i);
            SNATCH_CAPTURE(i);
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK_CAPTURES("1", "i := 1");
    }
};
