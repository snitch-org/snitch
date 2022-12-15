#include "testing.hpp"
#include "testing_event.hpp"

#include <stdexcept>

using namespace std::literals;
using snatch::matchers::contains_substring;

bool        test_called           = false;
bool        test_called_other_tag = false;
bool        test_called_skipped   = false;
bool        test_called_int       = false;
bool        test_called_float     = false;
bool        test_called_hidden1   = false;
bool        test_called_hidden2   = false;
std::size_t failure_line          = 0u;

enum class reporter { print, custom };

TEST_CASE("add regular test", "[registry]") {
    mock_framework framework;

    test_called = false;
    framework.registry.add("how many lights", "[tag]", []() { test_called = true; });

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
}

TEST_CASE("add template test", "[registry]") {
    for (bool with_type_list : {false, true}) {
        mock_framework framework;

        test_called       = false;
        test_called_int   = false;
        test_called_float = false;

        CAPTURE(with_type_list);

        if (with_type_list) {
            framework.registry.add_with_type_list<snatch::type_list<int, float>>(
                "how many lights", "[tag]", []<typename T>() {
                    if constexpr (std::is_same_v<T, int>) {
                        test_called_int = true;
                    } else if constexpr (std::is_same_v<T, float>) {
                        test_called_float = true;
                    } else {
                        test_called = true;
                    }
                });
        } else {
            framework.registry.add_with_types<int, float>(
                "how many lights", "[tag]", []<typename T>() {
                    if constexpr (std::is_same_v<T, int>) {
                        test_called_int = true;
                    } else if constexpr (std::is_same_v<T, float>) {
                        test_called_float = true;
                    } else {
                        test_called = true;
                    }
                });
        }

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
    }
}

SNATCH_WARNING_PUSH
SNATCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("report FAIL regular", "[registry]") {
    mock_framework framework;

    framework.registry.add("how many lights", "[tag]", []() {
        // clang-format off
        failure_line = __LINE__; SNATCH_FAIL("there are four lights");
        // clang-format on
    });

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
}

TEST_CASE("report FAIL template", "[registry]") {
    mock_framework framework;

    framework.registry.add_with_types<int>("how many lights", "[tag]", []<typename TestType>() {
        // clang-format off
        failure_line = __LINE__; SNATCH_FAIL("there are four lights");
        // clang-format on
    });

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
}

TEST_CASE("report FAIL section", "[registry]") {
    mock_framework framework;

    framework.registry.add("how many lights", "[tag]", []() {
        SNATCH_SECTION("ask nicely") {
            // clang-format off
            failure_line = __LINE__; SNATCH_FAIL("there are four lights");
            // clang-format on
        }
    });

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
}

TEST_CASE("report FAIL capture", "[registry]") {
    mock_framework framework;

    framework.registry.add("how many lights", "[tag]", []() {
        int number_of_lights = 3;
        SNATCH_CAPTURE(number_of_lights);
        // clang-format off
        failure_line = __LINE__; SNATCH_FAIL("there are four lights");
        // clang-format on
    });

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
}

TEST_CASE("report REQUIRE", "[registry]") {
    mock_framework framework;

    framework.registry.add("how many lights", "[tag]", []() {
        int number_of_lights = 4;
        // clang-format off
        failure_line = __LINE__; SNATCH_REQUIRE(number_of_lights == 3);
        // clang-format on
    });

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
}

#if SNATCH_WITH_EXCEPTIONS
TEST_CASE("report REQUIRE_THROWS_AS", "[registry]") {
    mock_framework framework;

    framework.registry.add("how many lights", "[tag]", []() {
        auto ask_how_many_lights = [] { throw std::runtime_error{"there are four lights"}; };
        // clang-format off
        failure_line = __LINE__; SNATCH_REQUIRE_THROWS_AS(ask_how_many_lights(), std::logic_error);
        // clang-format on
    });

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
}
#endif

TEST_CASE("report SKIP", "[registry]") {
    mock_framework framework;

    framework.registry.add("how many lights", "[tag]", []() {
        // clang-format off
        failure_line = __LINE__; SNATCH_SKIP("there are four lights");
        // clang-format on
    });

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
}

SNATCH_WARNING_POP

