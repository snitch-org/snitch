#include "testing.hpp"

using namespace std::literals;

namespace {
bool test_called = false;
} // namespace

TEST_CASE("test without tags") {
    CHECK(!test_called);
    test_called = true;
}

namespace {
std::size_t test_fixture_instances = 0u;

struct test_fixture {
    test_fixture() {
        ++test_fixture_instances;
    }

    ~test_fixture() {
        --test_fixture_instances;
    }

    bool used = false;
};
} // namespace

TEST_CASE_METHOD(test_fixture, "test with fixture 1") {
    CHECK(!used);
    CHECK(test_fixture_instances == 1u);
    used = true;
}

TEST_CASE_METHOD(test_fixture, "test with fixture 2") {
    CHECK(!used);
    CHECK(test_fixture_instances == 1u);
    used = true;
}

TEST_CASE_METHOD(test_fixture, "test with fixture and section") {
    SECTION("section 1") {
        CHECK(!used);
        CHECK(test_fixture_instances == 1u);
    }

    SECTION("section 2") {
        CHECK(!used);
        CHECK(test_fixture_instances == 1u);
    }

    used = true;
}

TEST_CASE("test after test with fixture") {
    CHECK(test_fixture_instances == 0u);
}

namespace {
#if SNITCH_WITH_EXCEPTIONS
// Dummy template parameters to allow commas when specifying the exception type
template<typename T, typename U>
struct test_exception : std::exception {
    const char* what() const noexcept override {
        return "test exception";
    }
};
#endif

template<std::size_t i, std::size_t j, std::size_t k>
int foo() {
    if constexpr (i != j || i != k) {
        return 0;
    } else {
#if SNITCH_WITH_EXCEPTIONS
        throw test_exception<int, int>{};
#else
        return 1;
#endif
    }
}

std::size_t matcher_created_count = 0u;
} // namespace

namespace snitch::matchers {
struct match_anything {
    // Dummy variable to allow commas when constructing the matcher
    int i = 0;
    int j = 0;

    template<typename T>
    bool match(T&&) const noexcept {
        return true;
    }

    template<typename T>
    small_string<max_message_length> describe_match(T&&, match_status) const noexcept {
        return "matched"sv;
    }
};

struct tracked_matcher {
    tracked_matcher() {
        ++matcher_created_count;
    }
    tracked_matcher(const tracked_matcher&) {
        ++matcher_created_count;
    }
    tracked_matcher(tracked_matcher&&) {
        ++matcher_created_count;
    }

    template<typename T>
    bool match(T&&) const noexcept {
        return true;
    }

    template<typename T>
    small_string<max_message_length> describe_match(T&&, match_status) const noexcept {
        return "matched"sv;
    }
};
} // namespace snitch::matchers

#if defined(SNITCH_TEST_WITH_SNITCH)
TEST_CASE("check macros with commas") {
    REQUIRE(foo<1, 2, 3>() == 0);
    REQUIRE_FALSE(foo<1, 2, 3>() != 0);
    CHECK(foo<1, 2, 3>() == 0);
    CHECK_FALSE(foo<1, 2, 3>() != 0);

    // Unfortunately, macros cannot support the following without the extra parentheses
    // around the expression:

    REQUIRE_THAT((foo<1, 2, 3>()), snitch::matchers::match_anything{0, 0});
    CHECK_THAT((foo<1, 2, 3>()), snitch::matchers::match_anything{0, 0});

#    if SNITCH_WITH_EXCEPTIONS
    CHECK_THROWS_AS((foo<2, 2, 2>()), test_exception<int, int>);
    REQUIRE_THROWS_AS((foo<2, 2, 2>()), test_exception<int, int>);
#    endif

    // Even more unfortunately, macros cannot support 'test_exception' to be specified inline here,
    // it must be declared first with an alias without template parameters:

#    if SNITCH_WITH_EXCEPTIONS
    using expected_exception = test_exception<int, int>;

    REQUIRE_THROWS_MATCHES(
        (foo<2, 2, 2>()), expected_exception, snitch::matchers::match_anything{0, 0});

    CHECK_THROWS_MATCHES(
        (foo<2, 2, 2>()), expected_exception, snitch::matchers::match_anything{0, 0});
#    endif
}

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_UNREACHABLE

TEST_CASE("matcher is not copied") {
    matcher_created_count = 0u;
    REQUIRE_THAT(1, snitch::matchers::tracked_matcher{});
    CHECK(matcher_created_count == 1u);

    matcher_created_count = 0u;
    CHECK_THAT(1, snitch::matchers::tracked_matcher{});
    CHECK(matcher_created_count == 1u);

#    if SNITCH_WITH_EXCEPTIONS
    matcher_created_count = 0u;
    REQUIRE_THROWS_MATCHES(throw 1, int, snitch::matchers::tracked_matcher{});
    CHECK(matcher_created_count == 1u);

    matcher_created_count = 0u;
    CHECK_THROWS_MATCHES(throw 1, int, snitch::matchers::tracked_matcher{});
    CHECK(matcher_created_count == 1u);
#    endif
}

SNITCH_WARNING_POP
#endif
