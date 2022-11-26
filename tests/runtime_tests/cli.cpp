#include "testing.hpp"

using namespace std::literals;
using snatch::matchers::contains_substring;

TEST_CASE("parse arguments empty", "[cli]") {
    const arg_vector args = {"test"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
};

TEST_CASE("parse arguments empty .exe", "[cli]") {
    const arg_vector args = {"test.exe"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
};

TEST_CASE("parse arguments empty .something.exe", "[cli]") {
    const arg_vector args = {"test.something.exe"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test.something"sv);
    CHECK(input->arguments.empty());
};

TEST_CASE("parse arguments help (long form)", "[cli]") {
    const arg_vector args = {"test", "--help"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--help"sv);
    CHECK(!input->arguments[0].value.has_value());
    CHECK(!input->arguments[0].value_name.has_value());
};

TEST_CASE("parse arguments help (short form)", "[cli]") {
    const arg_vector args = {"test", "-h"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--help"sv);
    CHECK(!input->arguments[0].value.has_value());
    CHECK(!input->arguments[0].value_name.has_value());
};

TEST_CASE("parse arguments help (duplicate)", "[cli]") {
    const arg_vector args = {"test", "--help", "--help"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(!input.has_value());
};

TEST_CASE("parse arguments verbosity (long form)", "[cli]") {
    const arg_vector args = {"test", "--verbosity", "high"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--verbosity"sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "high"sv);
    CHECK(input->arguments[0].value_name.value() == "quiet|normal|high"sv);
};

TEST_CASE("parse arguments verbosity (short form)", "[cli]") {
    const arg_vector args = {"test", "-v", "high"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == "--verbosity"sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "high"sv);
    CHECK(input->arguments[0].value_name.value() == "quiet|normal|high"sv);
};

TEST_CASE("parse arguments verbosity (no value)", "[cli]") {
    const arg_vector args = {"test", "--verbosity"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    CHECK(!input.has_value());
};

TEST_CASE("parse arguments unknown", "[cli]") {
    const arg_vector args = {"test", "--make-coffee"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    CHECK(input->arguments.empty());
};

TEST_CASE("parse arguments positional", "[cli]") {
    const arg_vector args = {"test", "arg1"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());
    CHECK(input->executable == "test"sv);
    REQUIRE(input->arguments.size() == 1u);
    CHECK(input->arguments[0].name == ""sv);
    REQUIRE(input->arguments[0].value.has_value());
    REQUIRE(input->arguments[0].value_name.has_value());
    CHECK(input->arguments[0].value.value() == "arg1"sv);
    CHECK(input->arguments[0].value_name.value() == "test regex"sv);
};

TEST_CASE("parse arguments too many positional", "[cli]") {
    const arg_vector args = {"test", "arg1", "arg2"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(!input.has_value());
};

TEST_CASE("get option", "[cli]") {
    const arg_vector args = {"test", "--help", "--verbosity", "high"};
    auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());

    REQUIRE(input.has_value());

    auto help_option = snatch::cli::get_option(*input, "--help");
    CHECK(help_option.has_value());
    CHECK(help_option->name == "--help"sv);
    CHECK(!help_option->value.has_value());
    CHECK(!help_option->value_name.has_value());

    auto verbosity_option = snatch::cli::get_option(*input, "--verbosity");
    CHECK(verbosity_option.has_value());
    CHECK(verbosity_option->name == "--verbosity"sv);
    REQUIRE(verbosity_option->value.has_value());
    REQUIRE(verbosity_option->value_name.has_value());
    CHECK(verbosity_option->value.value() == "high"sv);
    CHECK(verbosity_option->value_name.value() == "quiet|normal|high"sv);

    auto unknown_option = snatch::cli::get_option(*input, "--unknown");
    CHECK(!unknown_option.has_value());

    auto short_help_option = snatch::cli::get_option(*input, "-v");
    CHECK(!short_help_option.has_value());
};

TEST_CASE("get positional argument", "[cli]") {
    SECTION("good") {
        for (auto [scenario, args] : {
                 cli_input{"at end"sv, {"test", "--help", "--verbosity", "high", "arg1"}},
                 cli_input{"at middle"sv, {"test", "--help", "arg1", "--verbosity", "high"}},
                 cli_input{"at start"sv, {"test", "arg1", "--help", "--verbosity", "high"}},
                 cli_input{"alone"sv, {"test", "arg1"}},
             }) {

#if SNATCH_TEST_WITH_SNATCH
            CAPTURE(scenario);
#endif

            auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
            REQUIRE(input.has_value());

            auto arg = snatch::cli::get_positional_argument(*input, "test regex");
            REQUIRE(arg.has_value());
            CHECK(arg->name == ""sv);
            CHECK(arg->value == "arg1"sv);
            CHECK(arg->value_name == "test regex"sv);
        }
    }

    SECTION("bad") {
        for (auto [scenario, args] : {
                 cli_input{"only options"sv, {"test", "--help", "--verbosity", "high"}},
                 cli_input{"empty"sv, {"test"}},
             }) {

#if SNATCH_TEST_WITH_SNATCH
            CAPTURE(scenario);
#endif

            auto input = snatch::cli::parse_arguments(static_cast<int>(args.size()), args.data());
            REQUIRE(input.has_value());

            auto arg = snatch::cli::get_positional_argument(*input, "test regex");
            CHECK(!arg.has_value());
        }
    }
};
