#include "snatch/snatch.hpp"

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
    CHECK(snatch::type_name<long> == "long int");
    CHECK(snatch::type_name<float> == "float");
    CHECK(snatch::type_name<double> == "double");
    CHECK(snatch::type_name<void*> == "void*");
    CHECK(snatch::type_name<const void*> == "const void*");
    CHECK(snatch::type_name<std::string_view> == "std::basic_string_view<char>");
    CHECK(snatch::type_name<global_test_struct> == "global_test_struct");
    CHECK(snatch::type_name<test_struct>.ends_with("test_struct"));
};
