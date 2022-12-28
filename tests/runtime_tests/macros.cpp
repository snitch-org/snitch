#include "testing.hpp"

namespace {
bool test_called = false;
} // namespace

TEST_CASE("test without tags") {
    CHECK(!test_called);
    test_called = true;
}
