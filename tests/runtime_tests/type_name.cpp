#include "testing.hpp"

using namespace std::literals;

struct global_test_struct {
    int  i = 0;
    bool b = true;
};

TEST_CASE("type name", "[utility]") {
    struct test_struct {
        int  i = 0;
        bool b = true;
    };

    CHECK(snitch::type_name<int> == "int");
    CHECK(snitch::type_name<unsigned> == "unsigned int");
    CHECK(snitch::type_name<long> == snitch::matchers::is_any_of("long int"sv, "long"sv));
    CHECK(snitch::type_name<float> == "float");
    CHECK(snitch::type_name<double> == "double");
    CHECK(snitch::type_name<void*> == snitch::matchers::is_any_of("void*"sv, "void *"sv));

    CHECK(
        snitch::type_name<const void*> ==
        snitch::matchers::is_any_of("const void*"sv, "const void *"sv));

    CHECK(
        snitch::type_name<std::string_view> ==
        snitch::matchers::is_any_of(
            "std::basic_string_view<char>"sv, "std::string_view"sv,
            "class std::basic_string_view<char,struct std::char_traits<char> >"sv,
            "std::__1::basic_string_view<char, std::__1::char_traits<char> >"sv));

    CHECK(
        snitch::type_name<global_test_struct> ==
        snitch::matchers::is_any_of("global_test_struct"sv, "struct global_test_struct"sv));

    CHECK(snitch::type_name<test_struct>.ends_with("test_struct"));
}