namespace {
void register_tests(mock_framework& framework) {
    test_called           = false;
    test_called_other_tag = false;
    test_called_skipped   = false;
    test_called_int       = false;
    test_called_float     = false;
    test_called_hidden1   = false;
    test_called_hidden2   = false;

    framework.registry.add("how are you", "[tag]", []() { test_called = true; });

    framework.registry.add("how many lights", "[tag][other_tag]", []() {
        test_called_other_tag = true;
        SNATCH_FAIL_CHECK("there are four lights");
    });

    framework.registry.add("drink from the cup", "[tag][skipped]", []() {
        test_called_skipped = true;
        SNATCH_SKIP("not thirsty");
    });

    framework.registry.add_with_types<int, float>(
        "how many templated lights", "[tag][tag with spaces]", []<typename T>() {
            if constexpr (std::is_same_v<T, int>) {
                test_called_int = true;
                SNATCH_FAIL_CHECK("there are four lights (int)");
            } else if constexpr (std::is_same_v<T, float>) {
                test_called_float = true;
                SNATCH_FAIL_CHECK("there are four lights (float)");
            }
        });

    framework.registry.add(
        "hidden test 1", "[.][hidden][other_tag]", []() { test_called_hidden1 = true; });

    framework.registry.add("hidden test 2", "[.hidden]", []() { test_called_hidden2 = true; });

    framework.registry.add("may fail that does not fail", "[.][may fail][!mayfail]", []() {});

    framework.registry.add(
        "may fail that does fail", "[.][may fail][!mayfail]", []() { SNATCH_FAIL("it did fail"); });

    framework.registry.add(
        "should fail that does not fail", "[.][should fail][!shouldfail]", []() {});

    framework.registry.add("should fail that does fail", "[.][should fail][!shouldfail]", []() {
        SNATCH_FAIL("it did fail");
    });

    framework.registry.add(
        "may+should fail that does not fail", "[.][may+should fail][!mayfail][!shouldfail]",
        []() {});

    framework.registry.add(
        "may+should fail that does fail", "[.][may+should fail][!mayfail][!shouldfail]",
        []() { SNATCH_FAIL("it did fail"); });
}
} // namespace

