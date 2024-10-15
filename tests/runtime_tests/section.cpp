#include "testing.hpp"
#include "testing_event.hpp"

#if SNITCH_WITH_EXCEPTIONS
#    include <stdexcept>
#endif
#include <string>

using namespace std::literals;

#if SNITCH_WITH_EXCEPTIONS
struct destructor_asserter {
    bool pass = true;

    ~destructor_asserter() {
        SNITCH_CHECK(pass);
    }
};
#endif

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("section", "[test macros]") {
    mock_framework framework;
    framework.setup_reporter();

    SECTION("no section") {
        framework.test_case.func = []() { SNITCH_FAIL_CHECK("trigger"); };

        framework.run_test();
        CHECK(framework.get_num_failures() == 1u);
        CHECK_NO_SECTION;
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 2u, 1u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 1u, 1u);
#endif
    }

    SECTION("single section") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                SNITCH_FAIL_CHECK("trigger");
            }
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 2u, 1u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 1u, 1u);
#endif
    }

    SECTION("two sections") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                SNITCH_FAIL_CHECK("trigger1");
            }
            SNITCH_SECTION("section 2") {
                SNITCH_FAIL_CHECK("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 2");
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 3u, 2u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 2u, 2u);
#endif
    }

    SECTION("nested sections") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                SNITCH_FAIL_CHECK("trigger1");
                SNITCH_SECTION("section 1.1") {
                    SNITCH_FAIL_CHECK("trigger2");
                }
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.1");
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 3u, 2u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 2u, 2u);
#endif
    }

#if SNITCH_WITH_EXCEPTIONS
    SECTION("nested sections abort early") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                SNITCH_FAIL("trigger1");
                SNITCH_SECTION("section 1.1") {
                    SNITCH_FAIL_CHECK("trigger2");
                }
            }
            SNITCH_SECTION("section 2") {
                SNITCH_FAIL("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
        CHECK_CASE(snitch::test_case_state::failed, 1u, 1u);
    }

    SECTION("nested sections std::exception throw") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                throw std::runtime_error("no can do");
                SNITCH_SECTION("section 1.1") {
                    SNITCH_FAIL_CHECK("trigger2");
                }
            }
            SNITCH_SECTION("section 2") {
                SNITCH_FAIL("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
        CHECK_CASE(snitch::test_case_state::failed, 1u, 1u);
    }

    SECTION("nested sections unknown exception throw") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                throw 1;
                SNITCH_SECTION("section 1.1") {
                    SNITCH_FAIL_CHECK("trigger2");
                }
            }
            SNITCH_SECTION("section 2") {
                SNITCH_FAIL("trigger2");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
        CHECK_CASE(snitch::test_case_state::failed, 1u, 1u);
    }

    SECTION("unexpected throw with destructor assert ok") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                destructor_asserter a{.pass = true};
                SNITCH_SECTION("section 2") {
                    throw std::runtime_error("no can do");
                }
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);

        CHECK_SECTIONS("section 1", "section 2");
        CHECK_CASE(snitch::test_case_state::failed, 2u, 1u);
    }

    SECTION("unexpected throw with destructor assert nok") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                destructor_asserter a{.pass = false};
                SNITCH_SECTION("section 2") {
                    throw std::runtime_error("no can do");
                }
            }
        };

        framework.run_test();

        // This is what we want:
        // REQUIRE(framework.get_num_failures() == 2u);
        // CHECK_SECTIONS_FOR_FAILURE(0u, "section 1", "section 2"); // exception
        // CHECK_SECTIONS_FOR_FAILURE(1u, "section 1");              // destructor
        // CHECK_CASE(snitch::test_case_state::failed, 2u, 2u);

        // This is what we get:
        REQUIRE(framework.get_num_failures() == 2u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1", "section 2"); // destructor
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 2"); // exception
        CHECK_CASE(snitch::test_case_state::failed, 2u, 2u);
    }
