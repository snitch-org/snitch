#include "testing.hpp"
#include "testing_event.hpp"

#include <stdexcept>
#include <string>

using namespace std::literals;

SNATCH_WARNING_PUSH
SNATCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("section", "[test macros]") {
    mock_framework framework;
    framework.setup_reporter();

    SECTION("no section") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_FAIL_CHECK("trigger");
        };

        framework.run_test();
        CHECK(framework.get_num_failures() == 1u);
        CHECK_NO_SECTION;
    }

    SECTION("single section") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL_CHECK("trigger");
            }
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
    }

    SECTION("two sections") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL_CHECK("trigger1");
            }
            SNATCH_SECTION("section 2") {
                SNATCH_FAIL_CHECK("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 2");
    }

    SECTION("nested sections") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL_CHECK("trigger1");
                SNATCH_SECTION("section 1.1") {
                    SNATCH_FAIL_CHECK("trigger2");
                }
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.1");
    }

    SECTION("nested sections abort early") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_FAIL("trigger1");
                SNATCH_SECTION("section 1.1") {
                    SNATCH_FAIL_CHECK("trigger2");
                }
            }
            SNATCH_SECTION("section 2") {
                SNATCH_FAIL("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
    }

    SECTION("nested sections std::exception throw") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                throw std::runtime_error("no can do");
                SNATCH_SECTION("section 1.1") {
                    SNATCH_FAIL_CHECK("trigger2");
                }
            }
            SNATCH_SECTION("section 2") {
                SNATCH_FAIL("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_NO_SECTION;
    }

    SECTION("nested sections unknown exception throw") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                throw 1;
                SNATCH_SECTION("section 1.1") {
                    SNATCH_FAIL_CHECK("trigger2");
                }
            }
            SNATCH_SECTION("section 2") {
                SNATCH_FAIL("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_NO_SECTION;
    }

    SECTION("nested sections varying depth") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
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

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 5u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1", "section 1.2");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.3", "section 1.3.1");
        CHECK_SECTIONS_FOR_FAILURE(2u, "section 2", "section 2.1");
        CHECK_SECTIONS_FOR_FAILURE(3u, "section 2");
        CHECK_SECTIONS_FOR_FAILURE(4u, "section 3");
    }
};

SNATCH_WARNING_POP

TEST_CASE("section readme example", "[test macros]") {
    mock_framework framework;

    snatch::small_string<32> events;

    auto print = [&](std::string_view s) noexcept {
        if (!events.empty()) {
            append_or_truncate(events, "|", s);
        } else {
            append_or_truncate(events, s);
        }
    };

    framework.registry.print_callback  = print;
    framework.registry.report_callback = {};

    framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
        SNATCH_CURRENT_TEST.reg.print("S");

        SNATCH_SECTION("first section") {
            SNATCH_CURRENT_TEST.reg.print("1");
        }
        SNATCH_SECTION("second section") {
            SNATCH_CURRENT_TEST.reg.print("2");
        }
        SNATCH_SECTION("third section") {
            SNATCH_CURRENT_TEST.reg.print("3");
            SNATCH_SECTION("nested section 1") {
                SNATCH_CURRENT_TEST.reg.print("3.1");
            }
            SNATCH_SECTION("nested section 2") {
                SNATCH_CURRENT_TEST.reg.print("3.2");
            }
        }

        SNATCH_CURRENT_TEST.reg.print("E");
    };

    framework.run_test();

    CHECK(events == "S|1|E|S|2|E|S|3|3.1|E|S|3|3.2|E"sv);
};