TEST_CASE("run tests", "[registry]") {
    mock_framework framework;
    register_tests(framework);

    for (auto r : {reporter::print, reporter::custom}) {
        if (r == reporter::print) {
            framework.setup_print();
        } else {
            framework.setup_reporter();
        }

        INFO((r == reporter::print ? "default reporter" : "custom reporter"));

        SECTION("run all tests") {
            framework.registry.run_all_tests("test_app");

            CHECK(test_called);
            CHECK(test_called_other_tag);
            CHECK(test_called_skipped);
            CHECK(test_called_int);
            CHECK(test_called_float);
            CHECK(!test_called_hidden1);
            CHECK(!test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("some tests failed (3 out of 5 test cases, 3 assertions, 1 "
                                       "test cases skipped)"));
            } else {
                CHECK(framework.get_num_runs() == 5u);
                CHECK_RUN(false, 5u, 3u, 1u, 3u);
            }
        }

        SECTION("run tests filtered all pass") {
            framework.registry.run_tests_matching_name("test_app", "are you");

            CHECK(test_called);
            CHECK(!test_called_other_tag);
            CHECK(!test_called_skipped);
            CHECK(!test_called_int);
            CHECK(!test_called_float);
            CHECK(!test_called_hidden1);
            CHECK(!test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (1 test cases, 0 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 1u);
                CHECK_RUN(true, 1u, 0u, 0u, 0u);
            }
        }

        SECTION("run tests filtered all failed") {
            framework.registry.run_tests_matching_name("test_app", "lights");

            CHECK(!test_called);
            CHECK(test_called_other_tag);
            CHECK(!test_called_skipped);
            CHECK(test_called_int);
            CHECK(test_called_float);
            CHECK(!test_called_hidden1);
            CHECK(!test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("some tests failed (3 out of 3 test cases, 3 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 3u);
                CHECK_RUN(false, 3u, 3u, 0u, 3u);
            }
        }

        SECTION("run tests filtered all skipped") {
            framework.registry.run_tests_matching_name("test_app", "cup");

            CHECK(!test_called);
            CHECK(!test_called_other_tag);
            CHECK(test_called_skipped);
            CHECK(!test_called_int);
            CHECK(!test_called_float);
            CHECK(!test_called_hidden1);
            CHECK(!test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (1 test cases, 0 assertions, 1 "
                                       "test cases skipped)"));
            } else {
                CHECK(framework.get_num_runs() == 1u);
                CHECK_RUN(true, 1u, 0u, 1u, 0u);
            }
        }

        SECTION("run tests filtered tags") {
            framework.registry.run_tests_with_tag("test_app", "[other_tag]");

            CHECK(!test_called);
            CHECK(test_called_other_tag);
            CHECK(!test_called_skipped);
            CHECK(!test_called_int);
            CHECK(!test_called_float);
            CHECK(test_called_hidden1);
            CHECK(!test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("some tests failed (1 out of 2 test cases, 1 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(false, 2u, 1u, 0u, 1u);
            }
        }

        SECTION("run tests special tag [.]") {
            framework.registry.run_tests_with_tag("test_app", "[hidden]");

            CHECK(!test_called);
            CHECK(!test_called_other_tag);
            CHECK(!test_called_skipped);
            CHECK(!test_called_int);
            CHECK(!test_called_float);
            CHECK(test_called_hidden1);
            CHECK(test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (2 test cases, 0 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(true, 2u, 0u, 0u, 0u);
            }
        }

        SECTION("run tests special tag [!mayfail]") {
            framework.registry.run_tests_with_tag("test_app", "[may fail]");

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (2 test cases, 1 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(true, 2u, 0u, 0u, 1u);
            }
        }

        SECTION("run tests special tag [!shouldfail]") {
            framework.registry.run_tests_with_tag("test_app", "[should fail]");

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("some tests failed (1 out of 2 test cases, 1 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(false, 2u, 1u, 0u, 1u);
            }
        }

        SECTION("run tests special tag [!shouldfail][!mayfail]") {
            framework.registry.run_tests_with_tag("test_app", "[may+should fail]");

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (2 test cases, 1 assertions)"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(true, 2u, 0u, 0u, 1u);
            }
        }
    }
}

TEST_CASE("list tests", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    framework.setup_print();

    SECTION("list_all_tests") {
        framework.registry.list_all_tests();

        CHECK(framework.messages == contains_substring("how are you"));
        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("drink from the cup"));
        CHECK(framework.messages == contains_substring("how many templated lights [int]"));
        CHECK(framework.messages == contains_substring("how many templated lights [float]"));
        CHECK(framework.messages == contains_substring("hidden test 1"));
        CHECK(framework.messages == contains_substring("hidden test 2"));
    }

    SECTION("list_all_tags") {
        framework.registry.list_all_tags();

        CHECK(framework.messages == contains_substring("[tag]"));
        CHECK(framework.messages == contains_substring("[skipped]"));
        CHECK(framework.messages == contains_substring("[other_tag]"));
        CHECK(framework.messages == contains_substring("[tag with spaces]"));
        CHECK(framework.messages == contains_substring("[hidden]"));
        CHECK(framework.messages != contains_substring("[.]"));
        CHECK(framework.messages != contains_substring("[.hidden]"));
    }

    SECTION("list_tests_with_tag") {
        for (auto tag :
             {"[tag]"sv, "[other_tag]"sv, "[skipped]"sv, "[tag with spaces]"sv, "[wrong_tag]"sv,
              "[hidden]"sv, "[.]"sv, "[.hidden]"sv}) {

            CAPTURE(tag);
            framework.messages.clear();

            framework.registry.list_tests_with_tag(tag);
            if (tag == "[tag]"sv) {
                CHECK(framework.messages == contains_substring("how are you"));
                CHECK(framework.messages == contains_substring("how many lights"));
                CHECK(framework.messages == contains_substring("drink from the cup"));
                CHECK(framework.messages == contains_substring("how many templated lights [int]"));
                CHECK(
                    framework.messages == contains_substring("how many templated lights [float]"));
            } else if (tag == "[other_tag]"sv) {
                CHECK(framework.messages != contains_substring("how are you"));
                CHECK(framework.messages == contains_substring("how many lights"));
                CHECK(framework.messages != contains_substring("drink from the cup"));
                CHECK(framework.messages != contains_substring("how many templated lights [int]"));
                CHECK(
                    framework.messages != contains_substring("how many templated lights [float]"));
            } else if (tag == "[skipped]"sv) {
                CHECK(framework.messages != contains_substring("how are you"));
                CHECK(framework.messages != contains_substring("how many lights"));
                CHECK(framework.messages == contains_substring("drink from the cup"));
                CHECK(framework.messages != contains_substring("how many templated lights [int]"));
                CHECK(
                    framework.messages != contains_substring("how many templated lights [float]"));
            } else if (tag == "[tag with spaces]"sv) {
                CHECK(framework.messages != contains_substring("how are you"));
                CHECK(framework.messages != contains_substring("how many lights"));
                CHECK(framework.messages != contains_substring("drink from the cup"));
                CHECK(framework.messages == contains_substring("how many templated lights [int]"));
                CHECK(
                    framework.messages == contains_substring("how many templated lights [float]"));
            } else if (tag == "[hidden]"sv) {
                CHECK(framework.messages == contains_substring("hidden test 1"));
                CHECK(framework.messages == contains_substring("hidden test 2"));
            } else if (tag == "[wrong_tag]"sv || tag == "[.]"sv || tag == "[.hidden]"sv) {
                CHECK(framework.messages.empty());
            }
        }
    }
}

