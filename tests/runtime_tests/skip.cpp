#include "testing.hpp"
#include "testing_event.hpp"

#include <stdexcept>
#include <string>

using namespace std::literals;

SNATCH_WARNING_PUSH
SNATCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("skip", "[test macros]") {
    mock_framework framework;

    SECTION("no skip") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_FAIL_CHECK("trigger");
        };

        framework.run_test();
        CHECK(framework.get_num_skips() == 0u);
    }

    SECTION("only skip") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SKIP("hello");
        };

        framework.run_test();
        CHECK(framework.get_num_skips() == 1u);
    }

    SECTION("skip failure") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SKIP("hello");
            SNATCH_FAIL_CHECK("trigger");
        };

        framework.run_test();
        CHECK(framework.get_num_skips() == 1u);
        CHECK(framework.get_num_failures() == 0u);
    }

    SECTION("skip section") {
        framework.test_case.func = [](snatch::impl::test_run& SNATCH_CURRENT_TEST) {
            SNATCH_SECTION("section 1") {
                SNATCH_SKIP("hello");
            }
            SNATCH_SECTION("section 2") {
                SNATCH_FAIL_CHECK("trigger");
            }
        };

        framework.run_test();
        CHECK(framework.get_num_skips() == 1u);
        CHECK(framework.get_num_failures() == 0u);
    }
};

SNATCH_WARNING_POP
