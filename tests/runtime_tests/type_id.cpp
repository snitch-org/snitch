#include "testing.hpp"

#include <algorithm>

using namespace std::literals;

TEST_CASE("type id", "[utility]") {
    SECTION("all unique") {
        std::array types = {
            snitch::type_id<int>(),
            snitch::type_id<int*>(),
            snitch::type_id<float>(),
            snitch::type_id<void>(),
            snitch::type_id<std::string_view>(),
            snitch::type_id<snitch::small_string<8>>(),
            snitch::type_id<snitch::small_string<16>>()};

        CHECK(std::unique(types.begin(), types.end()) == types.end());
    }

    SECTION("constant") {
        CHECK(snitch::type_id<int>() == snitch::type_id<int>());
        CHECK(snitch::type_id<int*>() == snitch::type_id<int*>());
        CHECK(snitch::type_id<float>() == snitch::type_id<float>());
    }

    SECTION("void") {
        CHECK(snitch::type_id<void>() == nullptr);
    }
}
