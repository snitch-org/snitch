#include "testing.hpp"

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
