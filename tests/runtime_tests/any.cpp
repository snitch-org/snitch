#include "testing.hpp"
#include "testing_assertions.hpp"

namespace {
struct state_monitor {
    int* state = nullptr;

    state_monitor() = default;

    explicit state_monitor(int* s) : state(s) {
        *state += 1;
    }

    ~state_monitor() {
        *state -= 1;
    }
};
} // namespace

TEST_CASE("any", "[utility]") {
    constexpr std::size_t max_size = 16;

    int state1 = 0;
    int state2 = 0;

    SECTION("default construct") {
        snitch::inplace_any<max_size> storage;
    }

    SECTION("construct in-place") {
        {
            snitch::inplace_any<max_size> storage(std::in_place_type_t<state_monitor>{}, &state1);
            CHECK(storage.has_value());
            CHECK(storage.type() == snitch::type_id<state_monitor>());
            CHECK(state1 == 1);
            CHECK(storage.get<state_monitor>().state == &state1);
        }
        CHECK(state1 == 0);
    }

    SECTION("move constructor") {
        {
            snitch::inplace_any<max_size> storage1(std::in_place_type_t<state_monitor>{}, &state1);
            snitch::inplace_any<max_size> storage2(std::move(storage1));
            CHECK(!storage1.has_value());
            CHECK(storage2.has_value());
            CHECK(state1 == 1);
        }
        CHECK(state1 == 0);
    }

    SECTION("move assignment on empty") {
        {
            snitch::inplace_any<max_size> storage2;
            {
                snitch::inplace_any<max_size> storage1(
                    std::in_place_type_t<state_monitor>{}, &state1);
                storage2 = std::move(storage1);
                CHECK(!storage1.has_value());
            }

            CHECK(storage2.has_value());
            CHECK(state1 == 1);
        }
        CHECK(state1 == 0);
    }

    SECTION("move assignment on full") {
        {
            snitch::inplace_any<max_size> storage2(std::in_place_type_t<state_monitor>{}, &state2);
            {
                snitch::inplace_any<max_size> storage1(
                    std::in_place_type_t<state_monitor>{}, &state1);
                storage2 = std::move(storage1);
                CHECK(!storage1.has_value());
            }

            CHECK(storage2.has_value());
            CHECK(state1 == 1);
            CHECK(state2 == 0);
        }
        CHECK(state1 == 0);
        CHECK(state2 == 0);
    }

    SECTION("emplace and reset") {
        {
            snitch::inplace_any<max_size> storage;
            storage.emplace<state_monitor>(&state1);
            CHECK(storage.has_value());
            CHECK(storage.type() == snitch::type_id<state_monitor>());
            CHECK(state1 == 1);
            CHECK(storage.get<state_monitor>().state == &state1);

            storage.reset();
            CHECK(!storage.has_value());
            CHECK(state1 == 0);
        }
        CHECK(state1 == 0);
        CHECK(state2 == 0);
    }

    SECTION("emplace over existing") {
        {
            snitch::inplace_any<max_size> storage;
            storage.emplace<state_monitor>(&state1);
            storage.emplace<state_monitor>(&state2);
            CHECK(storage.has_value());
            CHECK(storage.type() == snitch::type_id<state_monitor>());
            CHECK(state1 == 0);
            CHECK(state2 == 1);
            CHECK(storage.get<state_monitor>().state == &state2);
        }
        CHECK(state1 == 0);
        CHECK(state2 == 0);
    }

    SECTION("reset empty") {
        snitch::inplace_any<max_size> storage;
        storage.reset();
        CHECK(!storage.has_value());
    }

#if SNITCH_WITH_EXCEPTIONS
    SECTION("get empty") {
        assertion_exception_enabler   enabler;
        snitch::inplace_any<max_size> storage;

        CHECK_THROWS_WHAT(
            storage.get<state_monitor>(), assertion_exception, "inplace_any is empty");
    }

    SECTION("get wrong type") {
        assertion_exception_enabler   enabler;
        snitch::inplace_any<max_size> storage;
        storage.emplace<int>(0);

        CHECK_THROWS_WHAT(
            storage.get<state_monitor>(), assertion_exception,
            "inplace_any holds an object of a different type");
    }
#endif
}
