#include "testing.hpp"
#include "testing_assertions.hpp"
#include "testing_event.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace std::literals;
using snitch::matchers::contains_substring;

namespace {
bool test_called           = false;
bool test_called_other_tag = false;
bool test_called_skipped   = false;
bool test_called_int       = false;
bool test_called_float     = false;
bool test_called_hidden1   = false;
bool test_called_hidden2   = false;
} // namespace

TEST_CASE("add regular test", "[registry]") {
    mock_framework framework;

    test_called = false;
    framework.registry.add(
        {"how many lights", "[tag]"}, SNITCH_CURRENT_LOCATION, []() { test_called = true; });

    REQUIRE(framework.get_num_registered_tests() == 1u);

    auto& test = framework.registry.test_cases()[0];
    CHECK(test.id.name == "how many lights"sv);
    CHECK(test.id.tags == "[tag]"sv);
    CHECK(test.id.type == ""sv);
    REQUIRE(test.func != nullptr);

    framework.setup_reporter();
    framework.registry.run(test);

    CHECK(test_called == true);
    REQUIRE(framework.events.size() == 2u);
    CHECK(framework.is_event<owning_event::test_case_started>(0));
    CHECK(framework.is_event<owning_event::test_case_ended>(1));
    CHECK_EVENT_TEST_ID(framework.events[0], test.id);
    CHECK_EVENT_TEST_ID(framework.events[1], test.id);
}

