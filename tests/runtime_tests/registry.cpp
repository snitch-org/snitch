#include "testing.hpp"
#include "testing_event.hpp"

using namespace std::literals;

bool test_called       = false;
bool test_called_int   = false;
bool test_called_float = false;

TEST_CASE("add test", "[registry]") {
    mock_framework framework;
    framework.setup_reporter();

    SECTION("regular test case") {
        test_called = false;

        framework.registry.add("name", "[tag]") = [](snatch::impl::test_run&) {
            test_called = true;
        };

        REQUIRE(framework.get_num_registered_tests() == 1u);

        auto& test = *framework.registry.begin();
        CHECK(test.id.name == "name"sv);
        CHECK(test.id.tags == "[tag]"sv);
        CHECK(test.id.type == ""sv);
        REQUIRE((test.func != nullptr));

        framework.registry.run(test);
        CHECK(test_called == true);
    }

    SECTION("template test case") {
        test_called       = false;
        test_called_int   = false;
        test_called_float = false;

        framework.registry.add_with_types<std::tuple<int, float>>("name", "[tag]") =
            []<typename T>(snatch::impl::test_run&) {
                if constexpr (std::is_same_v<T, int>) {
                    test_called_int = true;
                } else if constexpr (std::is_same_v<T, float>) {
                    test_called_float = true;
                } else {
                    test_called = true;
                }
            };

        REQUIRE(framework.get_num_registered_tests() == 2u);

        auto& test1 = *framework.registry.begin();
        CHECK(test1.id.name == "name"sv);
        CHECK(test1.id.tags == "[tag]"sv);
        CHECK(test1.id.type == "int"sv);
        REQUIRE((test1.func != nullptr));

        framework.registry.run(test1);
        CHECK(test_called == false);
        CHECK(test_called_int == true);
        CHECK(test_called_float == false);

        test_called       = false;
        test_called_int   = false;
        test_called_float = false;

        auto& test2 = *(framework.registry.begin() + 1);
        CHECK(test2.id.name == "name"sv);
        CHECK(test2.id.tags == "[tag]"sv);
        CHECK(test2.id.type == "float"sv);
        REQUIRE((test2.func != nullptr));

        framework.registry.run(test2);
        CHECK(test_called == false);
        CHECK(test_called_int == false);
        CHECK(test_called_float == true);
    }
};

TEST_CASE("report failure", "[registry]") {
    mock_framework framework;
    framework.setup_print();

    SECTION("FAIL") {
#define SNATCH_CURRENT_TEST mock_test
        framework.registry.add("name", "[tag]") = [](snatch::impl::test_run& mock_test) {
            SNATCH_FAIL("failure message");
        };
#undef SNATCH_CURRENT_TEST

        auto& test = *framework.registry.begin();
        framework.registry.run(test);
    }
};
