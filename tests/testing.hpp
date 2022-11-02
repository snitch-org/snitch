#if SNATCH_TEST_WITH_SNATCH
// The library being tested is also the library used for testing...
#    include "snatch/snatch.hpp"
#else
// The library being tested.
#    undef SNATCH_WITH_SHORTHAND_MACROS
#    define SNATCH_WITH_SHORTHAND_MACROS 0
#    include "snatch/snatch.hpp"
// The library used for testing.
#    include "doctest/doctest.h"
#    define SECTION(name) DOCTEST_SUBCASE(name)
#    undef TEST_CASE
#    define TEST_CASE(name, tags) DOCTEST_TEST_CASE(tags " " name)
#endif