TEST_CASE("add regular test no tags", "[registry]") {
    mock_framework framework;

    test_called = false;
    framework.registry.add(
        {"how many lights"}, SNITCH_CURRENT_LOCATION, []() { test_called = true; });

    REQUIRE(framework.get_num_registered_tests() == 1u);

    auto& test = framework.registry.test_cases()[0];
    CHECK(test.id.name == "how many lights"sv);
    CHECK(test.id.tags == ""sv);
    CHECK(test.id.type == ""sv);
    REQUIRE(test.func != nullptr);

    framework.setup_reporter();
    framework.registry.run(test);

    CHECK(test_called == true);
    REQUIRE(framework.events.size() == 2u);
    CHECK(framework.is_event<owning_event::test_case_started>(0u));
    CHECK(framework.is_event<owning_event::test_case_ended>(1u));
    CHECK_EVENT_TEST_ID(framework.events[0], test.id);
    CHECK_EVENT_TEST_ID(framework.events[1], test.id);
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
                {"how many lights", "[tag]"}, SNITCH_CURRENT_LOCATION, []<typename T>() {
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
                {"how many lights", "[tag]"}, SNITCH_CURRENT_LOCATION, []<typename T>() {
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

        auto& test1 = framework.registry.test_cases()[0];
        CHECK(test1.id.name == "how many lights"sv);
        CHECK(test1.id.tags == "[tag]"sv);
        CHECK(test1.id.type == "int"sv);
        REQUIRE(test1.func != nullptr);

        auto& test2 = framework.registry.test_cases()[1];
        CHECK(test2.id.name == "how many lights"sv);
        CHECK(test2.id.tags == "[tag]"sv);
        CHECK(test2.id.type == "float"sv);
        REQUIRE(test2.func != nullptr);

        framework.setup_reporter();

        SECTION("run int") {
            framework.registry.run(test1);

            CHECK(test_called == false);
            CHECK(test_called_int == true);
            CHECK(test_called_float == false);
            REQUIRE(framework.events.size() == 2u);
            CHECK(framework.is_event<owning_event::test_case_started>(0));
            CHECK(framework.is_event<owning_event::test_case_ended>(1));
            CHECK_EVENT_TEST_ID(framework.events[0], test1.id);
            CHECK_EVENT_TEST_ID(framework.events[1], test1.id);
        }

        SECTION("run float") {
            framework.registry.run(test2);

            CHECK(test_called == false);
            CHECK(test_called_int == false);
            CHECK(test_called_float == true);
            REQUIRE(framework.events.size() == 2u);
            CHECK(framework.is_event<owning_event::test_case_started>(0));
            CHECK(framework.is_event<owning_event::test_case_ended>(1));
            CHECK_EVENT_TEST_ID(framework.events[0], test2.id);
            CHECK_EVENT_TEST_ID(framework.events[1], test2.id);
        }
    }
}

namespace { namespace my_reporter {
bool init_called      = false;
bool configure_result = true;
bool configure_called = false;
bool report_called    = false;
bool finish_called    = false;

void init(snitch::registry&) noexcept {
    init_called = true;
}
bool configure(snitch::registry&, std::string_view, std::string_view) noexcept {
    configure_called = true;
    return configure_result;
}
void report(const snitch::registry&, const snitch::event::data&) noexcept {
    report_called = true;
}
void finish(snitch::registry&) noexcept {
    finish_called = true;
}

void register_one_test(snitch::registry& r) {
    r.add({"the test", "[tag]"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_CHECK(1 == 2); });
}
}} // namespace ::my_reporter

TEST_CASE("add reporter", "[registry]") {
    mock_framework         framework;
    console_output_catcher console;
    my_reporter::register_one_test(framework.registry);

    my_reporter::init_called      = false;
    my_reporter::configure_result = true;
    my_reporter::configure_called = false;
    my_reporter::report_called    = false;
    my_reporter::finish_called    = false;

    SECTION("full") {
        framework.registry.add_reporter(
            "custom", &my_reporter::init, &my_reporter::configure, &my_reporter::report,
            &my_reporter::finish);

        const arg_vector args = {"test", "--reporter", "custom::arg=value"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(my_reporter::init_called);
        CHECK(my_reporter::configure_called);
        CHECK(!my_reporter::report_called);
        CHECK(!my_reporter::finish_called);

        framework.registry.run_tests(*input);

        CHECK(my_reporter::report_called);
        CHECK(my_reporter::finish_called);
    }

    SECTION("no init") {
        framework.registry.add_reporter(
            "custom", {}, &my_reporter::configure, &my_reporter::report, &my_reporter::finish);

        const arg_vector args = {"test", "--reporter", "custom::arg=value"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(!my_reporter::init_called);
        CHECK(my_reporter::configure_called);
        CHECK(!my_reporter::report_called);
        CHECK(!my_reporter::finish_called);

        framework.registry.run_tests(*input);

        CHECK(my_reporter::report_called);
        CHECK(my_reporter::finish_called);
    }

    SECTION("no config") {
        framework.registry.add_reporter(
            "custom", &my_reporter::init, {}, &my_reporter::report, &my_reporter::finish);

        const arg_vector args = {"test", "--reporter", "custom::arg=value"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(my_reporter::init_called);
        CHECK(!my_reporter::configure_called);
        CHECK(console.messages == contains_substring("unknown reporter option 'arg'"));
        CHECK(!my_reporter::report_called);
        CHECK(!my_reporter::finish_called);

        framework.registry.run_tests(*input);

        CHECK(my_reporter::report_called);
        CHECK(my_reporter::finish_called);
    }

    SECTION("no finish") {
        framework.registry.add_reporter(
            "custom", &my_reporter::init, &my_reporter::configure, &my_reporter::report, {});

        const arg_vector args = {"test", "--reporter", "custom::arg=value"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(my_reporter::init_called);
        CHECK(my_reporter::configure_called);
        CHECK(!my_reporter::report_called);
        CHECK(!my_reporter::finish_called);

        framework.registry.run_tests(*input);

        CHECK(my_reporter::report_called);
        CHECK(!my_reporter::finish_called);
    }

#if SNITCH_WITH_EXCEPTIONS
    SECTION("max number reached") {
        assertion_exception_enabler enabler;

        std::array<snitch::small_string<32>, snitch::max_registered_reporters> names = {};
        for (std::size_t i = framework.registry.reporters().size();
             i < snitch::max_registered_reporters; ++i) {
            append_or_truncate(names[i], "dummy", i);
            framework.registry.add_reporter(names[i], {}, {}, &my_reporter::report, {});
        }

        CHECK_THROWS_WHAT(
            framework.registry.add_reporter("toomuch", {}, {}, &my_reporter::report, {}),
            assertion_exception, "max number of reporters reached");
        CHECK(
            console.messages ==
            contains_substring("max number of reporters reached; "
                               "please increase 'SNITCH_MAX_REGISTERED_REPORTERS'"));
    }

    SECTION("bad name") {
        assertion_exception_enabler enabler;

        CHECK_THROWS_WHAT(
            framework.registry.add_reporter("bad::name", {}, {}, &my_reporter::report, {}),
            assertion_exception, "invalid reporter name");
        CHECK(
            console.messages == contains_substring("reporter name cannot contains '::' "
                                                   "(trying to register 'bad::name')"));
    }
#endif
}

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
        {"how are you", "[tag]"}, SNITCH_CURRENT_LOCATION, []() { test_called = true; });

    framework.registry.add({"how many lights", "[tag][other_tag]"}, SNITCH_CURRENT_LOCATION, []() {
        test_called_other_tag = true;
        SNITCH_FAIL_CHECK("there are four lights");
    });

    framework.registry.add({"drink from the cup", "[tag][skipped]"}, SNITCH_CURRENT_LOCATION, []() {
        test_called_skipped = true;
        SNITCH_SKIP_CHECK("not thirsty");
    });

    framework.registry.add_with_types<int, float>(
        {"how many templated lights", "[tag][tag with spaces]"}, SNITCH_CURRENT_LOCATION,
        []<typename T>() {
            if constexpr (std::is_same_v<T, int>) {
                test_called_int = true;
                SNITCH_FAIL_CHECK("there are four lights (int)");
            } else if constexpr (std::is_same_v<T, float>) {
                test_called_float = true;
                SNITCH_FAIL_CHECK("there are four lights (float)");
            }
        });

    framework.registry.add(
        {"hidden test 1", "[.][hidden][other_tag]"}, SNITCH_CURRENT_LOCATION,
        []() { test_called_hidden1 = true; });

    framework.registry.add({"hidden test 2", "[.hidden]"}, SNITCH_CURRENT_LOCATION, []() {
        test_called_hidden2 = true;
    });

    framework.registry.add(
        {"may fail that does not fail", "[.][may fail][!mayfail]"}, SNITCH_CURRENT_LOCATION,
        []() {});

    framework.registry.add(
        {"may fail that does fail", "[.][may fail][!mayfail]"}, SNITCH_CURRENT_LOCATION,
        []() { SNITCH_FAIL_CHECK("it did fail"); });

    framework.registry.add(
        {"should fail that does not fail", "[.][should fail][!shouldfail]"},
        SNITCH_CURRENT_LOCATION, []() {});

    framework.registry.add(
        {"should fail that does fail", "[.][should fail][!shouldfail]"}, SNITCH_CURRENT_LOCATION,
        []() { SNITCH_FAIL_CHECK("it did fail"); });

    framework.registry.add(
        {"may+should fail that does not fail", "[.][may+should fail][!mayfail][!shouldfail]"},
        SNITCH_CURRENT_LOCATION, []() {});

    framework.registry.add(
        {"may+should fail that does fail", "[.][may+should fail][!mayfail][!shouldfail]"},
        SNITCH_CURRENT_LOCATION, []() { SNITCH_FAIL_CHECK("it did fail"); });
}
} // namespace

TEST_CASE("run tests", "[registry]") {
    mock_framework framework;
    register_tests(framework);

    const auto run_selected_tests = [&](std::string_view filter, bool tags) {
        const snitch::small_vector<std::string_view, 1> filter_strings = {filter};
        const auto filter_function = [&](const snitch::test_id& id) noexcept {
            return tags ? snitch::is_filter_match_tags(id.tags, filter).included
                        : snitch::is_filter_match_name(id.name, filter).included;
        };
        framework.registry.run_selected_tests("test_app", filter_strings, filter_function);
    };

    framework.setup_reporter();

    SECTION("run tests") {
        framework.registry.run_tests("test_app");

        CHECK(test_called);
        CHECK(test_called_other_tag);
        CHECK(test_called_skipped);
        CHECK(test_called_int);
        CHECK(test_called_float);
        CHECK(!test_called_hidden1);
        CHECK(!test_called_hidden2);

        CHECK(framework.get_num_runs() == 5u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 5u, 3u, 0u, 1u, 7u, 3u, 0u);
#else
        CHECK_RUN(false, 5u, 3u, 0u, 1u, 3u, 3u, 0u);
#endif
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

        CHECK(framework.get_num_runs() == 1u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(true, 1u, 0u, 0u, 0u, 1u, 0u, 0u);
#else
        CHECK_RUN(true, 1u, 0u, 0u, 0u, 0u, 0u, 0u);
#endif
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

        CHECK(framework.get_num_runs() == 3u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 3u, 3u, 0u, 0u, 6u, 3u, 0u);
#else
        CHECK_RUN(false, 3u, 3u, 0u, 0u, 3u, 3u, 0u);
#endif
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

        CHECK(framework.get_num_runs() == 1u);
        CHECK_RUN(true, 1u, 0u, 0u, 1u, 0u, 0u, 0u);
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

        CHECK(framework.get_num_runs() == 2u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 2u, 1u, 0u, 0u, 3u, 1u, 0u);
#else
        CHECK_RUN(false, 2u, 1u, 0u, 0u, 1u, 1u, 0u);
#endif
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

        CHECK(framework.get_num_runs() == 6u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 6u, 3u, 0u, 1u, 8u, 3u, 0u);
#else
        CHECK_RUN(false, 6u, 3u, 0u, 1u, 3u, 3u, 0u);
#endif
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

        CHECK(framework.get_num_runs() == 2u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(true, 2u, 0u, 0u, 0u, 2u, 0u, 0u);
#else
        CHECK_RUN(true, 2u, 0u, 0u, 0u, 0u, 0u, 0u);
#endif
    }

    SECTION("run tests special tag [!mayfail]") {
        run_selected_tests("[may fail]", true);

        CHECK(framework.get_num_runs() == 2u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(true, 2u, 0u, 1u, 0u, 3u, 0u, 1u);
#else
        CHECK_RUN(true, 2u, 0u, 1u, 0u, 1u, 0u, 1u);
#endif
    }

    SECTION("run tests special tag [!shouldfail]") {
        run_selected_tests("[should fail]", true);

        CHECK(framework.get_num_runs() == 2u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 2u, 1u, 1u, 0u, 5u, 1u, 1u);
#else
        CHECK_RUN(false, 2u, 1u, 1u, 0u, 3u, 1u, 1u);
#endif
    }

    SECTION("run tests special tag [!shouldfail][!mayfail]") {
        run_selected_tests("[may+should fail]", true);

        CHECK(framework.get_num_runs() == 2u);
#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(true, 2u, 0u, 2u, 0u, 5u, 0u, 2u);
#else
        CHECK_RUN(true, 2u, 0u, 2u, 0u, 3u, 0u, 2u);
#endif
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
            CHECK(console.messages == contains_substring("Matching test cases:"));
            CHECK(console.messages == contains_substring("matching test cases"));
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
                const auto no_tests = "Matching test cases:\n0 matching test cases\n"sv;
                CHECK(console.messages == no_tests);
            }
        }
    }
}

TEST_CASE("configure color", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    console_output_catcher console;

    SECTION("color = always") {
        for (const auto& args :
             {arg_vector{"test", "--color", "always"},
              arg_vector{"test", "--colour-mode", "ansi"}}) {

            SECTION(args[2]) {
                auto input =
                    snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
                framework.registry.configure(*input);

                CHECK(framework.registry.with_color == true);
            }
        }
    }

    SECTION("color = never") {
        for (const auto& args :
             {arg_vector{"test", "--color", "never"},
              arg_vector{"test", "--colour-mode", "none"}}) {

            SECTION(args[2]) {
                auto input =
                    snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
                framework.registry.configure(*input);

                CHECK(framework.registry.with_color == false);
            }
        }
    }

    SECTION("color = default") {
        for (const auto& args :
             {arg_vector{"test", "--color", "default"},
              arg_vector{"test", "--colour-mode", "default"}}) {

            SECTION(args[2]) {
                bool prev = framework.registry.with_color;
                auto input =
                    snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
                framework.registry.configure(*input);

                CHECK(framework.registry.with_color == prev);
            }
        }
    }

    SECTION("color = bad") {
        for (const auto& args :
             {arg_vector{"test", "--color", "bad"}, arg_vector{"test", "--colour-mode", "bad"}}) {
            auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
            framework.registry.configure(*input);

            CHECK(console.messages == contains_substring("unknown color directive"));
        }
    }
}

TEST_CASE("configure verbosity", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    console_output_catcher console;

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
}

TEST_CASE("configure reporter", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    console_output_catcher console;

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

    SECTION("reporter = console (multiple options)") {
        const arg_vector args = {"test", "--reporter", "console::color=never::colour-mode=none"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.with_color = false;
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

            SECTION(args[2]) {
                auto input =
                    snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
                framework.registry.configure(*input);

                CHECK(console.messages == contains_substring("invalid reporter"));
            }
        }
    }

    SECTION("reporter = unknown") {
        const arg_vector args = {"test", "--reporter", "fantasio"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        CHECK(console.messages == contains_substring("unknown reporter 'fantasio', using default"));
    }
}

TEST_CASE("configure output", "[registry]") {
    mock_framework framework;
    register_tests(framework);
    console_output_catcher console;

    SECTION("valid") {
        const arg_vector args = {"test", "--out", "test_output.txt"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        CHECK(console.messages.empty());

        std::string line;
        {
            std::ifstream file("test_output.txt");
            std::getline(file, line);
        }

        CHECK(line == snitch::matchers::contains_substring{"starting test with snitch"});

        std::filesystem::remove("test_output.txt");
    }

#if SNITCH_WITH_EXCEPTIONS
    SECTION("bad path") {
        assertion_exception_enabler enabler;

        const arg_vector args = {"test", "--out", ""};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

        CHECK_THROWS_WHAT(
            framework.registry.configure(*input), assertion_exception,
            "output file could not be opened for writing");
    }
#endif
}

TEST_CASE("run tests cli", "[registry][cli]") {
    mock_framework framework;
    framework.setup_reporter();
    register_tests(framework);
    console_output_catcher console;

    SECTION("no argument") {
        const arg_vector args = {"test"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 5u, 3u, 0u, 1u, 7u, 3u, 0u);
#else
        CHECK_RUN(false, 5u, 3u, 0u, 1u, 3u, 3u, 0u);
#endif
    }
}

TEST_CASE("print help cli", "[registry][cli]") {
    mock_framework framework;
    framework.setup_reporter();
    register_tests(framework);
    console_output_catcher console;

    SECTION("--help") {
        const arg_vector args = {"test", "--help"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(framework.get_num_runs() == 0u);
        CHECK(console.messages == contains_substring("test [options...]"));
    }

    SECTION("--help no color") {
        const arg_vector args = {"test", "--help", "--color", "never"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        CHECK(!contains_color_codes(console.messages));
    }
}

TEST_CASE("list stuff cli", "[registry][cli]") {
    mock_framework framework;
    framework.setup_reporter();
    register_tests(framework);
    console_output_catcher console;

    SECTION("--list-tests") {
        const arg_vector args = {"test", "--list-tests"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        REQUIRE(framework.events.size() == 15u);
        CHECK(framework.get_num_runs() == 0u);
        CHECK(framework.get_num_listed_tests() == 13u);
        CHECK(framework.is_test_listed({"how are you", "[tag]"}));
        // Not testing all...
    }

    SECTION("--list-tests filtered") {
        const arg_vector args = {"test", "--list-tests", "how*"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        REQUIRE(framework.events.size() == 6u);
        CHECK(framework.get_num_runs() == 0u);
        CHECK(framework.get_num_listed_tests() == 4u);
        CHECK(framework.is_test_listed({"how are you", "[tag]"}));
        CHECK(framework.is_test_listed({"how many lights", "[tag][other_tag]"}));
        CHECK(framework.is_test_listed(
            {"how many templated lights", "[tag][tag with spaces]", "int"}));
        CHECK(framework.is_test_listed(
            {"how many templated lights", "[tag][tag with spaces]", "float"}));
    }

    SECTION("--list-tags") {
        const arg_vector args = {"test", "--list-tags"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        CHECK(framework.events.empty());
        CHECK(framework.get_num_runs() == 0u);
        CHECK(console.messages == contains_substring("[tag]"));
        CHECK(console.messages == contains_substring("[skipped]"));
        CHECK(console.messages == contains_substring("[other_tag]"));
        CHECK(console.messages == contains_substring("[tag with spaces]"));
    }

    SECTION("--list-tests-with-tag") {
        const arg_vector args = {"test", "--list-tests-with-tag", "[other_tag]"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        REQUIRE(framework.events.size() == 4u);
        CHECK(framework.get_num_runs() == 0u);
        CHECK(framework.get_num_listed_tests() == 2u);
        CHECK(framework.is_test_listed({"how many lights", "[tag][other_tag]"}));
        CHECK(framework.is_test_listed({"hidden test 1", "[.][hidden][other_tag]"}));
    }

    SECTION("--list-reporters") {
        const arg_vector args = {"test", "--list-reporters"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);

        SECTION("default") {
            framework.registry.run_tests(*input);

            CHECK(framework.events.empty());
            CHECK(framework.get_num_runs() == 0u);
            CHECK(console.messages == contains_substring("console"));
            CHECK(console.messages != contains_substring("custom"));
        }

        SECTION("with custom reporter") {
            framework.registry.add_reporter(
                "custom", {}, {},
                [](const snitch::registry&, const snitch::event::data&) noexcept {}, {});

            framework.registry.run_tests(*input);

            CHECK(framework.events.empty());
            CHECK(framework.get_num_runs() == 0u);
            CHECK(console.messages == contains_substring("console"));
            CHECK(console.messages == contains_substring("custom"));
        }
    }
}

TEST_CASE("run tests filtered cli", "[registry][cli]") {
    mock_framework framework;
    framework.setup_reporter();
    register_tests(framework);
    console_output_catcher console;

    SECTION("test filter") {
        const arg_vector args = {"test", "how many*"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 3u, 3u, 0u, 0u, 6u, 3u, 0u);
#else
        CHECK_RUN(false, 3u, 3u, 0u, 0u, 3u, 3u, 0u);
#endif
    }

    SECTION("test filter multiple AND") {
        const arg_vector args = {"test", "how many*", "*templated*"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 2u, 2u, 0u, 0u, 4u, 2u, 0u);
#else
        CHECK_RUN(false, 2u, 2u, 0u, 0u, 2u, 2u, 0u);
#endif
    }

    SECTION("test filter multiple OR") {
        const arg_vector args = {"test", "how many*,*are you"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 4u, 3u, 0u, 0u, 7u, 3u, 0u);
#else
        CHECK_RUN(false, 4u, 3u, 0u, 0u, 3u, 3u, 0u);
#endif
    }

    SECTION("test filter exclusion") {
        const arg_vector args = {"test", "~*fail"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 5u, 3u, 0u, 1u, 7u, 3u, 0u);
#else
        CHECK_RUN(false, 5u, 3u, 0u, 1u, 3u, 3u, 0u);
#endif
    }

    SECTION("test filter hidden") {
        const arg_vector args = {"test", "hidden test*"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(true, 2u, 0u, 0u, 0u, 2u, 0u, 0u);
#else
        CHECK_RUN(true, 2u, 0u, 0u, 0u, 0u, 0u, 0u);
#endif
    }

    SECTION("test filter tag") {
        const arg_vector args = {"test", "[skipped]"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

        CHECK_RUN(true, 1u, 0u, 0u, 1u, 0u, 0u, 0u);
    }

    SECTION("test filter multiple tags") {
        const arg_vector args = {"test", "[other_tag][tag]"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 1u, 1u, 0u, 0u, 2u, 1u, 0u);
#else
        CHECK_RUN(false, 1u, 1u, 0u, 0u, 1u, 1u, 0u);
#endif
    }

    SECTION("test filter tag AND name") {
        const arg_vector args = {"test", "[tag]", "*many lights"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 1u, 1u, 0u, 0u, 2u, 1u, 0u);
#else
        CHECK_RUN(false, 1u, 1u, 0u, 0u, 1u, 1u, 0u);
#endif
    }

    SECTION("test filter tag OR name") {
        const arg_vector args = {"test", "[other_tag],how are*"};
        auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        framework.registry.configure(*input);
        framework.registry.run_tests(*input);

#if SNITCH_WITH_EXCEPTIONS
        CHECK_RUN(false, 3u, 1u, 0u, 0u, 4u, 1u, 0u);
#else
        CHECK_RUN(false, 3u, 1u, 0u, 0u, 1u, 1u, 0u);
#endif
    }
}
