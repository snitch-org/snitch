#include "testing.hpp"
#include "testing_event.hpp"

#include <string>

using namespace std::literals;

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

    SECTION("scoped out multiple capture") {
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

    SECTION("multiple failures") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_CAPTURE(i);
            SNATCH_FAIL_CHECK("trigger1");
            SNATCH_FAIL_CHECK("trigger2");
        };

        mock_registry.run(mock_case);
        REQUIRE(get_num_failures(events) == 2u);
        CHECK_CAPTURES_FOR_FAILURE(0u, "i := 1");
        CHECK_CAPTURES_FOR_FAILURE(1u, "i := 1");
    }

    SECTION("multiple failures interleaved") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_CAPTURE(i);
            SNATCH_FAIL_CHECK("trigger1");
            SNATCH_CAPTURE(2 * i);
            SNATCH_FAIL_CHECK("trigger2");
        };

        mock_registry.run(mock_case);
        REQUIRE(get_num_failures(events) == 2u);
        CHECK_CAPTURES_FOR_FAILURE(0u, "i := 1");
        CHECK_CAPTURES_FOR_FAILURE(1u, "i := 1", "2 * i := 2");
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

    SECTION("multiple failures") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_INFO(i);
            SNATCH_FAIL_CHECK("trigger1");
            SNATCH_FAIL_CHECK("trigger2");
        };

        mock_registry.run(mock_case);
        REQUIRE(get_num_failures(events) == 2u);
        CHECK_CAPTURES_FOR_FAILURE(0u, "1");
        CHECK_CAPTURES_FOR_FAILURE(1u, "1");
    }

    SECTION("multiple failures interleaved") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            int i = 1;
            SNATCH_INFO(i);
            SNATCH_FAIL_CHECK("trigger1");
            SNATCH_INFO(2 * i);
            SNATCH_FAIL_CHECK("trigger2");
        };

        mock_registry.run(mock_case);
        REQUIRE(get_num_failures(events) == 2u);
        CHECK_CAPTURES_FOR_FAILURE(0u, "1");
        CHECK_CAPTURES_FOR_FAILURE(1u, "1", "2");
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
