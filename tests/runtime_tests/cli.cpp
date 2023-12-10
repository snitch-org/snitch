#include "testing.hpp"
#include "testing_event.hpp"

using namespace std::literals;
using snitch::matchers::contains_substring;

TEST_CASE("parse arguments empty", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments empty .exe", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test.exe"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments empty .something.exe", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test.something.exe"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test.something"sv);
    CHECK(input->arguments.empty());
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments help (long form)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--help"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--help"sv);
    CHECK(!input->arguments[0].value.has_value());
    CHECK(!input->arguments[0].value_name.has_value());
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments help (short form)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "-h"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--help"sv);
    CHECK(!input->arguments[0].value.has_value());
    CHECK(!input->arguments[0].value_name.has_value());
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments help (duplicate)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--help", "--help"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(!input.has_value());
    CHECK(console.messages == contains_substring("duplicate command line argument '--help'"));
}

TEST_CASE("parse arguments verbosity (long form)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--verbosity", "high"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--verbosity"sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "high"sv);
    CHECK(input->arguments[0].value_name.value() == "quiet|normal|high|full"sv);
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments verbosity (short form)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "-v", "high"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--verbosity"sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "high"sv);
    CHECK(input->arguments[0].value_name.value() == "quiet|normal|high|full"sv);
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments verbosity (no value)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--verbosity"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    CHECK(!input.has_value());
    CHECK(
        console.messages ==
        contains_substring(
            "missing value '<quiet|normal|high|full>' for command line argument '--verbosity'"));
}

TEST_CASE("parse arguments unknown", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--make-coffee"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
    CHECK(console.messages == contains_substring("unknown command line argument '--make-coffee'"));
}

TEST_CASE("parse arguments unknown Catch2 (no value)", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--invisibles"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
    CHECK(console.messages == contains_substring("unknown command line argument '--invisibles'"));
}

TEST_CASE("parse arguments unknown Catch2", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "--wait-for-keypress", "never"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
    CHECK(
        console.messages ==
        contains_substring("unknown command line argument '--wait-for-keypress'"));
    CHECK(input->arguments.empty());
}

TEST_CASE("parse arguments positional", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "arg1"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == ""sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "arg1"sv);
    CHECK(input->arguments[0].value_name.value() == "test regex"sv);
    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments multiple positional", "[cli]") {
    console_output_catcher console;

    const arg_vector args = {"test", "arg1", "arg2"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 2u);

    CHECK(input->arguments[0].name == ""sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "arg1"sv);
    CHECK(input->arguments[0].value_name.value() == "test regex"sv);

    CHECK(input->arguments[1].name == ""sv);
    REQUIRE(input->arguments[1].value.has_value());
    REQUIRE(input->arguments[1].value_name.has_value());
    CHECK(input->arguments[1].value.value() == "arg2"sv);
    CHECK(input->arguments[1].value_name.value() == "test regex"sv);

    CHECK(console.messages.empty());
}

TEST_CASE("parse arguments error no color", "[cli]") {
    console_output_catcher console;

    SECTION("duplicate arg") {
        const arg_vector args = {"test", "--color", "never", "--color", "always"};
        snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        CHECK(!contains_color_codes(console.messages));
    }

    SECTION("missing value") {
        const arg_vector args = {"test", "--color", "never", "--verbosity"};
        snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        CHECK(!contains_color_codes(console.messages));
    }

    SECTION("unknown arg") {
        const arg_vector args = {"test", "--color", "never", "--foobar"};
        snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
        CHECK(!contains_color_codes(console.messages));
    }
}

TEST_CASE("get option", "[cli]") {
    const arg_vector args = {"test", "--help", "--verbosity", "high"};
    auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());

    auto help_option = snitch::cli::get_option(*input, "--help");
    CHECK(help_option.has_value());
    CHECK(help_option->name == "--help"sv);
    CHECK(!help_option->value.has_value());
    CHECK(!help_option->value_name.has_value());

    auto verbosity_option = snitch::cli::get_option(*input, "--verbosity");
    CHECK(verbosity_option.has_value());
    CHECK(verbosity_option->name == "--verbosity"sv);
    REQUIRE(verbosity_option->value.has_value());
    REQUIRE(verbosity_option->value_name.has_value());
    CHECK(verbosity_option->value.value() == "high"sv);
    CHECK(verbosity_option->value_name.value() == "quiet|normal|high|full"sv);

    auto unknown_option = snitch::cli::get_option(*input, "--unknown");
    CHECK(!unknown_option.has_value());

    auto short_help_option = snitch::cli::get_option(*input, "-v");
    CHECK(!short_help_option.has_value());
}

TEST_CASE("get positional argument", "[cli]") {
    SECTION("good") {
        for (auto [scenario, args] : {
                 cli_input{"at end"sv, {"test", "--help", "--verbosity", "high", "arg1"}},
                 cli_input{"at middle"sv, {"test", "--help", "arg1", "--verbosity", "high"}},
                 cli_input{"at start"sv, {"test", "arg1", "--help", "--verbosity", "high"}},
                 cli_input{"alone"sv, {"test", "arg1"}},
                 cli_input{"multiple"sv, {"test", "arg1", "arg2"}},
                 cli_input{
                     "multiple interleaved"sv, {"test", "arg1", "--verbosity", "high", "arg2"}},
             }) {

#if SNITCH_TEST_WITH_SNITCH
            CAPTURE(scenario);
#endif

            auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
            REQUIRE(input.has_value());

            auto arg = snitch::cli::get_positional_argument(*input, "test regex");
            REQUIRE(arg.has_value());
            CHECK(arg->name == ""sv);
            CHECK(arg->value == "arg1"sv);
            CHECK(arg->value_name == "test regex"sv);

            if (input->executable.starts_with("multiple")) {
                std::size_t i = 0u;

                auto callback = [&](std::string_view value) noexcept {
                    if (i == 0u) {
                        CHECK(value == "arg1"sv);
                    } else {
                        CHECK(value == "arg2"sv);
                    }

                    ++i;
                };

                snitch::cli::for_each_positional_argument(*input, "test regex", callback);
                CHECK(i == 2u);
            }
        }
    }

    SECTION("bad") {
        for (auto [scenario, args] : {
                 cli_input{"only options"sv, {"test", "--help", "--verbosity", "high"}},
                 cli_input{"empty"sv, {"test"}},
             }) {

#if SNITCH_TEST_WITH_SNITCH
            CAPTURE(scenario);
#endif

            auto input = snitch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
            REQUIRE(input.has_value());

            auto arg = snitch::cli::get_positional_argument(*input, "test regex");
            CHECK(!arg.has_value());
        }
    }
}
