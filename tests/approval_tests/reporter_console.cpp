#include "testing.hpp"
#include "testing_assertions.hpp"
#include "testing_event.hpp"
#include "testing_reporters.hpp"

#include <iostream>
#include <stdexcept>

using namespace std::literals;
using snitch::matchers::contains_substring;

TEST_CASE("console reporter", "[reporters]") {
    mock_framework framework;
    register_tests_for_reporters(framework.registry);

    framework.registry.add_reporter<snitch::reporter::console::reporter>("console");

    framework.registry.with_color = false;

    constexpr const char* reporter_name = "console";
#define REPORTER_PREFIX "reporter_console_"

    const std::vector<std::regex> ignores = {
        std::regex{R"(, ([0-9.e+\-]{12}) seconds)"},
        std::regex{R"(snitch v([0-9]+\.[0-9]+\.[0-9]+\.[0-9a-z]+))"},
        std::regex{R"(at (.+/snitch/tests/approval_tests/).+:([0-9]+))"},
        std::regex{R"(at (.+/snitch/tests/).+:([0-9]+))"},
        std::regex{R"(at (.+\\snitch\\tests\\approval_tests\\).+:([0-9]+))"},
        std::regex{R"(at (.+\\snitch\\tests\\).+:([0-9]+))"},
        std::regex{R"(^finished: .+\(([0-9.e+\-]{12}s)\))"}};

    SECTION("default") {
        const arg_vector args{"test", "--reporter", reporter_name};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "default");
    }

    SECTION("no test") {
        const arg_vector args{"test", "--reporter", reporter_name, "bad_filter"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "notest");
    }

    SECTION("all pass") {
        const arg_vector args{"test", "--reporter", reporter_name, "* pass*"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "allpass");
    }

    SECTION("all fail") {
        const arg_vector args{"test", "--reporter", reporter_name, "* fail*"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "allfail");
    }

    SECTION("with color") {
        const arg_vector args{"test", "--reporter", "console::color=always"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "withcolor");
    }

    SECTION("full output") {
        const arg_vector args{"test", "--reporter", reporter_name, "--verbosity", "full"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "full");
    }

    SECTION("list tests") {
        const arg_vector args{"test", "--reporter", reporter_name, "--list-tests"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "list_tests");
    }
}
