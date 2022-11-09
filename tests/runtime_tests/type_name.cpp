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

    CHECK(snatch::type_name<int> == "int");
    CHECK(snatch::type_name<unsigned> == "unsigned int");
    CHECK(snatch::type_name<long> == snatch::matchers::is_any_of("long int"sv, "long"sv));
    CHECK(snatch::type_name<float> == "float");
    CHECK(snatch::type_name<double> == "double");
    CHECK(snatch::type_name<void*> == snatch::matchers::is_any_of("void*"sv, "void *"sv));

    CHECK(
        snatch::type_name<const void*> ==
        snatch::matchers::is_any_of("const void*"sv, "const void *"sv));

    CHECK(
        snatch::type_name<std::string_view> ==
        snatch::matchers::is_any_of(
            "std::basic_string_view<char>"sv, "std::string_view"sv,
            "class std::basic_string_view<char,struct std::char_traits<char> >"sv,
            "std::__1::basic_string_view<char, std::__1::char_traits<char> >"sv));

    CHECK(
        snatch::type_name<global_test_struct> ==
        snatch::matchers::is_any_of("global_test_struct"sv, "struct global_test_struct"sv));

    CHECK(snatch::type_name<test_struct>.ends_with("test_struct"));
};
