#define SNITCH_IMPLEMENTATION

#if defined(SNITCH_TEST_WITH_SNITCH)
#    undef SNITCH_DEFINE_MAIN
#    define SNITCH_DEFINE_MAIN 1
#endif

#include "testing.hpp"

#if defined(SNITCH_TEST_WITH_SNITCH) && !defined(SNITCH_TEST_HEADER_ONLY)
#    undef SNITCH_EXPORT
#    define SNITCH_EXPORT
#    include "snitch_main.cpp"
#endif
