#if defined(SNITCH_TEST_WITH_SNITCH)
// The library being tested is also the library used for testing...
#    if defined(SNITCH_TEST_HEADER_ONLY)
#        include "snitch/snitch_all.hpp"
#    else
#        include "snitch/snitch.hpp"
#    endif

#else

// The library being tested.
#    if defined(SNITCH_TEST_HEADER_ONLY)
#        include "snitch/snitch_all.hpp"
#    else
#        include "snitch/snitch.hpp"
#    endif
// The library used for testing.
#    include "doctest/doctest.h"
// Adjust doctest macros to match the snitch API
#    define SECTION(name) DOCTEST_SUBCASE(name)
#    undef TEST_CASE
#    define TEST_CASE(name, tags) DOCTEST_TEST_CASE(tags " " name)
#    define TEMPLATE_TEST_CASE(name, tags, ...)                                                    \
        DOCTEST_TEST_CASE_TEMPLATE(tags " " name, TestType, __VA_ARGS__)
#    define SKIP(message) return

#    include <ostream>

namespace doctest::detail {
template<typename T>
concept function = std::is_function_v<T>;

template<function T>
struct filldata<T*> {
    static void fill(std::ostream* stream, T* in) {
        filldata<const void*>::fill(stream, in != nullptr ? "funcptr" : "nullptr");
    }
};
} // namespace doctest::detail
#endif

#if defined(__clang__)
#    define SNITCH_WARNING_DISABLE_UNREACHABLE
#elif defined(__GNUC__)
#    define SNITCH_WARNING_DISABLE_UNREACHABLE
#elif defined(_MSC_VER)
#    define SNITCH_WARNING_DISABLE_UNREACHABLE _Pragma("warning(disable: 4702)")
#else
#    define SNITCH_WARNING_DISABLE_UNREACHABLE
#endif
