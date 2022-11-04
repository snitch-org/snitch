#include "testing.hpp"

using namespace std::literals;

constexpr std::size_t max_length = 20u;

using string_type = snatch::small_string<max_length>;

TEMPLATE_TEST_CASE(
    "append",
    "[utility]",
    int,
    unsigned int,
    std::ptrdiff_t,
    std::size_t,
    float,
    double,
    bool,
    void*,
    const void*,
    std::nullptr_t,
    std::string_view) {

    auto create_value = []() -> std::pair<TestType, std::string_view> {
        if constexpr (std::is_same_v<TestType, int>) {
            return {-112, "-112"sv};
        } else if constexpr (std::is_same_v<TestType, unsigned int>) {
            return {203u, "203"sv};
        } else if constexpr (std::is_same_v<TestType, std::ptrdiff_t>) {
            return {-546876, "-546876"sv};
        } else if constexpr (std::is_same_v<TestType, std::size_t>) {
            return {26545u, "26545"sv};
        } else if constexpr (std::is_same_v<TestType, float>) {
            return {3.1415f, "3.141500"sv};
        } else if constexpr (std::is_same_v<TestType, double>) {
            return {-0.0001, "-0.000100"sv};
        } else if constexpr (std::is_same_v<TestType, bool>) {
            return {true, "true"sv};
        } else if constexpr (std::is_same_v<TestType, void*>) {
            static int i = 0;
            return {&i, "0x"sv};
        } else if constexpr (std::is_same_v<TestType, const void*>) {
            static const int i = 0;
            return {&i, "0x"sv};
        } else if constexpr (std::is_same_v<TestType, std::nullptr_t>) {
            return {{}, "nullptr"};
        } else if constexpr (std::is_same_v<TestType, std::string_view>) {
            return {"hello"sv, "hello"sv};
        }
    };

    SECTION("on empty") {
        string_type s;

        auto [value, expected] = create_value();
        CHECK(append(s, value));

        if constexpr (!std::is_pointer_v<TestType>) {
            CHECK(std::string_view(s) == expected);
        } else {
#if defined(SNATCH_COMPILER_MSVC)
            CHECK(std::string_view(s).size() > 0);
#else
            CHECK(std::string_view(s).starts_with(expected));
#endif
        }
    }

    SECTION("on partially full") {
        std::string_view initial = "abcdefghijklmnopqr"sv;
        string_type      s       = initial;

        auto [value, expected] = create_value();
        CHECK(!append(s, value));
        CHECK(std::string_view(s).starts_with(initial));
        if constexpr (
            (std::is_arithmetic_v<TestType> && !std::is_same_v<TestType, bool>) ||
            std::is_pointer_v<TestType>) {
            // We are stuck with snprintf, which insists on writing a null-terminator character,
            // therefore we loose one character at the end.
            CHECK(expected.substr(0, 1) == std::string_view(s).substr(s.size() - 1, 1));
        } else {
            CHECK(expected.substr(0, 2) == std::string_view(s).substr(s.size() - 2, 2));
        }
    }

    SECTION("on full") {
        std::string_view initial = "abcdefghijklmnopqrst"sv;
        string_type      s       = initial;

        auto [value, expected] = create_value();
        CHECK(!append(s, value));
        CHECK(std::string_view(s) == initial);
    }
};

TEST_CASE("append multiple", "[utility]") {
    string_type s;

    SECTION("nothing") {
        CHECK(append(s, "", "", "", ""));
        CHECK(std::string_view(s) == ""sv);
    }

    SECTION("enough space") {
        CHECK(append(s, "int=", 123456));
        CHECK(std::string_view(s) == "int=123456"sv);
    }

    SECTION("just enough space") {
        CHECK(append(s, "int=", 123456, " bool=", true));
        CHECK(std::string_view(s) == "int=123456 bool=true"sv);
    }

    SECTION("not enough space between arguments") {
        CHECK(!append(s, "int=", 123456, " bool=", true, " float=", 3.1415));
        CHECK(std::string_view(s) == "int=123456 bool=true"sv);
    }

    SECTION("not enough space in middle of argument") {
        CHECK(!append(s, "int=", 123456, ", bool=", true));
        CHECK(std::string_view(s) == "int=123456, bool=tru"sv);
    }
};