#endif

    SECTION("nested sections varying depth") {
        framework.test_case.func = []() {
            SNITCH_CHECK(true);

            SNITCH_SECTION("section 1") {
                SNITCH_SECTION("section 1.1") {
                }
                SNITCH_SECTION("section 1.2") {
                    SNITCH_FAIL_CHECK("trigger");
                }
                SNITCH_SECTION("section 1.3") {
                    SNITCH_SECTION("section 1.3.1") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                }
                SNITCH_SECTION("section 1.3") {
                }
            }
            SNITCH_SECTION("section 2") {
                SNITCH_SECTION("section 2.1") {
                    SNITCH_FAIL_CHECK("trigger");
                }
                SNITCH_FAIL_CHECK("trigger");
            }
            SNITCH_SECTION("section 3") {
                SNITCH_FAIL_CHECK("trigger");
            }
        };

        framework.run_test();

        // NB: the sections generate 6 repeats of the test.
        REQUIRE(framework.get_num_failures() == 5u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1", "section 1.2");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.3", "section 1.3.1");
        CHECK_SECTIONS_FOR_FAILURE(2u, "section 2", "section 2.1");
        CHECK_SECTIONS_FOR_FAILURE(3u, "section 2");
        CHECK_SECTIONS_FOR_FAILURE(4u, "section 3");
        // NB:
        // - 1 "no exceptions"
        // - 6 "CHECK(true)"
        // - 5 "trigger"
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 12u, 5u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 11u, 5u);
#endif
    }

    SECTION("nested sections multiple leaves") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                SNITCH_SECTION("section 1.1") {
                    SNITCH_SECTION("section 1.1.1") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                    SNITCH_SECTION("section 1.1.2") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                    SNITCH_SECTION("section 1.1.3") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                }
            }
            SNITCH_SECTION("section 2") {
                SNITCH_SECTION("section 2.1") {
                    SNITCH_SECTION("section 2.1.1") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                    SNITCH_SECTION("section 2.1.2") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                    SNITCH_SECTION("section 2.1.3") {
                        SNITCH_FAIL_CHECK("trigger");
                    }
                }
            }
        };

        framework.run_test();

        // NB: the sections generate 6 repeats of the test.
        REQUIRE(framework.get_num_failures() == 6u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1", "section 1.1", "section 1.1.1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1", "section 1.1", "section 1.1.2");
        CHECK_SECTIONS_FOR_FAILURE(2u, "section 1", "section 1.1", "section 1.1.3");
        CHECK_SECTIONS_FOR_FAILURE(3u, "section 2", "section 2.1", "section 2.1.1");
        CHECK_SECTIONS_FOR_FAILURE(4u, "section 2", "section 2.1", "section 2.1.2");
        CHECK_SECTIONS_FOR_FAILURE(5u, "section 2", "section 2.1", "section 2.1.3");
        // NB:
        // - 1 "no exception"
        // - 6 "trigger"
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 7u, 6u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 6u, 6u);
#endif
    }

    SECTION("one section in a loop") {
        framework.test_case.func = []() {
            for (std::size_t i = 0u; i < 5u; ++i) {
                SNITCH_CHECK(i <= 10u);

                SNITCH_SECTION("section 1") {
                    SNITCH_FAIL_CHECK("trigger");
                }
            }
        };

        framework.run_test();

        // NB: the sections generate 5 repeats of the test
        REQUIRE(framework.get_num_failures() == 5u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(2u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(3u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(4u, "section 1");
        // NB:
        // - 1 "no exceptions"
        // - 5x5 "i <= 10" (full loop executed for each repeat!)
        // - 5 "trigger"
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 31u, 5u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 30u, 5u);
#endif
    }

    SECTION("two sections in a loop") {
        framework.test_case.func = []() {
            for (std::size_t i = 0u; i < 5u; ++i) {
                SNITCH_CHECK(i <= 10u);

                SNITCH_SECTION("section 1") {
                    SNITCH_CHECK(i % 2u == 0u);
                }

                SNITCH_SECTION("section 2") {
                    SNITCH_CHECK(i % 2u == 1u);
                }
            }
        };

        framework.run_test();

        // NB: the sections generate 10 repeats of the test
        REQUIRE(framework.get_num_failures() == 5u);
        CHECK_SECTIONS_FOR_FAILURE(0u, "section 2");
        CHECK_SECTIONS_FOR_FAILURE(1u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(2u, "section 2");
        CHECK_SECTIONS_FOR_FAILURE(3u, "section 1");
        CHECK_SECTIONS_FOR_FAILURE(4u, "section 2");
        // NB:
        // - 1 "no exceptions"
        // - 10x5 "i <= 10" (full loop executed for each repeat!)
        // - 10 "i % 2u == x"
#if SNITCH_WITH_EXCEPTIONS
        CHECK_CASE(snitch::test_case_state::failed, 61u, 5u);
#else
        CHECK_CASE(snitch::test_case_state::failed, 60u, 5u);
#endif
    }

#if SNITCH_WITH_EXCEPTIONS
    SECTION("sections abort after discovery") {
        framework.test_case.func = []() {
            SNITCH_SECTION("section 1") {
                // Do nothing; allow the test to discover the other sections.
            }
            SNITCH_SECTION("section 2") {
                SNITCH_FAIL("trigger2");
            }
            SNITCH_SECTION("section 3") {
                SNITCH_FAIL("trigger3");
            }
        };

        framework.run_test();

        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 2");
        CHECK_CASE(snitch::test_case_state::failed, 1u, 1u);
    }

    SECTION("with handled exception") {
        framework.test_case.func = []() {
            try {
                SNITCH_SECTION("section 1") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
            }

            SNITCH_SECTION("section 2") {
                SNITCH_FAIL_CHECK("trigger");
            }
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 2");
    }

    SECTION("with handled exception no section") {
        framework.test_case.func = []() {
            try {
                SNITCH_SECTION("section 1") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
            }

            SNITCH_FAIL_CHECK("trigger");
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_NO_SECTION;
    }

    SECTION("with handled exceptions") {
        framework.test_case.func = []() {
            try {
                SNITCH_SECTION("section 1") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
            }

            try {
                SNITCH_SECTION("section 2") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
            }

            SNITCH_SECTION("section 3") {
                SNITCH_FAIL_CHECK("trigger");
            }
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 3");
    }

    SECTION("with handled exception then unhandled") {
        framework.test_case.func = []() {
            try {
                SNITCH_SECTION("section 1") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
            }

            SNITCH_SECTION("section 2") {
                throw std::runtime_error("bad");
            }
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 2");
    }

    SECTION("with handled exception then unhandled no section missing notify") {
        framework.test_case.func = []() {
            try {
                SNITCH_SECTION("section 1") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
            }

            throw std::runtime_error("bad");
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_SECTIONS("section 1");
    }

    SECTION("with handled exception then unhandled no section") {
        framework.test_case.func = []() {
            try {
                SNITCH_SECTION("section 1") {
                    throw std::runtime_error("bad");
                }
            } catch (...) {
                snitch::notify_exception_handled();
            }

            throw std::runtime_error("bad");
        };

        framework.run_test();
        REQUIRE(framework.get_num_failures() == 1u);
        CHECK_NO_SECTION;
    }
#endif
}

SNITCH_WARNING_POP

TEST_CASE("section readme example", "[test macros]") {
    mock_framework framework;

    snitch::small_string<32> events;

    auto print = [&](std::string_view s) noexcept {
        if (!events.empty()) {
            append_or_truncate(events, "|", s);
        } else {
            append_or_truncate(events, s);
        }
    };

    framework.registry.print_callback = print;

    framework.test_case.func = []() {
        auto& reg = snitch::impl::get_current_test().reg;

        reg.print("S");

        SNITCH_SECTION("first section") {
            reg.print("1");
        }
        SNITCH_SECTION("second section") {
            reg.print("2");
        }
        SNITCH_SECTION("third section") {
            reg.print("3");
            SNITCH_SECTION("nested section 1") {
                reg.print("3.1");
            }
            SNITCH_SECTION("nested section 2") {
                reg.print("3.2");
            }
        }

        reg.print("E");
    };

    framework.run_test();

    CHECK(events == "S|1|E|S|2|E|S|3|3.1|E|S|3|3.2|E"sv);
}
