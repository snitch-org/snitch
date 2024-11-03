#include "testing.hpp"
#include "testing_assertions.hpp"
#include "testing_event.hpp"
#include "testing_reporters.hpp"

#include <stdexcept>

#if SNITCH_WITH_TEAMCITY_REPORTER || SNITCH_WITH_ALL_REPORTERS

using namespace std::literals;
using snitch::matchers::contains_substring;

TEST_CASE("teamcity reporter", "[reporters]") {
    mock_framework framework;
    register_tests_for_reporters(framework.registry);
    framework.registry.add({"test escape |'\n\r[]"}, SNITCH_CURRENT_LOCATION, [] {
        SNITCH_FAIL("escape | message || | '\n\r[]");
    });
    framework.registry.add({"test escape very long"}, SNITCH_CURRENT_LOCATION, [] {
        SNITCH_FAIL(std::string(2 * snitch::max_message_length, '|'));
    });

    framework.registry.add_reporter(
        "teamcity", &snitch::reporter::teamcity::initialize, {},
        &snitch::reporter::teamcity::report, {});

    constexpr const char* reporter_name = "teamcity";
#    define REPORTER_PREFIX "reporter_teamcity_"

    const std::vector<std::regex> ignores = {
        std::regex{R"( duration='([0-9]+)')"},
        std::regex{R"( (?:message|out)='(.+/snitch/tests/approval_tests/).+:([0-9]+))"},
        std::regex{R"( (?:message|out)='(.+/snitch/tests/).+:([0-9]+))"},
        std::regex{R"( (?:message|out)='(.+\\snitch\\tests\\approval_tests\\).+:([0-9]+))"},
        std::regex{R"( (?:message|out)='(.+\\snitch\\tests\\).+:([0-9]+))"}};

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

    SECTION("full output") {
        const arg_vector args{"test", "--reporter", reporter_name, "--verbosity", "full"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "full");
    }

    SECTION("list tests") {
        const arg_vector args{"test", "--reporter", reporter_name, "--list-tests"};
        CHECK_FOR_DIFFERENCES(args, ignores, REPORTER_PREFIX "list_tests");
    }
}

#endif