TEST_CASE("configure", "[registry]") {
    mock_framework framework;
    framework.setup_print();
    register_tests(framework);

    SECTION("color = always") {
        const arg_vector args = {"test", "--color", "always"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.with_color == true);
    }

    SECTION("color = never") {
        const arg_vector args = {"test", "--color", "never"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.with_color == false);
    }

    SECTION("color = bad") {
        const arg_vector args = {"test", "--color", "bad"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.messages == contains_substring("unknown color directive"));
    }

    SECTION("verbosity = quiet") {
        const arg_vector args = {"test", "--verbosity", "quiet"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snatch::registry::verbosity::quiet);
    }

    SECTION("verbosity = normal") {
        const arg_vector args = {"test", "--verbosity", "normal"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snatch::registry::verbosity::normal);
    }

    SECTION("verbosity = high") {
        const arg_vector args = {"test", "--verbosity", "high"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snatch::registry::verbosity::high);
    }

    SECTION("verbosity = bad") {
        const arg_vector args = {"test", "--verbosity", "bad"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.messages == contains_substring("unknown verbosity level"));
    }
}

TEST_CASE("run tests cli", "[registry]") {
    mock_framework framework;
    framework.setup_reporter_and_print();
    register_tests(framework);
    console_output_catcher console;

    SECTION("no argument") {
        const arg_vector args = {"test"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(false, 5u, 3u, 1u, 3u);
    }

    SECTION("--help") {
        const arg_vector args = {"test", "--help"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(console.messages == contains_substring("test [options...]"));
    }

    SECTION("--list-tests") {
        const arg_vector args = {"test", "--list-tests"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(framework.messages == contains_substring("how are you"));
        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("drink from the cup"));
        CHECK(framework.messages == contains_substring("how many templated lights [int]"));
        CHECK(framework.messages == contains_substring("how many templated lights [float]"));
    }

    SECTION("--list-tags") {
        const arg_vector args = {"test", "--list-tags"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(framework.messages == contains_substring("[tag]"));
        CHECK(framework.messages == contains_substring("[skipped]"));
        CHECK(framework.messages == contains_substring("[other_tag]"));
        CHECK(framework.messages == contains_substring("[tag with spaces]"));
    }

    SECTION("--list-tests-with-tag") {
        const arg_vector args = {"test", "--list-tests-with-tag", "[other_tag]"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(framework.messages != contains_substring("how are you"));
        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages != contains_substring("drink from the cup"));
        CHECK(framework.messages != contains_substring("how many templated lights [int]"));
        CHECK(framework.messages != contains_substring("how many templated lights [float]"));
    }

    SECTION("test filter") {
        const arg_vector args = {"test", "how many"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(false, 3u, 3u, 0u, 3u);
    }

    SECTION("test tag filter") {
        const arg_vector args = {"test", "--tags", "[skipped]"};
        auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(true, 1u, 0u, 1u, 0u);
    }
}
