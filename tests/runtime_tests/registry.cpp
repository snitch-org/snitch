#include "testing.hpp"
#include "testing_event.hpp"

#include <stdexcept>

using namespace std::literals;
using snitch::matchers::contains_substring;

namespace {
bool        test_called           = false;
bool        test_called_other_tag = false;
bool        test_called_skipped   = false;
bool        test_called_int       = false;
bool        test_called_float     = false;
bool        test_called_hidden1   = false;
bool        test_called_hidden2   = false;
std::size_t failure_line          = 0u;

enum class reporter { print, custom };
} // namespace

TEST_CASE("add regular test", "[registry]") {
    mock_framework framework;

    test_called = false;
    framework.registry.add(
        {"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() { test_called = true; });

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

TEST_CASE("add regular test no tags", "[registry]") {
    mock_framework framework;

    test_called = false;
    framework.registry.add({"how many lights"}, {__FILE__, __LINE__}, []() { test_called = true; });

    REQUIRE(framework.get_num_registered_tests() == 1u);

    auto& test = *framework.registry.begin();
    CHECK(test.id.name == "how many lights"sv);
    CHECK(test.id.tags == ""sv);
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
            framework.registry.add_with_type_list<snitch::type_list<int, float>>(
                "how many lights", "[tag]", {__FILE__, __LINE__}, []<typename T>() {
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
                "how many lights", "[tag]", {__FILE__, __LINE__}, []<typename T>() {
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
            CHECK(framework.messages == contains_substring("starting: how many lights <int>"));
            CHECK(framework.messages == contains_substring("finished: how many lights <int>"));
        }

        SECTION("run float default reporter") {
            framework.setup_print();
            framework.registry.run(test2);

            CHECK(test_called == false);
            CHECK(test_called_int == false);
            CHECK(test_called_float == true);
            CHECK(framework.messages == contains_substring("starting: how many lights <float>"));
            CHECK(framework.messages == contains_substring("finished: how many lights <float>"));
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

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("report FAIL_CHECK regular", "[registry]") {
    mock_framework framework;

    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() {
        // clang-format off
        failure_line = __LINE__; SNITCH_FAIL_CHECK("there are four lights");
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

TEST_CASE("report FAIL_CHECK template", "[registry]") {
    mock_framework framework;

    framework.registry.add_with_types<int>(
        "how many lights", "[tag]", {__FILE__, __LINE__}, []<typename TestType>() {
            // clang-format off
        failure_line = __LINE__; SNITCH_FAIL_CHECK("there are four lights");
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

TEST_CASE("report FAIL_CHECK section", "[registry]") {
    mock_framework framework;

    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() {
        SNITCH_SECTION("ask nicely") {
            // clang-format off
            failure_line = __LINE__; SNITCH_FAIL_CHECK("there are four lights");
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

TEST_CASE("report FAIL_CHECK capture", "[registry]") {
    mock_framework framework;

    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() {
        int number_of_lights = 3;
        SNITCH_CAPTURE(number_of_lights);
        // clang-format off
        failure_line = __LINE__; SNITCH_FAIL_CHECK("there are four lights");
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

TEST_CASE("report CHECK", "[registry]") {
    mock_framework framework;

    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() {
        int number_of_lights = 4;
        // clang-format off
        failure_line = __LINE__; SNITCH_CHECK(number_of_lights == 3);
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

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, failure_line);
        CHECK(failure.message == contains_substring("number_of_lights == 3"));
        CHECK(failure.message == contains_substring("4 != 3"));
    }
}

TEST_CASE("report CHECK success", "[registry]") {
    mock_framework framework;
    framework.catch_success = true;

    framework.registry.add({"how many fingers", "[tag]"}, {__FILE__, __LINE__}, []() {
        int number_of_fingers = 5;
        // clang-format off
        failure_line = __LINE__; SNITCH_CHECK(number_of_fingers == 5);
        // clang-format on
    });

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many fingers"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("number_of_fingers == 5"));
#if SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS
        CHECK(framework.messages == contains_substring("5 == 5"));
#else
        CHECK(framework.messages != contains_substring("5 == 5"));
#endif
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_successes() == 2u);
        auto success_opt = framework.get_success_event(0u);
        REQUIRE(success_opt.has_value());
        const auto& success = success_opt.value();
        CHECK_EVENT_TEST_ID(success, test.id);
        CHECK_EVENT_LOCATION(success, __FILE__, failure_line);
        CHECK(success.message == contains_substring("number_of_fingers == 5"));
#if SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS
        CHECK(success.message == contains_substring("5 == 5"));
#else
        CHECK(success.message != contains_substring("5 == 5"));
#endif
    }
}

#if SNITCH_WITH_EXCEPTIONS
TEST_CASE("report REQUIRE_THROWS_AS", "[registry]") {
    mock_framework framework;

    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() {
        auto ask_how_many_lights = [] { throw std::runtime_error{"there are four lights"}; };
        // clang-format off
        failure_line = __LINE__; SNITCH_REQUIRE_THROWS_AS(ask_how_many_lights(), std::logic_error);
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

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, failure_line);
        CHECK(
            failure.message ==
            contains_substring("std::logic_error expected but other std::exception thrown"));
        CHECK(failure.message == contains_substring("there are four lights"));
    }
}

TEST_CASE("report unexpected std::exception", "[registry]") {
    mock_framework framework;

    const std::size_t test_line = __LINE__;
    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, test_line}, []() {
        throw std::runtime_error("error message");
    });

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(
            framework.messages ==
            contains_substring("unexpected std::exception caught; message: error message"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, test_line);
        CHECK(
            failure.message ==
            contains_substring("unexpected std::exception caught; message: error message"));
    }
}

TEST_CASE("report unexpected unknown exception", "[registry]") {
    mock_framework framework;

    const std::size_t test_line = __LINE__;
    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, test_line}, []() { throw 42; });

    auto& test = *framework.registry.begin();

    SECTION("default reporter") {
        framework.setup_print();
        framework.registry.run(test);

        CHECK(framework.messages == contains_substring("how many lights"));
        CHECK(framework.messages == contains_substring("registry.cpp"));
        CHECK(framework.messages == contains_substring("unexpected unknown exception caught"));
    }

    SECTION("custom reporter") {
        framework.setup_reporter();
        framework.registry.run(test);

        REQUIRE(framework.get_num_failures() == 1u);
        auto failure_opt = framework.get_failure_event(0u);
        REQUIRE(failure_opt.has_value());
        const auto& failure = failure_opt.value();
        CHECK_EVENT_TEST_ID(failure, test.id);
        CHECK_EVENT_LOCATION(failure, __FILE__, test_line);
        CHECK(failure.message == contains_substring("unexpected unknown exception caught"));
    }
}
#endif

TEST_CASE("report SKIP", "[registry]") {
    mock_framework framework;

    framework.registry.add({"how many lights", "[tag]"}, {__FILE__, __LINE__}, []() {
        // clang-format off
        failure_line = __LINE__; SNITCH_SKIP_CHECK("there are four lights");
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

        REQUIRE(framework.get_num_skips() == 1u);
        auto skip_opt = framework.get_skip_event();
        REQUIRE(skip_opt.has_value());
        const auto& skip = skip_opt.value();
        CHECK_EVENT_TEST_ID(skip, test.id);
        CHECK_EVENT_LOCATION(skip, __FILE__, failure_line);
        CHECK(skip.message == contains_substring("there are four lights"));
    }
}

SNITCH_WARNING_POP

namespace {
void register_tests(mock_framework& framework) {
    test_called           = false;
    test_called_other_tag = false;
    test_called_skipped   = false;
    test_called_int       = false;
    test_called_float     = false;
    test_called_hidden1   = false;
    test_called_hidden2   = false;

    framework.registry.add(
        {"how are you", "[tag]"}, {__FILE__, __LINE__}, []() { test_called = true; });

    framework.registry.add({"how many lights", "[tag][other_tag]"}, {__FILE__, __LINE__}, []() {
        test_called_other_tag = true;
        SNITCH_FAIL_CHECK("there are four lights");
    });

    framework.registry.add({"drink from the cup", "[tag][skipped]"}, {__FILE__, __LINE__}, []() {
        test_called_skipped = true;
        SNITCH_SKIP_CHECK("not thirsty");
    });

    framework.registry.add_with_types<int, float>(
        "how many templated lights", "[tag][tag with spaces]", {__FILE__, __LINE__},
        []<typename T>() {
            if constexpr (std::is_same_v<T, int>) {
                test_called_int = true;
                SNITCH_FAIL_CHECK("there are four lights (int)");
            } else if constexpr (std::is_same_v<T, float>) {
                test_called_float = true;
                SNITCH_FAIL_CHECK("there are four lights (float)");
            }
        });

    framework.registry.add({"hidden test 1", "[.][hidden][other_tag]"}, {__FILE__, __LINE__}, []() {
        test_called_hidden1 = true;
    });

    framework.registry.add(
        {"hidden test 2", "[.hidden]"}, {__FILE__, __LINE__}, []() { test_called_hidden2 = true; });

    framework.registry.add(
        {"may fail that does not fail", "[.][may fail][!mayfail]"}, {__FILE__, __LINE__}, []() {});

    framework.registry.add(
        {"may fail that does fail", "[.][may fail][!mayfail]"}, {__FILE__, __LINE__},
        []() { SNITCH_FAIL_CHECK("it did fail"); });

    framework.registry.add(
        {"should fail that does not fail", "[.][should fail][!shouldfail]"}, {__FILE__, __LINE__},
        []() {});

    framework.registry.add(
        {"should fail that does fail", "[.][should fail][!shouldfail]"}, {__FILE__, __LINE__},
        []() { SNITCH_FAIL_CHECK("it did fail"); });

    framework.registry.add(
        {"may+should fail that does not fail", "[.][may+should fail][!mayfail][!shouldfail]"},
        {__FILE__, __LINE__}, []() {});

    framework.registry.add(
        {"may+should fail that does fail", "[.][may+should fail][!mayfail][!shouldfail]"},
        {__FILE__, __LINE__}, []() { SNITCH_FAIL_CHECK("it did fail"); });
}
} // namespace

TEST_CASE("run tests", "[registry]") {
    mock_framework framework;
    register_tests(framework);

    const auto run_selected_tests = [&](std::string_view filter, bool tags) {
        const snitch::small_vector<std::string_view, 1> filter_strings = {filter};
        const auto filter_function = [&](const snitch::test_id& id) noexcept {
            return (tags ? snitch::is_filter_match_tags(id.tags, filter)
                         : snitch::is_filter_match_name(id.name, filter)) ==
                   snitch::filter_result::included;
        };
        framework.registry.run_selected_tests("test_app", filter_strings, filter_function);
    };

    for (auto r : {reporter::print, reporter::custom}) {
        if (r == reporter::print) {
            framework.setup_print();
        } else {
            framework.setup_reporter();
        }

        INFO((r == reporter::print ? "default reporter" : "custom reporter"));

        SECTION("run tests") {
            framework.registry.run_tests("test_app");

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
                    contains_substring("some tests failed (3 out of 5 test cases, 7 assertions, 1 "
                                       "test cases skipped"));
            } else {
                CHECK(framework.get_num_runs() == 5u);
                CHECK_RUN(false, 5u, 3u, 0u, 1u, 7u, 3u, 0u);
            }
        }

        SECTION("run tests filtered all pass") {
            run_selected_tests("*are you", false);

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
                    contains_substring("all tests passed (1 test cases, 1 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 1u);
                CHECK_RUN(true, 1u, 0u, 0u, 0u, 1u, 0u, 0u);
            }
        }

        SECTION("run tests filtered all failed") {
            run_selected_tests("*lights*", false);

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
                    contains_substring("some tests failed (3 out of 3 test cases, 6 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 3u);
                CHECK_RUN(false, 3u, 3u, 0u, 0u, 6u, 3u, 0u);
            }
        }

        SECTION("run tests filtered all skipped") {
            run_selected_tests("*cup", false);

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
                                       "test cases skipped"));
            } else {
                CHECK(framework.get_num_runs() == 1u);
                CHECK_RUN(true, 1u, 0u, 0u, 1u, 0u, 0u, 0u);
            }
        }

        SECTION("run tests filtered tags") {
            run_selected_tests("[other_tag]", true);

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
                    contains_substring("some tests failed (1 out of 2 test cases, 3 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(false, 2u, 1u, 0u, 0u, 3u, 1u, 0u);
            }
        }

        SECTION("run tests filtered tags wildcard") {
            run_selected_tests("*tag]", true);

            CHECK(test_called);
            CHECK(test_called_other_tag);
            CHECK(test_called_skipped);
            CHECK(test_called_int);
            CHECK(test_called_float);
            CHECK(test_called_hidden1);
            CHECK(!test_called_hidden2);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("some tests failed (3 out of 6 test cases, 8 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 6u);
                CHECK_RUN(false, 6u, 3u, 0u, 1u, 8u, 3u, 0u);
            }
        }

        SECTION("run tests special tag [.]") {
            run_selected_tests("[hidden]", true);

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
                    contains_substring("all tests passed (2 test cases, 2 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(true, 2u, 0u, 0u, 0u, 2u, 0u, 0u);
            }
        }

        SECTION("run tests special tag [!mayfail]") {
            run_selected_tests("[may fail]", true);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (2 test cases, 3 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(true, 2u, 0u, 1u, 0u, 3u, 0u, 1u);
            }
        }

        SECTION("run tests special tag [!shouldfail]") {
            run_selected_tests("[should fail]", true);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("some tests failed (1 out of 2 test cases, 5 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(false, 2u, 1u, 1u, 0u, 5u, 1u, 1u);
            }
        }

        SECTION("run tests special tag [!shouldfail][!mayfail]") {
            run_selected_tests("[may+should fail]", true);

            if (r == reporter::print) {
                CHECK(
                    framework.messages ==
                    contains_substring("all tests passed (2 test cases, 5 assertions"));
            } else {
                CHECK(framework.get_num_runs() == 2u);
                CHECK_RUN(true, 2u, 0u, 2u, 0u, 5u, 0u, 2u);
            }
        }
    }
}

TEST_CASE("list tests", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    console_output_catcher console;

    SECTION("list_all_tests") {
        framework.registry.list_all_tests();

        CHECK(console.messages == contains_substring("how are you"));
        CHECK(console.messages == contains_substring("how many lights"));
        CHECK(console.messages == contains_substring("drink from the cup"));
        CHECK(console.messages == contains_substring("how many templated lights <int>"));
        CHECK(console.messages == contains_substring("how many templated lights <float>"));
        CHECK(console.messages == contains_substring("hidden test 1"));
        CHECK(console.messages == contains_substring("hidden test 2"));
    }

    SECTION("list_all_tags") {
        framework.registry.list_all_tags();

        CHECK(console.messages == contains_substring("[tag]"));
        CHECK(console.messages == contains_substring("[skipped]"));
        CHECK(console.messages == contains_substring("[other_tag]"));
        CHECK(console.messages == contains_substring("[tag with spaces]"));
        CHECK(console.messages == contains_substring("[hidden]"));
        CHECK(console.messages == contains_substring("[.]"));
        CHECK(console.messages != contains_substring("[.hidden]"));
        CHECK(console.messages == contains_substring("[!shouldfail]"));
        CHECK(console.messages == contains_substring("[!mayfail]"));
    }

    SECTION("list_tests_with_tag") {
        for (auto tag :
             {"[tag]"sv, "[other_tag]"sv, "[skipped]"sv, "[tag with spaces]"sv, "[wrong_tag]"sv,
              "[hidden]"sv, "[.]"sv, "[.hidden]"sv, "*tag]"sv}) {

            CAPTURE(tag);
            console.messages.clear();

            framework.registry.list_tests_with_tag(tag);
            if (tag == "[tag]"sv) {
                CHECK(console.messages == contains_substring("how are you"));
                CHECK(console.messages == contains_substring("how many lights"));
                CHECK(console.messages == contains_substring("drink from the cup"));
                CHECK(console.messages == contains_substring("how many templated lights <int>"));
                CHECK(console.messages == contains_substring("how many templated lights <float>"));
            } else if (tag == "[other_tag]"sv) {
                CHECK(console.messages != contains_substring("how are you"));
                CHECK(console.messages == contains_substring("how many lights"));
                CHECK(console.messages != contains_substring("drink from the cup"));
                CHECK(console.messages != contains_substring("how many templated lights <int>"));
                CHECK(console.messages != contains_substring("how many templated lights <float>"));
            } else if (tag == "[skipped]"sv) {
                CHECK(console.messages != contains_substring("how are you"));
                CHECK(console.messages != contains_substring("how many lights"));
                CHECK(console.messages == contains_substring("drink from the cup"));
                CHECK(console.messages != contains_substring("how many templated lights <int>"));
                CHECK(console.messages != contains_substring("how many templated lights <float>"));
            } else if (tag == "[tag with spaces]"sv) {
                CHECK(console.messages != contains_substring("how are you"));
                CHECK(console.messages != contains_substring("how many lights"));
                CHECK(console.messages != contains_substring("drink from the cup"));
                CHECK(console.messages == contains_substring("how many templated lights <int>"));
                CHECK(console.messages == contains_substring("how many templated lights <float>"));
            } else if (tag == "[hidden]"sv || tag == "[.]"sv) {
                CHECK(console.messages == contains_substring("hidden test 1"));
                CHECK(console.messages == contains_substring("hidden test 2"));
            } else if (tag == "*tag]"sv) {
                CHECK(console.messages == contains_substring("how are you"));
                CHECK(console.messages == contains_substring("how many lights"));
                CHECK(console.messages == contains_substring("drink from the cup"));
                CHECK(console.messages == contains_substring("how many templated lights"));
                CHECK(console.messages == contains_substring("hidden test 1"));
            } else if (tag == "[wrong_tag]"sv || tag == "[.hidden]"sv) {
                CHECK(console.messages.empty());
            }
        }
    }
}

TEST_CASE("configure", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    console_output_catcher console;

    SECTION("color = always") {
        const arg_vector args = {"test", "--color", "always"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.with_color == true);
    }

    SECTION("color = never") {
        const arg_vector args = {"test", "--color", "never"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.with_color == false);
    }

    SECTION("color = bad") {
        const arg_vector args = {"test", "--color", "bad"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(console.messages == contains_substring("unknown color directive"));
    }

    SECTION("verbosity = quiet") {
        const arg_vector args = {"test", "--verbosity", "quiet"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snitch::registry::verbosity::quiet);
    }

    SECTION("verbosity = normal") {
        const arg_vector args = {"test", "--verbosity", "normal"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snitch::registry::verbosity::normal);
    }

    SECTION("verbosity = high") {
        const arg_vector args = {"test", "--verbosity", "high"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snitch::registry::verbosity::high);
    }

    SECTION("verbosity = full") {
        const arg_vector args = {"test", "--verbosity", "full"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(framework.registry.verbose == snitch::registry::verbosity::full);
    }

    SECTION("verbosity = bad") {
        const arg_vector args = {"test", "--verbosity", "bad"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(console.messages == contains_substring("unknown verbosity level"));
    }

    SECTION("reporter = console (no option)") {
        const arg_vector args = {"test", "--reporter", "console"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(console.messages != contains_substring("error"));
    }

    SECTION("reporter = console (with option)") {
        const arg_vector args = {"test", "--reporter", "console::color=never"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.with_color = true;
        framework.registry.configure(*input);

        CHECK(framework.registry.with_color == false);
    }

    SECTION("reporter = console (unknown option)") {
        const arg_vector args = {"test", "--reporter", "console::abcd=never"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(console.messages == contains_substring("unknown reporter option 'abcd'"));
    }

    SECTION("reporter = console (bad: missing value)") {
        const arg_vector args = {"test", "--reporter", "console::abcdnever"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(
            console.messages ==
            contains_substring(
                "badly formatted reporter option 'abcdnever'; expected 'key=value'"));
    }

    SECTION("reporter = console (bad: empty option)") {
        const arg_vector args = {"test", "--reporter", "console::=value"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(
            console.messages ==
            contains_substring("badly formatted reporter option '=value'; expected 'key=value'"));
    }

    SECTION("reporter = console (bad: only equal)") {
        const arg_vector args = {"test", "--reporter", "console::="};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(
            console.messages ==
            contains_substring("badly formatted reporter option '='; expected 'key=value'"));
    }

    SECTION("reporter = bad colons") {
        for (const auto& args :
             {arg_vector{"test", "--reporter", ""}, arg_vector{"test", "--reporter", ":"},
              arg_vector{"test", "--reporter", "::"}, arg_vector{"test", "--reporter", ":::"},
              arg_vector{"test", "--reporter", "::::"}}) {
            CAPTURE(args[2]);
            console.messages.clear();

            auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
            framework.registry.configure(*input);

            CHECK(console.messages == contains_substring("invalid reporter"));
        }
    }
}

TEST_CASE("run tests cli", "[registry]") {
    mock_framework framework;
    framework.setup_reporter_and_print();
    register_tests(framework);
    console_output_catcher console;

    SECTION("no argument") {
        const arg_vector args = {"test"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(false, 5u, 3u, 0u, 1u, 7u, 3u, 0u);
    }

    SECTION("--help") {
        const arg_vector args = {"test", "--help"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(console.messages == contains_substring("test [options...]"));
    }

    SECTION("--list-tests") {
        const arg_vector args = {"test", "--list-tests"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(console.messages == contains_substring("how are you"));
        CHECK(console.messages == contains_substring("how many lights"));
        CHECK(console.messages == contains_substring("drink from the cup"));
        CHECK(console.messages == contains_substring("how many templated lights <int>"));
        CHECK(console.messages == contains_substring("how many templated lights <float>"));
    }

    SECTION("--list-tags") {
        const arg_vector args = {"test", "--list-tags"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(console.messages == contains_substring("[tag]"));
        CHECK(console.messages == contains_substring("[skipped]"));
        CHECK(console.messages == contains_substring("[other_tag]"));
        CHECK(console.messages == contains_substring("[tag with spaces]"));
    }

    SECTION("--list-tests-with-tag") {
        const arg_vector args = {"test", "--list-tests-with-tag", "[other_tag]"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(console.messages != contains_substring("how are you"));
        CHECK(console.messages == contains_substring("how many lights"));
        CHECK(console.messages != contains_substring("drink from the cup"));
        CHECK(console.messages != contains_substring("how many templated lights <int>"));
        CHECK(console.messages != contains_substring("how many templated lights <float>"));
    }

    SECTION("--list-reporters") {
        const arg_vector args = {"test", "--list-reporters"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

        SECTION("default") {
            framework.registry.run_tests(*input);

            CHECK(framework.events.empty());
            CHECK(console.messages == contains_substring("console"));
            CHECK(console.messages != contains_substring("custom"));
        }

        SECTION("with custom reporter") {
            framework.registry.add_reporter(
                "custom", {}, {},
                [](const snitch::registry&, const snitch::event::data&) noexcept {}, {});

            framework.registry.run_tests(*input);

            CHECK(framework.events.empty());
            CHECK(console.messages == contains_substring("console"));
            CHECK(console.messages == contains_substring("custom"));
        }
    }

    SECTION("test filter") {
        const arg_vector args = {"test", "how many*"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(false, 3u, 3u, 0u, 0u, 6u, 3u, 0u);
    }

    SECTION("test filter exclusion") {
        const arg_vector args = {"test", "~*fail"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(false, 7u, 3u, 0u, 1u, 9u, 3u, 0u);
    }

    SECTION("test tag filter") {
        const arg_vector args = {"test", "[skipped]"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.run_tests(*input);

        CHECK_RUN(true, 1u, 0u, 0u, 1u, 0u, 0u, 0u);
    }
}

std::array<bool, 7> readme_test_called = {false};

TEST_CASE("run tests cli readme example", "[registry]") {
    mock_framework framework;
    framework.setup_reporter_and_print();
    console_output_catcher console;

    readme_test_called = {false};

    framework.registry.add({"a"}, {__FILE__, __LINE__}, []() { readme_test_called[0] = true; });
    framework.registry.add({"b"}, {__FILE__, __LINE__}, []() { readme_test_called[1] = true; });
    framework.registry.add({"c"}, {__FILE__, __LINE__}, []() { readme_test_called[2] = true; });
    framework.registry.add({"d"}, {__FILE__, __LINE__}, []() { readme_test_called[3] = true; });
    framework.registry.add({"abc"}, {__FILE__, __LINE__}, []() { readme_test_called[4] = true; });
    framework.registry.add({"abd"}, {__FILE__, __LINE__}, []() { readme_test_called[5] = true; });
    framework.registry.add({"abcd"}, {__FILE__, __LINE__}, []() { readme_test_called[6] = true; });

    const arg_vector args = {"test", "a*", "~*d", "abcd"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
    framework.registry.run_tests(*input);

    std::array<bool, 7> expected = {true, false, false, false, true, false, true};
    CHECK(readme_test_called == expected);
}
