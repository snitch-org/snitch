#if defined(SNITCH_TEST_WITH_SNITCH)
// The library being tested is also the library used for testing...
#    if defined(SNITCH_TEST_HEADER_ONLY)
#        include "snitch/snitch_all.hpp"
#    else
#        include "snitch/snitch.hpp"
#    endif
#    define CHECK_THROWS_WHAT(EXPR, EXCEPT, MESSAGE)                                               \
        CHECK_THROWS_MATCHES(EXPR, EXCEPT, snitch::matchers::with_what_contains{(MESSAGE)})
#else

// The library being tested.
#    if defined(SNITCH_TEST_HEADER_ONLY)
#        include "snitch/snitch_all.hpp"
#    else
#        include "snitch/snitch.hpp"
#    endif
#    if !SNITCH_WITH_EXCEPTIONS
#        define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#    endif
// The library used for testing.
#    include <catch2/catch_message.hpp>
#    include <catch2/catch_template_test_macros.hpp>
#    include <catch2/catch_test_macros.hpp>
#    include <catch2/catch_tostring.hpp>
#    include <catch2/matchers/catch_matchers.hpp>
#    define CONSTEXPR_CHECK(...)                                                                   \
        if constexpr (__VA_ARGS__) {                                                               \
            CHECK(__VA_ARGS__);                                                                    \
        } else {                                                                                   \
            CHECK(__VA_ARGS__);                                                                    \
        }
#    define CONSTEXPR_REQUIRE(...)                                                                 \
        if constexpr (__VA_ARGS__) {                                                               \
            REQUIRE(__VA_ARGS__);                                                                  \
        } else {                                                                                   \
            REQUIRE(__VA_ARGS__);                                                                  \
        }
#    define CHECK_THROWS_WHAT(EXPR, EXCEPT, MESSAGE) REQUIRE_THROWS_WITH(EXPR, MESSAGE)

#    undef INFO
#    define INFO(ARG, ...) INTERNAL_CATCH_INFO("INFO", ARG)

#    include <ostream>

namespace concepts {
struct any_arg {
    template<typename T>
    operator T() const noexcept;
};

template<typename T>
concept matcher = requires(const T& m) {
    { m.match(any_arg{}) } -> snitch::convertible_to<bool>;
    {
        m.describe_match(any_arg{}, snitch::matchers::match_status{})
    } -> snitch::convertible_to<std::string_view>;
};

template<typename T>
concept function = std::is_function_v<T>;
} // namespace concepts

namespace snitch {
template<std::size_t N>
std::ostream& operator<<(std::ostream& str, const snitch::small_string<N>& in) {
    return str << std::string_view{in};
}
} // namespace snitch

namespace Catch {
template<concepts::function T>
struct StringMaker<T*> {
    static std::string convert(T* in) {
        return in != nullptr ? "funcptr" : "nullptr";
    }
};
} // namespace Catch
#endif

#if defined(__clang__)
#    define SNITCH_WARNING_DISABLE_UNREACHABLE
#    define SNITCH_WARNING_DISABLE_INT_BOOLEAN
#    define SNITCH_WARNING_DISABLE_PRECEDENCE
#    define SNITCH_WARNING_DISABLE_ASSIGNMENT
#elif defined(__GNUC__)
#    define SNITCH_WARNING_DISABLE_UNREACHABLE
#    define SNITCH_WARNING_DISABLE_INT_BOOLEAN                                                     \
        _Pragma("GCC diagnostic ignored \"-Wint-in-bool-context\"")
#    define SNITCH_WARNING_DISABLE_PRECEDENCE
#    define SNITCH_WARNING_DISABLE_ASSIGNMENT
#elif defined(_MSC_VER)
#    define SNITCH_WARNING_DISABLE_UNREACHABLE _Pragma("warning(disable: 4702)")
#    define SNITCH_WARNING_DISABLE_INT_BOOLEAN
#    define SNITCH_WARNING_DISABLE_PRECEDENCE _Pragma("warning(disable: 4554)")
#    define SNITCH_WARNING_DISABLE_ASSIGNMENT _Pragma("warning(disable: 4706)")
#else
#    define SNITCH_WARNING_DISABLE_UNREACHABLE
#    define SNITCH_WARNING_DISABLE_INT_BOOLEAN
#    define SNITCH_WARNING_DISABLE_PRECEDENCE
#    define SNITCH_WARNING_DISABLE_ASSIGNMENT
#endif

bool contains_color_codes(std::string_view msg) noexcept;
