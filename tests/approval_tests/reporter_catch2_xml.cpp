#include "testing.hpp"
#include "testing_assertions.hpp"
#include "testing_event.hpp"
#include "testing_reporters.hpp"

#include <stdexcept>

#if SNITCH_WITH_CATCH2_XML_REPORTER || SNITCH_WITH_ALL_REPORTERS

using namespace std::literals;
using snitch::matchers::contains_substring;

TEST_CASE("xml reporter", "[reporters]") {
    mock_framework framework;
    register_tests_for_reporters(framework.registry);
    framework.registry.add({"test escape <>&\"'"}, SNITCH_CURRENT_LOCATION, [] {
        SNITCH_FAIL("escape <>&\"' in messages");
    });
    framework.registry.add({"test escape very long"}, SNITCH_CURRENT_LOCATION, [] {
        SNITCH_FAIL(std::string(2 * snitch::max_message_length, '&'));
    });

    std::optional<snitch::reporter::catch2_xml::reporter> reporter;
    auto init      = [&](snitch::registry& r) { reporter.emplace(r); };
    auto configure = [&](snitch::registry& r, std::string_view k, std::string_view v) noexcept {
        return reporter.value().configure(r, k, v);
    };
    auto report = [&](const snitch::registry& r, const snitch::event::data& e) noexcept {
        return reporter.value().report(r, e);
    };
    auto finish = [&](snitch::registry&) noexcept { reporter.reset(); };

    framework.registry.add_reporter("xml", init, configure, report, finish);

    constexpr const char* reporter_name = "xml";
#    define REPORTER_PREFIX "reporter_catch2_xml_"

    const std::vector<std::regex> ignores = {
        std::regex{R"|(durationInSeconds="([0-9.e+\-]{12})")|"},
        std::regex{R"(catch2-version="([0-9]+\.[0-9]+\.[0-9]+\.[0-9a-z]+).snitch)"},
        std::regex{R"(filename="(.+/snitch/tests/approval_tests/))"},
        std::regex{R"(filename="(.+/snitch/tests/))"},
        std::regex{R"(<File>(.+/snitch/tests/approval_tests/))"},
        std::regex{R"(<File>(.+/snitch/tests/))"},
        std::regex{R"(filename="(.+\\snitch\\tests\\approval_tests\\))"},
        std::regex{R"(filename="(.+\\snitch\\tests\\))"},
        std::regex{R"(<File>(.+\\snitch\\tests\\approval_tests\\))"},
        std::regex{R"(<File>(.+\\snitch\\tests\\))"},
        std::regex{R"|(line="([0-9]+)")|"},
        std::regex{R"(<Line>([0-9]+))"}};

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
