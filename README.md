# snatch

![Build Status](https://github.com/cschreib/snatch/actions/workflows/cmake.yml/badge.svg) [![codecov](https://codecov.io/gh/cschreib/snatch/branch/main/graph/badge.svg?token=X422DE81PN)](https://codecov.io/gh/cschreib/snatch)

Lightweight C++20 testing framework.

The goal of _snatch_ is to be a simple, cheap, non-invasive, and user-friendly testing framework. The design philosophy is to keep the testing API lean, including only what is strictly necessary to present clear messages when a test fails.

<!-- MarkdownTOC autolink="true" -->

- [Features and limitations](#features-and-limitations)
- [Example](#example)
- [Example build configurations with CMake](#example-build-configurations-with-cmake)
    - [Using _snatch_ as a regular library](#using-snatch-as-a-regular-library)
    - [Using _snatch_ as a header-only library](#using-snatch-as-a-header-only-library)
- [Benchmark](#benchmark)
- [Documentation](#documentation)
    - [Test case macros](#test-case-macros)
    - [Test check macros](#test-check-macros)
    - [Matchers](#matchers)
    - [Sections](#sections)
    - [Captures](#captures)
    - [Custom string serialization](#custom-string-serialization)
    - [Reporters](#reporters)
    - [Default main function](#default-main-function)
    - [Using your own main function](#using-your-own-main-function)
    - [Exceptions](#exceptions)
    - [Header-only build](#header-only-build)
    - [`clang-format` support](#clang-format-support)

<!-- /MarkdownTOC -->

## Features and limitations

 - No heap allocation from the testing framework, so memory leaks from your code can be detected precisely.
 - Works with exceptions disabled, albeit with a minor limitation (see [Exceptions](#exceptions) below).
 - No external dependency; just pure C++20 with the STL.
 - Compiles tests at least 40% faster than other testing frameworks (see [Benchmark](#benchmark)).
 - Defaults to reporting test results to the standard output, with coloring for readability, but test events can also be forwarded to a reporter callback for reporting to CI frameworks (Teamcity, ..., see [Reporters](#reporters)).
 - Limited subset of the [_Catch2_](https://github.com/catchorg/_Catch2_) API, including:
   - Simple test cases with `TEST_CASE(name, tags)`.
   - Typed test cases with `TEMPLATE_LIST_TEST_CASE(name, tags, types)` and  `TEMPLATE_TEST_CASE(name, tags, types...)`.
   - Pretty-printing check macros: `REQUIRE(expr)`, `CHECK(expr)`, `REQUIRE_THAT(expr, matcher)`, `CHECK_THAT(expr, matcher)`, `FAIL(msg)`, `FAIL_CHECK(msg)`.
   - Exception checking macros: `REQUIRE_THROWS_AS(expr, except)`, `CHECK_THROWS_AS(expr, except)`, `REQUIRE_THROWS_MATCHES(expr, exception, matcher)`, `CHECK_THROWS_MATCHES(expr, except, matcher)`.
   - Nesting multiple tests in a single test case with `SECTION(name, description)`.
   - Capturing context information to display on failure with `CAPTURE(vars...)` and `INFO(message)`.
   - Optional `main()` with simple command-line API similar to _Catch2_.
 - Additional API not in _Catch2_, or different from _Catch2_:
   - Macro to mark a test as skipped: `SKIP(msg)`.
   - Matchers use a different API (see [Matchers](#matchers) below).

If you need features that are not in the list above, please use _Catch2_ or _doctest_.

Notable current limitations:

 - Test macros (`REQUIRE(...)`, etc.) may only be used inside the test body (or in lambdas defined in the test body), and cannot be used in other functions.
 - No fixtures, or set-up/tear-down helpers (`SECTION()` can be used to share set-up/tear-down logic).
 - No multi-threaded test execution.


## Example

This is the same example code as in the [_Catch2_ tutorials](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md):

```c++
#include <snatch/snatch.hpp>

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE("Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(0) == 1 ); // this check will fail
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}; // note the required semicolon here: snatch test cases are expressions, not functions
```

Output:

![Screenshot from 2022-10-16 16-17-04](https://user-images.githubusercontent.com/2236577/196043565-531635c5-64e0-401c-8ff6-a533c9bbbf11.png)

And here is an example code for a typed test, also borrowed (and adapted) from the [_Catch2_ tutorials](https://github.com/catchorg/Catch2/blob/devel/docs/test-cases-and-sections.md#type-parametrised-test-cases):

```c++
#include <snatch/snatch.hpp>

using MyTypes = snatch::type_list<int, char, float>; // could also be std::tuple; any template type list will do
TEMPLATE_LIST_TEST_CASE("Template test case with test types specified inside snatch::type_list", "[template][list]", MyTypes)
{
    REQUIRE(sizeof(TestType) > 1); // will fail for 'char'
};
```

Output:

![Screenshot from 2022-10-16 16-16-50](https://user-images.githubusercontent.com/2236577/196043558-ed9ab329-5934-4bb3-a422-b48d6781cf96.png)


## Example build configurations with CMake

### Using _snatch_ as a regular library

Here is an example CMake file to download _snatch_ and define a test application:

```cmake
include(FetchContent)

FetchContent_Declare(snatch
                     GIT_REPOSITORY https://github.com/cschreib/snatch.git
                     GIT_TAG        6b71837fc60e2adc0d0d5603e04a16f694445804)
FetchContent_MakeAvailable(snatch)

set(RUNTIME_TEST_FILES
  # add your test files here...
  )

add_executable(my_tests ${YOUR_TEST_FILES})
target_link_libraries(my_tests PRIVATE snatch::snatch)
```

_snatch_ will provide the definition of `main()` [unless otherwise specified](#using-your-own-main-function).


### Using _snatch_ as a header-only library

Here is an example CMake file to download _snatch_ and define a test application:

```cmake
include(FetchContent)

FetchContent_Declare(snatch
                     GIT_REPOSITORY https://github.com/cschreib/snatch.git
                     GIT_TAG        6b71837fc60e2adc0d0d5603e04a16f694445804)
FetchContent_MakeAvailable(snatch)

set(RUNTIME_TEST_FILES
  # add your test files here...
  )

add_executable(my_tests ${YOUR_TEST_FILES})
target_link_libraries(my_tests PRIVATE snatch::snatch-header-only)
```

One (and only one!) of your test files needs to include _snatch_ as:
```c++
#define SNATCH_IMPLEMENTATION
#include <snatch_all.hpp>
```

See the documentation for the [header-only mode](#header-only-build) for more information. This will include the definition of `main()` [unless otherwise specified](#using-your-own-main-function).


## Benchmark

The following benchmarks were done using real-world tests from another library ([_observable_unique_ptr_](https://github.com/cschreib/observable_unique_ptr)), which generates about 4000 test cases and 25000 checks. This library uses "typed" tests almost exclusively, where each test case is instantiated several times, each time with a different tested type (here, 25 types). Building and running the tests was done without parallelism to simplify the comparison. The benchmarks were ran on a desktop with the following specs:
 - OS: Linux Mint 20.3, linux kernel 5.15.0-48-generic.
 - CPU: AMD Ryzen 5 2600 (6 core).
 - RAM: 16GB.
 - Storage: NVMe.
 - Compiler: GCC 10.3.0 with `-std=c++20`.

The benchmark tests can be found in different branches of _observable_unique_ptr_:
 - _snatch_: https://github.com/cschreib/observable_unique_ptr/tree/snatch
 - _Catch2_ v3.2.0: https://github.com/cschreib/observable_unique_ptr/tree/catch2
 - _doctest_ v2.4.9: https://github.com/cschreib/observable_unique_ptr/tree/doctest
 - _Boost.UT_ v1.1.9: https://github.com/cschreib/observable_unique_ptr/tree/boost_ut

Description of results below:
 - *Build framework*: Time required to build the testing framework library (if any), without any test.
 - *Build tests*: Time required to build the tests, assuming the framework library (if any) was already built.
 - *Build all*: Total time to build the tests and the framework library (if any).
 - *Run tests*: Total time required to run the tests.
 - *Library size*: Size of the compiled testing framework library (if any).
 - *Executable size*: Size of the compiled test executable, static linking to the testing framework library (if any).

Results for Debug builds:

| **Debug**       | _snatch_ | _Catch2_ | _doctest_ | _Boost UT_ |
|-----------------|----------|----------|-----------|------------|
| Build framework | 1.7s     | 64s      | 2.0s      | 0s         |
| Build tests     | 61s      | 86s      | 78s       | 109s       |
| Build all       | 63s      | 150s     | 80s       | 109s       |
| Run tests       | 16ms     | 83ms     | 60ms      | 20ms       |
| Library size    | 2.80MB   | 38.6MB   | 2.8MB     | 0MB        |
| Executable size | 31.7MB   | 49.3MB   | 38.6MB    | 51.9MB     |

Results for Release builds:

| **Release**     | _snatch_ | _Catch2_ | _doctest_ | _Boost UT_ |
|-----------------|----------|----------|-----------|------------|
| Build framework | 2.5s     | 68s      | 3.6s      | 0s         |
| Build tests     | 132s     | 264s     | 216s      | 281s       |
| Build all       | 135s     | 332s     | 220s      | 281s       |
| Run tests       | 9ms      | 31ms     | 36ms      | 10ms       |
| Library size    | 0.62MB   | 2.6MB    | 0.39MB    | 0MB        |
| Executable size | 9.3MB    | 17.4MB   | 15.2MB    | 11.3MB     |

Notes:
 - No attempt was made to optimize each framework's configuration; the defaults were used. C++20 modules were not used.
 - _Boost UT_ was unable to compile and pass the tests without modifications to its implementation (issues were reported).

## Documentation

### Test case macros

`TEST_CASE(NAME, TAGS) { /* test body */ };`

This must be called at namespace, global, or class scope; not inside a function or another test case. This defines a new test case of name `NAME`. `NAME` must be a string literal, and may contain any character, up to a maximum length configured by `SNATCH_MAX_TEST_NAME_LENGTH` (default is `1024`). This name will be used to display test reports, and can be used to filter the tests. It is not required to be a unique name. `TAGS` specify which tag(s) are associated with this test case. This must be a string literal with the same limitations as `NAME`. Within this string, individual tags must be surrounded by square brackets, with no white-space between tags (although white space within a tag is allowed). Tags can be used to filter the tests (e.g., run all tests with a given tag). Finally, `test body` is the body of your test case. Within this scope, you can use the test macros listed [below](#test-check-macros).


`TEMPLATE_TEST_CASE(NAME, TAGS, TYPES...) { /* test code for TestType */ };`

This is similar to `TEST_CASE`, except that it declares a new test case for each of the types listed in `TYPES...`. Within the test body, the current type can be accessed as `TestType`. If you tend to reuse the same list of types for multiple test cases, then `TEMPLATE_LIST_TEST_CASE()` is recommended instead.


`TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) { /* test code for TestType */ };`

This is equivalent to `TEMPLATE_TEST_CASE`, except that `TYPES` must be a template type list of the form `T<Types...>`, for example `snatch::type_list<Types...>` or `std::tuple<Types...>`. This type list can be declared once and reused for multiple test cases.


### Test check macros

The following macros can be used inside a test body, either immediately in the body itself, or inside a lambda function defined inside the body (if the lambda uses automatic by-reference capture, `[&]`). They _cannot_ be used inside other functions.


`REQUIRE(EXPR);`

This evaluates the expression `EXPR`, as in `if (EXPR)`, and reports a failure if `EXPR` evaluates to `false`. On failure, the current test case is stopped. Execution then continues with the next test case, if any. The value of each operand of the expression will be displayed on failure, provided the types involved can be serialized to a string. See [Custom string serialization](#custom-string-serialization) for more information. If one of the operands is a [matcher](#matchers) and the operation is `==`, then this will report a failure if there is no match. Conversely, if the operation is `!=`, then this will report a failure if there is a match.


`CHECK(EXPR);`

This is similar to `REQUIRE`, except that on failure the test case continues. Further failures may be reported in the same test case.


`REQUIRE_THAT(EXPR, MATCHER);`

This is equivalent to `REQUIRE(EXPR == MATCHER)`, and is provided for compatibility with _Catch2_.


`CHECK_THAT(EXPR, MATCHER);`

This is equivalent to `CHECK(EXPR == MATCHER)`, and is provided for compatibility with _Catch2_.


`FAIL(MSG);`

This reports a test failure with the message `MSG`. The current test case is stopped. Execution then continues with the next test case, if any.


`FAIL_CHECK(MSG);`

This is similar to `FAIL`, except that the test case continues. Further failures may be reported in the same test case.


`SKIP(MSG);`

This reports the current test case as "skipped". Any previously reported status for this test case is ignored. The current test case is stopped. Execution then continues with the next test case, if any.


`REQUIRE_THROWS_AS(EXPR, EXCEPT);`

This evaluates the expression `EXPR` inside a `try/catch` block, and attempts to catch an exception of type `EXCEPT`. If no exception is thrown, or an exception of a different type is thrown, then this reports a test failure. On failure, the current test case is stopped. Execution then continues with the next test case, if any.


`CHECK_THROWS_AS(EXPR, EXCEPT);`

This is similar to `REQUIRE_THROWS_AS`, except that on failure the test case continues. Further failures may be reported in the same test case.


`REQUIRE_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`

This is similar to `REQUIRE_THROWS_AS`, but further checks the content of the exception that has been caught. The caught exception is given to the matcher object specified in `MATCHER` (see [Matchers](#matchers)). If the exception object is not a match, then this reports a test failure.


`CHECK_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`

This is similar to `REQUIRE_THROWS_MATCHES`, except that on failure the test case continues. Further failures may be reported in the same test case.


### Matchers

Matchers in _snatch_ work differently than in _Catch2_. The required interface is:

 - `matcher.match(obj)` must return `true` if `obj` is a match, `false` otherwise.
 - `matcher.describe_match(obj, status)` must return a value convertible to `std::string_view`, describing why `obj` is or is not a match, depending on the value of `snatch::matchers::match_status`.
 - `matcher == obj` and `obj == matcher` must return `matcher.match(obj)`, and `matcher != obj` and `obj != matcher` must return `!matcher.match(obj)`; any matcher defined in the `snatch::matchers` namespace will have these operators defined automatically.

The following matchers are provided with _snatch_:

 - `snatch::matchers::contains_substring{"substring"}`: accepts a `std::string_view`, and will return a match if the string contains `"substring"`.
 - `snatch::matchers::with_what_contains{"substring"}`: accepts a `std::exception`, and will return a match if `what()` contains `"substring"`.
 - `snatch::matchers::is_any_of{T...}`: accepts an object of any type `T`, and will return a match if it is equal to any of the `T...`.


Here is an example matcher that, given a prefix `p`, checks if a string starts with the prefix `"<p>:"`:
```c++
namespace snatch::matchers {
struct has_prefix {
    std::string_view prefix;

    bool match(std::string_view s) const noexcept {
        return s.starts_with(prefix) && s.size() >= prefix.size() + 1 && s[prefix.size()] == ':';
    }

    small_string<max_message_length>
    describe_match(std::string_view s, match_status status) const noexcept {
        small_string<max_message_length> message;
        append_or_truncate(
            message, status == match_status::matched ? "found" : "could not find", " prefix '",
            prefix, ":' in '", s, "'");

        if (status == match_status::failed) {
            if (auto pos = s.find_first_of(':'); pos != s.npos) {
                append_or_truncate(message, "; found prefix '", s.substr(0, pos), ":'");
            } else {
                append_or_truncate(message, "; no prefix found");
            }
        }

        return message;
    }
};
} // namespace snatch::matchers
```


### Sections

As in _Catch2_, _snatch_ supports nesting multiple tests inside a single test case, to share set-up/tear-down logic. This is done using the `SECTION("name")` macro. Please see the [Catch2 documentation](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#test-cases-and-sections) for more details. Note: if any exception is thrown inside a section, or if a `REQUIRE()` check fails (or any other check which aborts execution), the whole test case is stopped. No other section will be executed.

Here is a brief example to demonstrate the flow of the test:

```c++
TEST_CASE( "test with sections", "[section]" ) {
    std::cout << "set-up" << std::endl;
    // shared set-up logic here...

    SECTION( "first section" ) {
        std::cout << " 1" << std::endl;
    }
    SECTION( "second section" ) {
        std::cout << " 2" << std::endl;
    }
    SECTION( "third section" ) {
        std::cout << " 3" << std::endl;
        SECTION( "nested section 1" ) {
            std::cout << "  3.1" << std::endl;
        }
        SECTION( "nested section 2" ) {
            std::cout << "  3.2" << std::endl;
        }
    }

    std::cout << "tear-down" << std::endl;
    // shared tear-down logic here...
};
```

The output of this test will be:
```
set-up
 1
tear-down
set-up
 2
tear-down
set-up
 3
  3.1
tear-down
set-up
 3
  3.2
tear-down

```


### Captures

As in _Catch2_, _snatch_ supports capturing contextual information to be displayed in the test failure report. This can be done with the `INFO(message)` and `CAPTURE(vars...)` macros. The captured information is "scoped", and will only be displayed for failures happening:
 - after the capture, and
 - in the same scope (or deeper).

For example, in the test below we compute a complicated formula in a `CHECK()`:

```c++
#include <cmath>

TEST_CASE("test without captures", "[captures]") {
    for (std::size_t i = 0; i < 10; ++i) {
        CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4);
    }
};

```

The output of this test is:

```
failed: running test case "test without captures"
          at test.cpp:116
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.309018 <= 0.400000
failed: running test case "test without captures"
          at test.cpp:116
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.000001 <= 0.400000
failed: running test case "test without captures"
          at test.cpp:116
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.309015 <= 0.400000
```

We are told the computed values that failed the check, but from just this information, it is difficult to recover the value of the loop index `i` which triggered the failure. To fix this, we can add `CAPTURE(i)` to capture the value of `i`:

```c++
#include <cmath>

TEST_CASE("test with captures", "[captures]") {
    for (std::size_t i = 0; i < 10; ++i) {
        CAPTURE(i);
        CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4);
    }
};

```

This new test now outputs:

```
failed: running test case "test with captures"
          at test.cpp:116
          with i := 4
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.309018 <= 0.400000
failed: running test case "test with captures"
          at test.cpp:116
          with i := 5
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.000001 <= 0.400000
failed: running test case "test with captures"
          at test.cpp:116
          with i := 6
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.309015 <= 0.400000
```

For convenience, any number of variables or expressions may be captured in a single `CAPTURE()` call; this is equivalent to writing multiple `CAPTURE()` calls:

```c++
#include <cmath>

TEST_CASE("test with many captures", "[captures]") {
    for (std::size_t i = 0; i < 10; ++i) {
        CAPTURE(i, 2 * i, std::pow(i, 3.0f));
        CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.2);
    }
};

```

This outputs:

```
failed: running test case "test with many captures"
          at test.cpp:122
          with i := 5
          with 2 * i := 10
          with std::pow(i, 3.0f) := 125.000000
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4), got 0.000001 <= 0.400000
```

The only requirement is that the captured variable or expression must of a type that _snatch_ can serialize to a string. See [Custom string serialization](#custom-string-serialization) for more information.

A more free-form way to add context to the tests is to use `INFO(...)`. The parameters to this macro will be serialized together to form a single string, which will be appended as one capture. This can be combined with `CAPTURE()`. For example:

```c++
#include <cmath>

TEST_CASE("test with info", "[captures]") {
    for (std::size_t i = 0; i < 5; ++i) {
        INFO("first loop (i < 5, with i = ", i, ")");
        CAPTURE(i);
        CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.2);
    }
    for (std::size_t i = 5; i < 10; ++i) {
        INFO("second loop (i >= 5, with i = ", i, ")");
        CAPTURE(i);
        CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.2);
    }
};

```

This outputs:

```
failed: running test case "test with info"
          at test.cpp:123
          with second loop (i >= 5, with i = 5)
          with i := 5
          CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.2), got 0.000001 <= 0.200000

```

### Custom string serialization

When the _snatch_ framework needs to serialize a value to a string, it does so with the free function `append(span, value)`, where `span` is a `snatch::small_string_span`, and `value` is the value to serialize. The function must return a boolean, equal to `true` if the serialization was successful, or `false` if there was not enough room in the output string to store the complete textual representation of the value. On failure, it is recommended to write as many characters as possible, and just truncate the output; this is what builtin functions do.

Builtin serialization functions are provided for all fundamental types: integers, enums (serialized as their underlying integer type), floating point, booleans, standard `string_view` and `char*`, and raw pointers.

If you want to serialize custom types not supported out of the box by _snatch_, you need to provide your own `append()` function. This function must be placed in the same namespace as your custom type or in the `snatch` namespace, so it can be found by ADL. In most cases, this function can be written in terms of serialization of fundamental types, and won't require low-level string manipulation. For example, to serialize a structure representing the 3D coordinates of a point:

```c++
namespace my_namespace {
    struct vec3d {
        float x;
        float y;
        float z;
    };

    bool append(small_string_span ss, const vec3d& v) {
        return append(ss, "{", v.x, ",", v.y, ",", v.z, "}");
    }
}
```

Alternatively, to serialize a class with an existing `toString()` member:

```c++
namespace my_namespace {
    class MyClass {
        // ...

    public:
        std::string toString() const;
    };

    bool append(small_string_span ss, const MyClass& c) {
        return append(ss, c.toString());
    }
}
```

If you cannot write your serialization function in this way (or for optimal speed), you will have to explicitly manage the string span. This typically involves:
 - calculating the expected length `n` of the textual representation of your value,
 - checking if `n` would exceed `ss.available()` (return `false` if so),
 - storing the current size of the span, using `old_size = ss.size()`,
 - growing the string span by this amount using `ss.grow(n)` or `ss.resize(old_size + n)`,
 - actually writing the textual representation of your value into the raw character array, accessible between `ss.begin() + old_size` and `ss.end()`.

Note that _snatch_ small strings have a fixed capacity; once this capacity is reached, the string cannot grow further, and the output must be truncated. This will normally be indicated by a `...` at the end of the strings being reported (this is automatically added by _snatch_; you do not need to do this yourself). If this happens, depending on which string was truncated, there are a number of compilation options that can be modified to increase the maximum string length. See `CMakeLists.txt`, or at the top of `snatch.hpp`, for a complete list.


### Reporters

By default, _snatch_ will report the test results to the standard output, using its own report format. You can override this by supplying your own "reporter" callback function to the test registry. This requires [using your own main function](#using-your-own-main-function).

The callback is a single `noexcept` function, taking two arguments:
 - a reference to the `snatch::registry` that generated the event
 - a reference to the `snatch::event::data` containing the event data. This type is a `std::variant`; use `std::visit` to act on the event.

The callback can be registered either as a free function, a stateless lambda, or a member function. You can register your own callback as follows:

```c++
// Free function.
// --------------
void report_function(const snatch::registry& r, const snatch::event::data& e) noexcept {
    /* ... */
}

snatch::tests.report_callback = &report_function;

// Stateless lambda (no captures).
// -------------------------------
snatch::tests.report_callback = [](const snatch::registry& r, const snatch::event::data& e) noexcept {
    /* ... */
};

// Stateful lambda (with captures).
// -------------------------------
auto lambda = [&](const snatch::registry& r, const snatch::event::data& e) noexcept {
    /* ... */
};

// 'lambda' must remain alive for the duration of the tests!
snatch::tests.report_callback = lambda;

// Member function (const or non-const, up to you).
// ------------------------------------------------
struct Reporter {
    void report(const snatch::registry& r, const snatch::event::data& e) /*const*/ noexcept {
        /* ... */
    }
};

Reporter reporter; // must remain alive for the duration of the tests!

snatch::tests.report_callback = {reporter, snatch::constant<&Reporter::report>};
```

If you need to use a reporter member function, please make sure that the reporter object remains alive for the duration of the tests (e.g., declare it static, global, or as a local variable declared in `main()`), or make sure to de-register it when your reporter is destroyed.

Likewise, when receiving a test event, the event object will only contain non-owning references (e.g., in the form of string views) to the actual event data. These references are only valid until the report function returns, after which point the event data will be destroyed or overwritten. If you need persistent copies of this data, you must explicitly copy the data, and not the references. For example, for strings, this could involve creating a `std::string` (or `snatch::small_string`) from the `std::string_view` stored in the event object.

An example reporter for _Teamcity_ is included for demonstration, see `include/snatch/snatch_teamcity.hpp`.


### Default main function

The default `main()` function provided in _snatch_ offers the following command-line API:
 - positional argument for filtering tests by name.
 - `-h,--help`: show command line help.
 - `-l,--list-tests`: list all tests.
 - `   --list-tags`: list all tags.
 - `   --list-tests-with-tag`: list all tests with a given tag.
 - `-t,--tags`: filter tests by tags instead of by name.
 - `-v,--verbosity [quiet|normal|high]`: select level of detail for the default reporter.
 - `   --color [always|never]`: enable/disable colors in the default reporter.


### Using your own main function

By default _snatch_ defines `main()` for you. To prevent this and provide your own `main()` function, when compiling _snatch_, `SNATCH_DEFINE_MAIN` must be set to `0`. If using CMake, this can be done with

```cmake
set(SNATCH_DEFINE_MAIN OFF)
```

just before calling `FetchContent_Declare()`. If using the header-only mode, this can also be done in the file that defines the _snatch_ implementation:

```c++
#define SNATCH_IMPLEMENTATION
#define SNATCH_DEFINE_MAIN 0
#include <snatch_all.hpp>
```

Here is a recommended `main()` function that replicates the default behavior of snatch:

```c++
int main(int argc, char* argv[]) {
    // Parse the command line arguments.
    std::optional<snatch::cli::input> args = snatch::cli::parse_arguments(argc, argv);
    if (!args) {
        // Parsing failed, an error has been reported, just return.
        return 1;
    }

    // Configure snatch using command line options.
    // You can then override the configuration below, or just remove this call to disable
    // command line options entirely.
    snatch::tests.configure(*args);

    // Your own initialization code goes here.
    // ...

    // Actually run the tests.
    // This will apply any filtering specified on the command line.
    return snatch::tests.run_tests(*args) ? 0 : 1;
```


### Exceptions

By default, _snatch_ assumes exceptions are enabled, and uses them in two cases:

 1. Obviously, in test macros that check exceptions being thrown (e.g., `REQUIRE_THROWS_AS(...)`).
 2. In `REQUIRE_*()` or `FAIL()` macros, to abort execution of the current test case.

If _snatch_ detects that exceptions are not available (or is configured with exceptions disabled, by setting `SNATCH_WITH_EXCEPTIONS` to `0`), then

 1. Test macros that check exceptions being thrown will not be defined.
 2. `REQUIRE_*()` and `FAIL()` macros will simply use `return` to abort execution. As a consequence, if these macros are used inside lambda functions, they will only abort execution of the lambda and not of the actual test case. Therefore, these macros should only be used in the immediate body of the test case, or simply not at all.


### Header-only build

The recommended way to use _snatch_ is to build and consume it like any other library. This provides the best incremental build times, a standard way to include and link to the _snatch_ implementation, and a cleaner separation between your code and _snatch_ code, but this also requires a bit more set up (running CMake, etc.).

For extra convenience, _snatch_ is also provided as a header-only library. The main header is called `snatch_all.hpp`, and can be downloaded as an artifact from each release on GitHub. It is also produced by any local CMake build, so you can also use it like other CMake libraries; just link to `snatch::snatch-header-only` instead of `snatch::snatch`. This is the only header required to use the library; other headers may be provided for convenience functions (e.g., reporters for common CI frameworks) and these must still be included separately.

To use _snatch_ as header-only in your code, simply include `snatch_all.hpp` instead of `snatch.hpp`. Then, one of your file must include the _snatch_ implementation. This can be done with a `.cpp` file containing only the following:

```c++
#define SNATCH_IMPLEMENTATION
#include <snatch_all.hpp>
```


### `clang-format` support

With its default configuration, `clang-format` will incorrectly format code using `SECTION()` if the section is the first statement inside a test case. This is because it does not know the semantic of this macro, and by default interprets it as a declaration rather than a control statement.

Fixing this requires `clang-format` version 13 at least, and requires adding the following to your `.clang-format` file:
```yaml
IfMacros: ['SECTION', 'SNATCH_SECTION']
SpaceBeforeParens: ControlStatementsExceptControlMacros
```
