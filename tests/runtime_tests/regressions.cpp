#include "testing.hpp"

// Checks that we don't get parenthesis warnings from GCC when in template function.
// https://github.com/catchorg/Catch2/issues/870
// https://github.com/catchorg/Catch2/issues/565
// The original issue was a GCC bug, which forced Catch2 to disable the parentheses warning
// globally. This seems to have been fixed in GCC now.
namespace {
template<typename T>
void template_test_function() {
    T a = 1, b = 1;
    CHECK(a == b);
}
} // namespace

TEST_CASE("Wparentheses", "[regressions]") {
    template_test_function<int>();
}
