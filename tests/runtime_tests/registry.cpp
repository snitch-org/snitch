#include "testing.hpp"
#include "testing_event.hpp"

#include <stdexcept>

using namespace std::literals;
using snatch::matchers::contains_substring;

bool        test_called       = false;
bool        test_called_int   = false;
bool        test_called_float = false;
std::size_t failure_line      = 0u;

TEST_CASE("add regular test", "[registry]") {
    mock_framework framework;

    test_called                                        = false;
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run&) {
        test_called = true;
    };

    REQUIRE(framework.get_num_registered_tests() == 1u);

    auto& test = *framework.registry.begin();
    CHECK(test.id.name == "how many lights"sv);
    CHECK(test.id.tags == "[tag]"sv);
    CHECK(test.id.type == ""sv);
    REQUIRE(test.func != nullptr);

    SECTION("run default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(test_called == true);
        CHECK(framework.messages == contains_substring("starting: how many lights"));
        CHECK(framework.messages == contains_substring("finished: how many lights"));
    }

    SECTION("run custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        CHECK(test_called == true);
        REQUIRE(framework.events.size() == 2u);
        CHECK(framework.events[0].event_type == event_deep_copy::type::test_case_started);
        CHECK(framework.events[1].event_type == event_deep_copy::type::test_case_ended);
        CHECK_EVENT_TEST_ID(framework.events[0], test.id);
        CHECK_EVENT_TEST_ID(framework.events[1], test.id);
    }
};

TEST_CASE("add template test", "[registry]") {
    mock_framework framework;

    test_called       = false;
    test_called_int   = false;
    test_called_float = false;

    framework.registry.add_with_types<std::tuple<int, float>>("how many lights", "[tag]") =
        []<typename T>(snatch::impl::test_run&) {
            if constexpr (std::is_same_v<T, int>) {
                test_called_int = true;
            } else if constexpr (std::is_same_v<T, float>) {
                test_called_float = true;
            } else {
                test_called = true;
            }
        };

    REQUIRE(framework.get_num_registered_tests() == 2u);

    auto& test1 = *framework.registry.begin();
    CHECK(test1.id.name == "how many lights"sv);
    CHECK(test1.id.tags == "[tag]"sv);
    CHECK(test1.id.type == "int"sv);
    REQUIRE(test1.func != nullptr);

    auto& test2 = *(framework.registry.begin() + 1);
    CHECK(test2.id.name == "how many lights"sv);
    CHECK(test2.id.tags == "[tag]"sv);
    CHECK(test2.id.type == "float"sv);
    REQUIRE(test2.func != nullptr);

    SECTION("run int default reporter") {
        framework.setup_print();
        framework.registry.run(test1);

        CHECK(test_called == false);
        CHECK(test_called_int == true);
        CHECK(test_called_float == false);
        CHECK(framework.messages == contains_substring("starting: how many lights [int]"));
        CHECK(framework.messages == contains_substring("finished: how many lights [int]"));
    }

    SECTION("run float default reporter") {
        framework.setup_print();
        framework.registry.run(test2);

        CHECK(test_called == false);
        CHECK(test_called_int == false);
        CHECK(test_called_float == true);
        CHECK(framework.messages == contains_substring("starting: how many lights [float]"));
        CHECK(framework.messages == contains_substring("finished: how many lights [float]"));
    }

    SECTION("run int custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test1);

        CHECK(test_called == false);
        CHECK(test_called_int == true);
        CHECK(test_called_float == false);
        REQUIRE(framework.events.size() == 2u);
        CHECK(framework.events[0].event_type == event_deep_copy::type::test_case_started);
        CHECK(framework.events[1].event_type == event_deep_copy::type::test_case_ended);
        CHECK_EVENT_TEST_ID(framework.events[0], test1.id);
        CHECK_EVENT_TEST_ID(framework.events[1], test1.id);
    }

    SECTION("run float custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test2);

        CHECK(test_called == false);
        CHECK(test_called_int == false);
        CHECK(test_called_float == true);
        REQUIRE(framework.events.size() == 2u);
        CHECK(framework.events[0].event_type == event_deep_copy::type::test_case_started);
        CHECK(framework.events[1].event_type == event_deep_copy::type::test_case_ended);
        CHECK_EVENT_TEST_ID(framework.events[0], test2.id);
        CHECK_EVENT_TEST_ID(framework.events[1], test2.id);
    }
};

SNATCH_WARNING_PUSH
SNATCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("report FAIL regular", "[registry]") {
    mock_framework framework;

#define SNATCH_CURRENT_TEST mock_test
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run& mock_test) {
        // clang-format off
        failure_line = __LINE__; SNATCH_FAIL("there are four lights");
        // clang-format on
    };
#undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("there are four lights"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, failure_line);
        CHECK(failure.message == contains_substring("there are four lights"));
    }
};

TEST_CASE("report FAIL template", "[registry]") {
    mock_framework framework;

#define SNATCH_CURRENT_TEST mock_test
    framework.registry.add_with_types<std::tuple<int>>("how many lights", "[tag]") =
        []<typename TestType>(snatch::impl::test_run& mock_test) {
            // clang-format off
            failure_line = __LINE__; SNATCH_FAIL("there are four lights");
            // clang-format on
        };
#undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("for type int"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("there are four lights"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, failure_line);
        CHECK(failure.message == contains_substring("there are four lights"));
    }
};

TEST_CASE("report FAIL section", "[registry]") {
    mock_framework framework;

#define SNATCH_CURRENT_TEST mock_test
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run& mock_test) {
        SNATCH_SECTION("ask nicely") {
            // clang-format off
            failure_line = __LINE__; SNATCH_FAIL("there are four lights");
            // clang-format on
        }
    };
#undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("in section \"ask nicely\""));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("there are four lights"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, failure_line);
        CHECK(failure.message == contains_substring("there are four lights"));
        REQUIRE(failure.sections.size() == 1u);
        REQUIRE(failure.sections[0] == "ask nicely"sv);
    }
};

TEST_CASE("report FAIL capture", "[registry]") {
    mock_framework framework;

#define SNATCH_CURRENT_TEST mock_test
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run& mock_test) {
        int number_of_lights = 3;
        SNATCH_CAPTURE(number_of_lights);
        // clang-format off
        failure_line = __LINE__; SNATCH_FAIL("there are four lights");
        // clang-format on
    };
#undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("with number_of_lights := 3"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("there are four lights"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, failure_line);
        CHECK(failure.message == contains_substring("there are four lights"));
        REQUIRE(failure.captures.size() == 1u);
        REQUIRE(failure.captures[0] == "number_of_lights := 3"sv);
    }
};

TEST_CASE("report REQUIRE", "[registry]") {
    mock_framework framework;

#define SNATCH_CURRENT_TEST mock_test
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run& mock_test) {
        int number_of_lights = 4;
        // clang-format off
        failure_line = __LINE__; SNATCH_REQUIRE(number_of_lights == 3);
        // clang-format on
    };
#undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("number_of_lights == 3"));
        CHECK(framework.messages == contains_substring("4 != 3"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);
    }
};

#if SNATCH_WITH_EXCEPTIONS
TEST_CASE("report REQUIRE_THROWS_AS", "[registry]") {
    mock_framework framework;

#    define SNATCH_CURRENT_TEST mock_test
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run& mock_test) {
        auto ask_how_many_lights = [] { throw std::runtime_error{"there are four lights"}; };
        // clang-format off
        failure_line = __LINE__; SNATCH_REQUIRE_THROWS_AS(ask_how_many_lights(), std::logic_error);
        // clang-format on
    };
#    undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(
            framework.messages ==
            contains_substring("std::logic_error expected but other std::exception thrown"));
        CHECK(framework.messages == contains_substring("there are four lights"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);
    }
#endif
};

TEST_CASE("report SKIP", "[registry]") {
    mock_framework framework;

#define SNATCH_CURRENT_TEST mock_test
    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run& mock_test) {
        // clang-format off
        failure_line = __LINE__; SNATCH_SKIP("there are four lights");
        // clang-format on
    };
#undef SNATCH_CURRENT_TEST

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("there are four lights"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);
    }
};

SNATCH_WARNING_POP

TEST_CASE("add regular test", "[registry]") {
    mock_framework framework;

    test_called = false;

    framework.registry.add("how are you", "[tag]") = [](snatch::impl::test_run&) {
        test_called = true;
    };

    framework.registry.add("how many lights", "[tag]") = [](snatch::impl::test_run&) {
        test_called = true;
        SNATCH_FAIL_CHECK("there are four lights");
    };

    framework.registry.add_with_types<std::tuple<int, float>>(
        "how many lights templated", "[tag]") = []<typename T>(snatch::impl::test_run&) {
        if constexpr (std::is_same_v<T, int>) {
            test_called_int = true;
            SNATCH_FAIL_CHECK("there are four lights (int)");
        } else if constexpr (std::is_same_v<T, float>) {
            test_called_float = true;
            SNATCH_FAIL_CHECK("there are four lights (float)");
        } else {
            test_called = true;
            SNATCH_FAIL_CHECK("there are four lights (unreachable)");
        }
    };

    SECTION("run all tests") {
        framework.registry.run_tests();

        // TODO:
    }

    SECTION("run tests filtered") {
        framework.registry.run_tests();

        // TODO:
    };

    SECTION("run tests filtered all pass") {
        framework.registry.run_tests();

        // TODO:
    };
