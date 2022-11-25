#include "testing.hpp"

using namespace std::literals;
using snatch::matchers::contains_substring;

using arg_vector = snatch::small_vector<const char*, snatch::max_command_line_args>;

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
