#include "testing.hpp"
#include "testing_event.hpp"

#include <string>

using namespace std::literals;

TEST_CASE("section", "[test macros]") {
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

    SECTION("no section") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_FAIL("trigger");
        };

        mock_registry.run(mock_case);
        CHECK(get_num_failures(events) == 1u);
        CHECK_NO_SECTION;
    }

    SECTION("single section") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL("trigger");
            }
        };

        mock_registry.run(mock_case);
        REQUIRE(get_num_failures(events) == 1u);
        CHECK_SECTIONS("section 1");
    }

    SECTION("two sections") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL("trigger1");
            }
            SNATCH_SECTION("section 2") {
                SNATCH_FAIL("trigger2");
            }
        };

        mock_registry.run(mock_case);

        REQUIRE(get_num_failures(events) == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 2");
    }

    SECTION("nested sections") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL_CHECK("trigger1");
                SNATCH_SECTION("section 1.1") {
                    SNATCH_FAIL_CHECK("trigger2");
                }
            }
        };

        mock_registry.run(mock_case);

        REQUIRE(get_num_failures(events) == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.1");
    }

    SECTION("nested sections abort early") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL("trigger1");
                SNATCH_SECTION("section 1.1") {
                    SNATCH_FAIL_CHECK("trigger2");
                }
            }
        };

        mock_registry.run(mock_case);

        REQUIRE(get_num_failures(events) == 1u);
        CHECK_SECTIONS("section 1");
    }

    SECTION("nested sections varying depth") {
        mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_SECTION("section 1.1") {}
                SNATCH_SECTION("section 1.2") {
                    SNATCH_FAIL_CHECK("trigger");
                }
                SNATCH_SECTION("section 1.3") {
                    SNATCH_SECTION("section 1.3.1") {
                        SNATCH_FAIL_CHECK("trigger");
                    }
                }
                SNATCH_SECTION("section 1.3") {}
            }
            SNATCH_SECTION("section 2") {
                SNATCH_SECTION("section 2.1") {
                    SNATCH_FAIL_CHECK("trigger");
                }
                SNATCH_FAIL_CHECK("trigger");
            }
            SNATCH_SECTION("section 3") {
                SNATCH_FAIL_CHECK("trigger");
            }
        };

        mock_registry.run(mock_case);

        REQUIRE(get_num_failures(events) == 5u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1", "section 1.2");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.3", "section 1.3.1");
        CHECK_SECTIONS_FOR_FAILURE(2u, "section 2", "section 2.1");
        CHECK_SECTIONS_FOR_FAILURE(3u, "section 2");
        CHECK_SECTIONS_FOR_FAILURE(4u, "section 3");
    }
};

TEST_CASE("section readme example", "[test macros]") {
    snatch::registry mock_registry;

    snatch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snatch::impl::test_state::not_run};

    snatch::small_string<32> events;

    auto print = [&](std::string_view s) noexcept {
        if (!events.empty()) {
            append_or_truncate(events, "|", s);
        } else {
            append_or_truncate(events, s);
        }
    };

    mock_registry.print_callback = print;

    mock_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
        SNATCH_CURRENT_TEST.reg.print("S");

        SECTION("first section") {
            SNATCH_CURRENT_TEST.reg.print("1");
        }
        SECTION("second section") {
            SNATCH_CURRENT_TEST.reg.print("2");
        }
        SECTION("third section") {
            SNATCH_CURRENT_TEST.reg.print("3");
            SECTION("nested section 1") {
                SNATCH_CURRENT_TEST.reg.print("3.1");
            }
            SECTION("nested section 2") {
                SNATCH_CURRENT_TEST.reg.print("3.2");
            }
        }

        SNATCH_CURRENT_TEST.reg.print("E");
    };

    mock_registry.run(mock_case);

    CHECK(events == "S|1|E|S|2|E|S|3|3.1|E|S|3|3.2|E"sv);
};
