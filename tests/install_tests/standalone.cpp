#if defined(HEADER_ONLY)
#    define SNITCH_IMPLEMENTATION
#    include <snitch/snitch_all.hpp>
#else
#    include <snitch/snitch.hpp>
#endif

TEST_CASE("compiles and runs") {
    REQUIRE(true == !false);
}
