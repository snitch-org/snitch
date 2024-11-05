# snitch

![Build Status](https://github.com/snitch-org/snitch/actions/workflows/cmake.yml/badge.svg) [![codecov](https://codecov.io/gh/snitch-org/snitch/branch/main/graph/badge.svg?token=X422DE81PN)](https://codecov.io/gh/snitch-org/snitch) [![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)

![The snitch logo: a jay feather](doc/logo-big.png)

Lightweight C++20 testing framework.

The goal of _snitch_ is to be a simple, cheap, non-invasive, and user-friendly testing framework. The design philosophy is to keep the testing API lean, including only what is strictly necessary to present clear messages when a test fails.

<!-- MarkdownTOC autolink="true" -->

- [Features and limitations](#features-and-limitations)
- [Example](#example)
- [Example build configurations with CMake](#example-build-configurations-with-cmake)
    - [Using _snitch_ as a regular library](#using-snitch-as-a-regular-library)
    - [Using _snitch_ as a header-only library](#using-snitch-as-a-header-only-library)
- [Example build configuration with meson](#example-build-configuration-with-meson)
- [Example build configuration with vcpkg](#example-build-configuration-with-vcpkg)
- [Benchmark](#benchmark)
- [Documentation](#documentation)
    - [Detailed comparison with _Catch2_](#detailed-comparison-with-catch2)
    - [Test case macros](#test-case-macros)
        - [Standalone test cases](#standalone-test-cases)
        - [Test cases with fixtures](#test-cases-with-fixtures)
    - [Test check macros](#test-check-macros)
        - [Run-time](#run-time)
        - [Compile-time](#compile-time)
        - [Run-time and compile-time](#run-time-and-compile-time)
        - [Exception checks](#exception-checks)
        - [Miscellaneous](#miscellaneous)
    - [Advanced API](#advanced-api)
    - [Tags](#tags)
    - [Matchers](#matchers)
    - [Sections](#sections)
    - [Captures](#captures)
    - [Custom string serialization](#custom-string-serialization)
    - [Reporters](#reporters)
        - [Built-in reporters](#built-in-reporters)
        - [Overriding the default reporter](#overriding-the-default-reporter)
        - [Registering a new reporter](#registering-a-new-reporter)
    - [Output colors](#output-colors)
    - [Command-line API](#command-line-api)
    - [Selecting which tests to run](#selecting-which-tests-to-run)
    - [Using your own main function](#using-your-own-main-function)
    - [Exceptions](#exceptions)
    - [Header-only build](#header-only-build)
    - [IDE integrations](#ide-integrations)
    - [`clang-format` support](#clang-format-support)
- [Contributing](#contributing)
- [Code of conduct](#code-of-conduct)
- [Why the name _snitch_?](#why-the-name-_snitch_)

<!-- /MarkdownTOC -->

## Features and limitations

 - No heap allocation from the testing framework, so heap allocations from your code can be tracked precisely.
 - Works with exceptions disabled, albeit with a minor limitation (see [Exceptions](#exceptions) below).
 - No external dependency; just pure C++20 with the STL.
 - Compiles template-heavy tests at least 50% faster than other testing frameworks (see Release [benchmarks](#benchmark)).
 - By defaults, test results are reported to the standard output, with optional coloring for readability. Test events can also be forwarded to a reporter callback for reporting to CI frameworks (Teamcity, ..., see [Reporters](#reporters)).
 - Limited subset of the [_Catch2_](https://github.com/catchorg/Catch2) API, see [Comparison with _Catch2_](#detailed-comparison-with-catch2).
 - [IDE integrations](#ide-integrations) using existing _Catch2_ plugins/adaptors.
 - Additional API not in _Catch2_, or different from _Catch2_:
   - Matchers use a different API (see [Matchers](#matchers) below).
   - Additional macros for testing [`constexpr`](#run-time-and-compile-time) and [`consteval`](#compile-time) expressions.
- Can be disabled at build time, to allow mixing code and tests in the same file with minimal overheads.

If you need features that are not in the list above, please use _Catch2_ or _doctest_.

Notable current limitations:

 - No multithreaded test execution yet; the code is thread-friendly, this is just not implemented.

Supported compilers:

 - Mininum: GCC 10, recommended: GCC 11.
 - Minimum: Clang 10, recommended: Clang 13.
 - Minimum: MSVC 14.30 (compiler 19.29).


## Example

This is the same example code as in the [_Catch2_ tutorials](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md):

```c++
#include <snitch/snitch.hpp>

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE("Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(0) == 1 ); // this check will fail
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}
```

Output:

![Example console output of a regular test](https://user-images.githubusercontent.com/2236577/196043565-531635c5-64e0-401c-8ff6-a533c9bbbf11.png)

And here is an example code for a typed test, also borrowed (and adapted) from the [_Catch2_ tutorials](https://github.com/catchorg/Catch2/blob/devel/docs/test-cases-and-sections.md#type-parametrised-test-cases):

```c++
#include <snitch/snitch.hpp>

using MyTypes = snitch::type_list<int, char, float>; // could also be std::tuple; any template type list will do
TEMPLATE_LIST_TEST_CASE("Template test case with test types specified inside snitch::type_list", "[template][list]", MyTypes)
{
    REQUIRE(sizeof(TestType) > 1); // will fail for 'char'
}
```

Output:

![Example console output of a typed test](https://user-images.githubusercontent.com/2236577/196043558-ed9ab329-5934-4bb3-a422-b48d6781cf96.png)


## Example build configurations with CMake

### Using _snitch_ as a regular library

Here is an example CMake file to download _snitch_ and define a test application:

```cmake
include(FetchContent)

FetchContent_Declare(snitch
                     GIT_REPOSITORY https://github.com/snitch-org/snitch.git
                     GIT_TAG        v1.2.0) # update version number as needed
FetchContent_MakeAvailable(snitch)

set(YOUR_TEST_FILES
  # add your test files here...
  )

add_executable(my_tests ${YOUR_TEST_FILES})
target_link_libraries(my_tests PRIVATE snitch::snitch)
```

_snitch_ will provide the definition of `main()` [unless otherwise specified](#using-your-own-main-function).


### Using _snitch_ as a header-only library

Here is an example CMake file to download _snitch_ and define a test application:

```cmake
include(FetchContent)

set(SNITCH_HEADER_ONLY 1)

FetchContent_Declare(snitch
                     GIT_REPOSITORY https://github.com/snitch-org/snitch.git
                     GIT_TAG        v1.2.0) # update version number as needed
FetchContent_MakeAvailable(snitch)

set(YOUR_TEST_FILES
  # add your test files here...
  )

add_executable(my_tests ${YOUR_TEST_FILES})
target_link_libraries(my_tests PRIVATE snitch::snitch-header-only)
```

One (and only one!) of your test files needs to include _snitch_ as:
```c++
#define SNITCH_IMPLEMENTATION
#include <snitch_all.hpp>
```

See the documentation for the [header-only build](#header-only-build) for more information. This will include the definition of `main()` [unless otherwise specified](#using-your-own-main-function).


## Example build configuration with meson

First, [meson build](https://mesonbuild.com/) needs a [`subprojects`](https://mesonbuild.com/Subprojects.html#using-a-subproject) directory in your project source root for dependencies. Create this directory if it does not exist, then, from within your project source root, run:

```bash
> meson wrap install snitch
```

This downloads a [_wrap file_](https://mesonbuild.com/Wrap-dependency-system-manual.html#wrap-format), `snitch.wrap`, from [WrapDB](https://mesonbuild.com/Wrapdb-projects.html)
to the `subprojects` directory. A `[provide]` section declares `snitch = snitch_dep`,
and that guides meson's [`wrap` dependency system](https://mesonbuild.com/Wrap-dependency-system-manual.html) to use a _snitch_ install, if found, or to download _snitch_ as a fallback.

The provided `snitch_dep` dependency is retrieved and used in a `meson.build` script, e.g.:

```python
snitch_dep = dependency('snitch')

test('mytest', executable('test','test.cpp',dependencies:snitch_dep) )
```

Alternatively, you can `git clone` _snitch_ directly to `subprojects/snitch`. A wrap file is then optional. You can retrieve the dependency directly (as is necessary in meson < v0.54):

```python
snitch_dep = subproject('snitch').get_variable('snitch_dep')
```

If you use _snitch_ only as a [header-only library](#header-only-build)
then you can disable the library build by configuring with:

- `-D snitch:create_library=false`

Otherwise, if you use _snitch_ only as a regular library,
then you can configure with:

- `-D snitch:create_header_only=false`

And this disables the build step that generates the single-header file "`snitch_all.hpp`".


## Example build configuration with vcpkg

See [the dedicated page in the docs folder](doc/vcpkg-example/README.md).


## Benchmark

The following benchmarks were done using real-world tests from another library ([_observable_unique_ptr_](https://github.com/cschreib/observable_unique_ptr)), which generates about 4000 test cases and 25000 checks. This library uses "typed" tests almost exclusively, where each test case is instantiated several times, each time with a different tested type (here, 25 types). Building and running the tests was done without parallelism to simplify the comparison. The benchmarks were run on a desktop with the following specs:
 - OS: Linux Mint 21.2, linux kernel 6.2.0-26-generic.
 - CPU: AMD Ryzen 5 2600 (6 core).
 - RAM: 16GB.
 - Storage: NVMe.
 - Compiler: GCC 10.5.0 with `-std=c++20`.

The benchmark tests can be found in different branches of _observable_unique_ptr_:
 - _snitch_: https://github.com/cschreib/observable_unique_ptr/tree/snitch
 - _Catch2_ v3.4.0: https://github.com/cschreib/observable_unique_ptr/tree/catch2
 - _doctest_ v2.4.11: https://github.com/cschreib/observable_unique_ptr/tree/doctest
 - _Boost.UT_ v1.1.9: https://github.com/cschreib/observable_unique_ptr/tree/boost_ut

Description of results below:
 - *Build framework*: Time required to build the testing framework library (if any), without any test.
 - *Build tests*: Time required to build the tests, assuming the framework library (if any) was already built.
 - *Build all*: Total time to build the tests and the framework library (if any).
 - *Run tests*: Total time required to run the tests.
 - *Library size*: Size of the compiled testing framework library (if any).
 - *Executable size*: Size of the compiled test executable, static linking to the testing framework library (if any).

Results for Debug builds:

| **Debug**       | _snitch_ | _Catch2_ | _doctest_ | _Boost UT_ |
|-----------------|----------|----------|-----------|------------|
| Build framework | 4.2s     | 42s      | 2.1s      | 0s         |
| Build tests     | 70s      | 75s      | 76s       | 117s       |
| Build all       | 74s      | 117s     | 78s       | 117s       |
| Run tests       | 44ms     | 67ms     | 63ms      | 14ms       |
| Library size    | 9.2MB    | 33.5MB   | 2.8MB     | 0MB        |
| Executable size | 37.0MB   | 47.7MB   | 38.6MB    | 51.8MB     |

Results for Release builds:

| **Release**     | _snitch_ | _Catch2_ | _doctest_ | _Boost UT_ |
|-----------------|----------|----------|-----------|------------|
| Build framework | 5.7s     | 48s      | 3.7s      | 0s         |
| Build tests     | 146s     | 233s     | 210s      | 289s       |
| Build all       | 152s     | 281s     | 214s      | 289s       |
| Run tests       | 26ms     | 37ms     | 42ms      | 5ms        |
| Library size    | 1.4MB    | 2.5MB    | 0.39MB    | 0MB        |
| Executable size | 10.2MB   | 17.4MB   | 15.5MB    | 11.4MB     |

Notes:
 - No attempt was made to optimize each framework's configuration; the defaults were used. C++20 modules were not used.
 - _Boost UT_ was unable to compile and pass the tests without modifications to its implementation (issues were reported).


## Documentation

### Detailed comparison with _Catch2_

See [the dedicated page in the docs folder](doc/comparison_catch2.md) for a breakdown of _Catch2_ features and their implementation status in _snitch_.

Given that _snitch_ mostly offers a subset of the _Catch2_ API, why would anyone want to use it over _Catch2_?

 - _snitch_ does not do any heap allocation while running tests. This is important if the tests need to monitor the global heap usage, to ensure that the tested code only allocates what it is supposed to (or not at all). This is tricky to do with _Catch2_, since some check macros will trigger heap allocations by using `std::string` and other heap-allocated data structures. To add to the confusion, some `std::string` instances used by _Catch2_ will fall under the small-string-optimization threshold, and won't generate heap allocations on some implementations of the C++ STL. This makes any measurement of heap usage not only noisy, but platform-dependent. If this is a concern to you, then _snitch_ is a better choice.

 - _snitch_ has a much smaller compile-time footprint than _Catch2_, see the benchmarks above. If your tested code is very cheap to compile, but you have a large number of tests and/or assertions, your compilation time may be dominated by the testing framework implementation (this is the case in the benchmarks). If the compilation time with _Catch2_ becomes prohibitive or annoying, you can give _snitch_ a try to see if it improves it.

 - _snitch_ can be used as a header-only library. This may be relevant for very small projects, or projects that do not use one of the supported build systems. Note however that doing so will likely nullify any compile-time advantage over alternative testing libraries; this is not recommended if compilation time is a concern.

 - _snitch_ has better reporting of typed tests (template test cases). While _Catch2_ will only report the type index in the test type list, _snitch_ will actually report the type name. This makes it easier to find which type generated a failure.

 - _snitch_ is able to test and decompose expressions both at run-time and compile-time in the same build (see `CONSTEXPR_CHECK`). _Catch2_ on the other hand is only able to test at compile-time (but not decompose) in one build, and test and decompose at run-time in a different build (using `STATIC_CHECK`).

If none of the above applies, then _Catch2_ will generally offer more value.


### Test case macros

#### Standalone test cases

`TEST_CASE(NAME, TAGS) { /* test body */ }`

This must be called at namespace, global, or class scope; not inside a function or another test case. This defines a new test case of name `NAME`. `NAME` must be a string literal, and may contain any character, up to a maximum length configured by `SNITCH_MAX_TEST_NAME_LENGTH` (default is `1024`). This name will be used to display test reports, and can be used to filter the tests. It is not required to be a unique name. `TAGS` specify which tag(s) are associated with this test case. This must be a string literal with the same limitations as `NAME`. See the [Tags](#tags) section for more information on tags. Finally, `test body` is the body of your test case. Within this scope, you can use the test macros listed [below](#test-check-macros).


`TEMPLATE_TEST_CASE(NAME, TAGS, TYPES...) { /* test code for TestType */ }`

This is similar to `TEST_CASE`, except that it declares a new test case for each of the types listed in `TYPES...`. Within the test body, the current type can be accessed as `TestType`. The full name of the test, used when filtering tests by name, is `"NAME <TYPE>"`. If you tend to reuse the same list of types for multiple test cases, then `TEMPLATE_LIST_TEST_CASE()` is recommended instead.


`TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) { /* test code for TestType */ }`

This is equivalent to `TEMPLATE_TEST_CASE`, except that `TYPES` must be a template type list of the form `T<Types...>`, for example `snitch::type_list<Types...>` or `std::tuple<Types...>`. This type list can be declared once and reused for multiple test cases.


#### Test cases with fixtures

`TEST_CASE_METHOD(FIXTURE, NAME, TAGS) { /* test body */ }`

This is similar to `TEST_CASE`, except that the test body is interpreted "as if" it was a member function of a class deriving from the `FIXTURE` class. This means the test body has access to public and protected members of `FIXTURE`, but not to private members. Each time the test is executed, a new instance of `FIXTURE` is created, the test body is run on this temporary instance, and finally the instance is destroyed; the instance is not shared between tests.


`TEMPLATE_TEST_CASE_METHOD(NAME, TAGS, TYPES...) { /* test code for TestType */ }`

This is similar to `TEST_CASE_METHOD`, except that it declares a new test case for each of the types listed in `TYPES...`. Within the test body, the current type can be accessed as `TestType`. If you tend to reuse the same list of types for multiple test cases, then `TEMPLATE_LIST_TEST_CASE_METHOD()` is recommended instead.


`TEMPLATE_LIST_TEST_CASE_METHOD(NAME, TAGS, TYPES) { /* test code for TestType */ }`

This is equivalent to `TEMPLATE_TEST_CASE_METHOD`, except that `TYPES` must be a template type list of the form `T<Types...>`, for example `snitch::type_list<Types...>` or `std::tuple<Types...>`. This type list can be declared once and reused for multiple test cases.


### Test check macros

The following macros can only be used inside a test body, either immediately in the body itself, or inside a function called by the test. They _cannot_ be used if a test is not running (e.g.,  they cannot be used as generic assertion macros).


#### Run-time

The macros in this section evaluate their operands are run-time exclusively.

`REQUIRE(EXPR);`

This evaluates the expression `EXPR`, as in `if (EXPR)`, and reports a failure if `EXPR` evaluates to `false`. On failure, the current test case is stopped. Execution then continues with the next test case, if any. The value of each operand of the expression will be displayed on failure, provided the types involved can be serialized to a string. See [Custom string serialization](#custom-string-serialization) for more information. If one of the operands is a [matcher](#matchers) and the operation is `==`, then this will report a failure if there is no match. Conversely, if the operation is `!=`, then this will report a failure if there is a match.


`CHECK(EXPR);`

This is similar to `REQUIRE`, except that on failure the test case continues. Further failures may be reported in the same test case.


`REQUIRE_FALSE(EXPR);`

This is equivalent to `REQUIRE(!(EXPR))`, except that it is able to decompose `EXPR` (otherwise, the `!(...)` forces evaluation of the expression, which then cannot be decomposed).


`CHECK_FALSE(EXPR);`

This is equivalent to `CHECK(!(EXPR))`, except that it is able to decompose `EXPR` (otherwise, the `!(...)` forces evaluation of the expression, which then cannot be decomposed).


`REQUIRE_THAT(EXPR, MATCHER);`

This is equivalent to `REQUIRE(EXPR == MATCHER)`, and is provided for compatibility with _Catch2_.


`CHECK_THAT(EXPR, MATCHER);`

This is equivalent to `CHECK(EXPR == MATCHER)`, and is provided for compatibility with _Catch2_.


#### Compile-time

The macros in this section evaluate their operands are compile-time exclusively. To benefit from the run-time infrastructure of _snitch_ (allowed failures, custom reporter, etc.), the test report is still generated at run-time. However, if the operands cannot be evaluated at compile-time, a compiler error will be generated.

These macros are recommended for testing `consteval` functions, which are always evaluated at compile-time. For `constexpr` functions, which can be evaluated both at compile-time and run-time, prefer the `CONSTEXPR_*` macros described below.

`CONSTEVAL_REQUIRE(EXPR);`

Same as `REQUIRE(EXPR)` but with operands evaluated at compile-time.

`CONSTEVAL_CHECK(EXPR);`

Same as `CHECK(EXPR)` but with operands evaluated at compile-time.

`CONSTEVAL_REQUIRE_FALSE(EXPR);`

Same as `REQUIRE_FALSE(EXPR)` but with operands evaluated at compile-time.

`CONSTEVAL_CHECK_FALSE(EXPR);`

Same as `CHECK_FALSE(EXPR)` but with operands evaluated at compile-time.

`CONSTEVAL_REQUIRE_THAT(EXPR, MATCHER);`

Same as `REQUIRE_THAT(EXPR, MATCHER)` but with operands evaluated at compile-time.

`CONSTEVAL_CHECK_THAT(EXPR, MATCHER);`

Same as `CHECK_THAT(EXPR, MATCHER)` but with operands evaluated at compile-time.


#### Run-time and compile-time

The macros in this section evaluate their operands both are compile-time and at run-time. To benefit from the run-time infrastructure of _snitch_ (allowed failures, custom reporter, etc.), the test report is still generated at run-time regardless of the above. However, if the operands cannot be evaluated at compile-time, a compiler error will be generated.

These macros are recommended for testing `constexpr` functions, which can be evaluated both at compile-time and at run-time. Since the operands are also evaluated at run-time, the test will contribute to the coverage analysis (if any), which is otherwise impossible for purely compile-time tests (e.g., `CONSTEVAL_*` macros above).

`CONSTEXPR_REQUIRE(EXPR);`

Same as `REQUIRE(EXPR)` but with operands evaluated both at compile-time and run-time.

`CONSTEXPR_CHECK(EXPR);`

Same as `CHECK(EXPR)` but with operands evaluated both at compile-time and run-time.

`CONSTEXPR_REQUIRE_FALSE(EXPR);`

Same as `REQUIRE_FALSE(EXPR)` but with operands evaluated both at compile-time and run-time.

`CONSTEXPR_CHECK_FALSE(EXPR);`

Same as `CHECK_FALSE(EXPR)` but with operands evaluated both at compile-time and run-time.

`CONSTEXPR_REQUIRE_THAT(EXPR, MATCHER);`

Same as `REQUIRE_THAT(EXPR, MATCHER)` but with operands evaluated both at compile-time and run-time.

`CONSTEXPR_CHECK_THAT(EXPR, MATCHER);`

Same as `CHECK_THAT(EXPR, MATCHER)` but with operands evaluated both at compile-time and run-time.


#### Exception checks

`REQUIRE_THROWS_AS(EXPR, EXCEPT);`

This evaluates the expression `EXPR` inside a `try/catch` block, and attempts to catch an exception of type `EXCEPT`. If no exception is thrown, or an exception of a different type is thrown, then this reports a test failure. On failure, the current test case is stopped. Execution then continues with the next test case, if any.


`CHECK_THROWS_AS(EXPR, EXCEPT);`

This is similar to `REQUIRE_THROWS_AS`, except that on failure the test case continues. Further failures may be reported in the same test case.


`REQUIRE_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`

This is similar to `REQUIRE_THROWS_AS`, but further checks the content of the exception that has been caught. The caught exception is given to the matcher object specified in `MATCHER` (see [Matchers](#matchers)). If the exception object is not a match, then this reports a test failure.


`CHECK_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`

This is similar to `REQUIRE_THROWS_MATCHES`, except that on failure the test case continues. Further failures may be reported in the same test case.


`REQUIRE_NOTHROW(EXPR);`

This evaluates the expression `EXPR` inside a `try/catch` block. If an exception is thrown, then this reports a test failure. On failure, the current test case is stopped. Execution then continues with the next test case, if any.


`CHECK_NOTHROW(EXPR);`

This is similar to `REQUIRE_NOTHROW`, except that on failure the test case continues. Further failures may be reported in the same test case.


#### Miscellaneous

`FAIL(MSG);`

This reports a test failure with the message `MSG`. The current test case is stopped. Execution then continues with the next test case, if any.


`FAIL_CHECK(MSG);`

This is similar to `FAIL`, except that the test case continues. Further failures may be reported in the same test case.


`SKIP(MSG);`

This reports the current test case as "skipped". Any previously reported status for this test case is ignored. The current test case is stopped. Execution then continues with the next test case, if any.


`SKIP_CHECK(MSG);`

This is similar to `SKIP`, except that the test case continues. Further failure will not be reported. This is only recommended as an alternative to `SKIP()` when exceptions cannot be used.


### Advanced API

`snitch::notify_exception_handled();`

If handling exceptions explicitly with a `try/catch` block in a test case, this should be called at the end of the `catch` block. This clears up internal state that would have been used to report that exception, had it not been handled. Calling this is not strictly necessary in most cases, but omitting it can lead to confusing contextual data (incorrect section/capture/info) if another exception is thrown afterwards and not handled.


### Tags

Tags are assigned to each test case using the [Test case macros](#test-case-macros), as a single string. Within this string, individual tags must be surrounded by square brackets, with no white-space between tags (although white space within a tag is allowed). For example:

```c++
TEST_CASE("test", "[tag1][tag 2][some other tag]") {
    //             ^---- these are the tags ---^
}
```

Tags can be used to filter the tests, for example, by running all tests with a given tag. There are also a few "special" tags recognized by _snitch_, which change the behavior of the test:
 - `[.]` is the "hidden" tag; any test with this tag will be excluded from the default list of tests. The test will only be run if selected explicitly, either when filtering by name, or by tag.
 - `[.<some tag>]` is a shortcut for `[.][<some_tag>]`.
 - `[!mayfail]` indicates that the test may fail; if so, any failure will be recorded, but the test case will still be marked as passed.
 - `[!shouldfail]` indicates that the test must fail; any failure will be recorded, but the test case will still be marked as passed. If no failure is recorded, the test is marked as failed.


### Matchers

Matchers in _snitch_ work differently than in _Catch2_. Matchers do not need to inherit from a common base class. The only required interface is:

 - `matcher.match(obj)` must return `true` if `obj` is a match, `false` otherwise.
 - `matcher.describe_match(obj, status)` must return a value convertible to `std::string_view`, describing why `obj` is or is not a match, depending on the value of `snitch::matchers::match_status`.
 - `matcher == obj` and `obj == matcher` must return `matcher.match(obj)`, and `matcher != obj` and `obj != matcher` must return `!matcher.match(obj)`; any matcher defined in the `snitch::matchers` namespace will have these operators defined automatically.

The following matchers are provided with _snitch_:

 - `snitch::matchers::contains_substring{"substring"}`: accepts a `std::string_view`, and will return a match if the string contains `"substring"`.
 - `snitch::matchers::with_what_contains{"substring"}`: accepts a `std::exception`, and will return a match if `what()` contains `"substring"`.
 - `snitch::matchers::is_any_of{T...}`: accepts an object of any type `T`, and will return a match if it is equal to any of the `T...`.


Here is an example matcher that, given a prefix `p`, checks if a string starts with the prefix `"<p>:"`:
```c++
namespace snitch::matchers {
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
} // namespace snitch::matchers
```

_snitch_ will always call `match()` before calling `describe_match()`. Therefore, you can save any intermediate calculation performed during `match()` as a member variable, to be reused later in `describe_match()`. This can prevent duplicating effort, and can be important if calculating the match is an expensive operation.


### Sections

As in _Catch2_, _snitch_ supports nesting multiple tests inside a single test case, to share set-up/tear-down logic. This is done using the `SECTION("name")` macro. Please see the [Catch2 documentation](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#test-cases-and-sections) for more details. Note: if any exception is thrown inside a section, or if a `REQUIRE()` check fails (or any other check which aborts execution), the whole test case is stopped. No other section will be executed.

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
}
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

As in _Catch2_, _snitch_ supports capturing contextual information to be displayed in the test failure report. This can be done with the `INFO(message)` and `CAPTURE(vars...)` macros. The captured information is "scoped", and will only be displayed for failures happening:
 - after the capture, and
 - in the same scope (or deeper).

For example, in the test below we compute a complicated formula in a `CHECK()`:

```c++
#include <cmath>

TEST_CASE("test without captures", "[captures]") {
    for (std::size_t i = 0; i < 10; ++i) {
        CHECK(std::abs(std::cos(i * 3.14159 / 10)) > 0.4);
    }
}
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
}
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
}
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

The only requirement is that the captured variable or expression must be of a type that _snitch_ can serialize to a string. See [Custom string serialization](#custom-string-serialization) for more information.

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
}
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

When the _snitch_ framework needs to serialize a value to a string, it does so with the free function `append(span, value)`, where `span` is a `snitch::small_string_span`, and `value` is the value to serialize. The function must return a boolean, equal to `true` if the serialization was successful, or `false` if there was not enough room in the output string to store the complete textual representation of the value. On failure, it is recommended to write as many characters as possible, and just truncate the output; this is what built-in functions do.

Built-in serialization functions are provided for all fundamental types: integers, enums (serialized as their underlying integer type), floating point, booleans, standard `string_view` and `char*`, and raw pointers.

If you want to serialize custom types not supported out of the box by _snitch_, you need to provide your own `append()` function. This function must be placed in the same namespace as your custom type or in the `snitch` namespace, so it can be [found by ADL (argument-dependent lookup)](https://en.cppreference.com/w/cpp/language/adl). ADL rules can be complex to follow, so if in doubt, simply define your `append()` function in the `snitch` namespace.

In most cases, the `append()` function can be written in terms of serialization of fundamental types which are already supported by _snitch_, and therefore won't require low-level string manipulation. For example, to serialize a structure representing the 3D coordinates of a point:

```c++
namespace my_namespace {
    struct vec3d {
        float x;
        float y;
        float z;
    };

    bool append(snitch::small_string_span ss, const vec3d& v) {
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

    bool append(snitch::small_string_span ss, const MyClass& c) {
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

Note that _snitch_ small strings have a fixed capacity; once this capacity is reached, the string cannot grow further, and the output must be truncated. This will normally be indicated by a `...` at the end of the strings being reported (this is automatically added by _snitch_; you do not need to do this yourself). If this happens, depending on which string was truncated, there are a number of compilation options that can be modified to increase the maximum string length. See `CMakeLists.txt`, or `snitch_config.hpp`, or the top of `snitch_all.hpp`, for a complete list.


### Reporters

By default, _snitch_ will report the test results to the standard output, using its own report format. There are two ways you can override this:
 - Register a new reporter with `REGISTER_REPORTER(...)` and select it from the command-line. This is more flexible as you can change which reporter to use without re-compiling, but it requires a bit more boilerplate. See [Registering a new reporter](#registering-a-new-reporter). A list of standard reporters is provided with _snitch_ and enabled by default; see [Built-in reporters](#built-in-reporters).
 - Override the default reporter by directly supplying your own callback function to the test registry. This is simpler but requires [using your own main function](#using-your-own-main-function), and is only a good option if the reporter never needs to change. See [Overriding the default reporter](#overriding-the-default-reporter).

In both cases, the core of the reporter is its "report" callback function. It is a `noexcept` function, taking two arguments:
 - a reference to the `snitch::registry` that generated the event
 - a reference to the `snitch::event::data` containing the event data. This type is a `std::variant`; use `std::visit` to act on the event.

Most events are generated during the course of a normal test run. The only exceptions are `list_test_run_started`, `list_test_run_ended`, and `test_case_listed`, which are generated when listing tests (`--list-tests` option).

When receiving a test event, the event object will only contain non-owning references (e.g., in the form of string views) to the actual event data. These references are only valid until the report function returns; after this, the event data will be destroyed or overwritten. If you need persistent access to this data (e.g., because your reporting format requires reporting the data at a different time than when the event is generated), you must explicitly copy the relevant data, and not the references. For example, for strings, this could involve creating a `std::string` (or `snitch::small_string`) from the `std::string_view` stored in the event object.

Finally, note that events being sent to the reporter are affected by the chosen verbosity:
 - `quiet`: `assertion_failed`, `test_case_skipped`, `list_test_run_started`, `list_test_run_ended`, and `test_case_listed` only.
 - `normal`: same as `quiet`, plus `test_run_started` and `test_run_ended`.
 - `high`: same as `normal`, plus `test_case_started` and `test_case_ended`.
 - `full`: same as `high`, plus `assertion_succeeded` (i.e., all events).

It may be necessary to override the default verbosity when the reporter is initialized if the reporter requires certain events to be sent.


#### Built-in reporters

With the default build configuration, _snitch_ provides the following built-in reporters. They can all be disabled by turning off the CMake option `SNITCH_WITH_ALL_REPORTERS` or Meson option `with_all_reporters`, then enabled individually with specific build options if desired.
 - `console`: This is the default reporter, always present.
 - `teamcity`: Reports events in a format suitable for JetBrains TeamCity.
 - `xml`: Reports events in the _Catch2_ XML format. Provided for compatibility with _Catch2_.


#### Overriding the default reporter

The default reporter callback can be registered either as a free function, a stateless lambda, or a member function. This is the reporter that is used if no `--reporter` option is passed to the command-line. You can register your own callback as follows:

```c++
// Free function.
// --------------
void report_function(const snitch::registry& r, const snitch::event::data& e) noexcept {
    /* ... */
}

snitch::tests.report_callback = &report_function;

// Stateless lambda (no captures).
// -------------------------------
snitch::tests.report_callback = [](const snitch::registry& r, const snitch::event::data& e) noexcept {
    /* ... */
};

// Stateful lambda (with captures).
// -------------------------------
auto lambda = [&](const snitch::registry& r, const snitch::event::data& e) noexcept {
    /* ... */
};

// 'lambda' must remain alive for the duration of the tests!
snitch::tests.report_callback = lambda;

// Member function (const or non-const, up to you).
// ------------------------------------------------
struct Reporter {
    void report(const snitch::registry& r, const snitch::event::data& e) /*const*/ noexcept {
        /* ... */
    }
};

Reporter reporter; // must remain alive for the duration of the tests!

snitch::tests.report_callback = {reporter, snitch::constant<&Reporter::report>{}};
```

If you need to use a reporter member function, please make sure that the reporter object remains alive for the duration of the tests (e.g., declare it static, global, or as a local variable declared in `main()`), or make sure to de-register it when your reporter is destroyed.


#### Registering a new reporter

There are two macros available to register a new reporter: `REGISTER_REPORTER` and `REGISTER_REPORTER_CALLBACKS`. The former registers a reporter class or struct, and is useful for stateful reporters. The latter registers a reporter as a series of callback functions, which only need defining as needed. Both offer the same functionality, and you can simply choose the one that is most convenient for you.

`REGISTER_REPORTER(NAME, TYPE);`

This must be called at namespace, global, or class scope; not inside a function or another test case. This registers a new reporter with name `NAME` (which is used to select it from the command-line), and type `TYPE`. The type must define:
 - A constructor taking a `snitch::registry&`, called when the reporter is selected.
 - A `bool configure(snitch::registry&, std::string_view k, std::string_view v)` member function, called for each reporter option from the command-line. It is called once for each of the options provided on the command-line, with `k` the name of the option, and `v` its value. The function is expected to return `false` if the option was unknown, and `true` otherwise.
 - A `void report(const snitch::registry&, const snitch::event::data&)` member function. It is the main report callback, and should be implemented as described in the [Reporters](#reporters) section.

An example can be found in [`include/snitch_catch2_xml.hpp`](include/snitch_catch2_xml.hpp) / [`src/snitch_catch2_xml.cpp`](src/snitch_catch2_xml.cpp).


`REGISTER_REPORTER_CALLBACKS(NAME, INIT, CONFIG, REPORT, FINISH);`

This is similar to `REGISTER_REPORTER`, but takes four separate callback functions instead of a single type as argument. The four callback functions are:
 - `INIT` has signature `void(snitch::registry& r) noexcept` and is used to initialize the reporter. It is called once when the reporter is selected.
 - `CONFIG` has signature `bool(snitch::registry& r, std::string_view k, std::string_view v) noexcept` and is used to configure the reporter. It is called once for each of the options provided on the command-line, with `k` the name of the option, and `v` its value. The function is expected to return `false` if the option was unknown, and `true` otherwise.
 - `REPORT` has signature `void(const snitch::registry& r, const snitch::event::data& e) noexcept`. It is the main report callback, as described in [Reporters](#reporters).
 - `FINISH` has signature `void(snitch::registry& r) noexcept` and is used to close the reporter. It is called once when the tests are finished running.

All callback functions are optional except `REPORT`. If a callback is unused, simply specify the function as `{}`. Otherwise, please refer to [Overriding the default reporter](#overriding-the-default-reporter) for instructions on how to specify your own callback functions.

An example can be found in [`include/snitch_reporter_teamcity.hpp`](include/snitch_reporter_teamcity.hpp) / [`src/snitch_reporter_teamcity.cpp`](src/snitch_reporter_teamcity.cpp).


### Output colors

_snitch_ is able to use color codes when outputting text to the console. These help with readability, but only when the output is printed directly into a terminal that supports color codes. If the chosen output target does not support color codes (which includes in particular the Windows command prompt, outputting to a file, or some CI frameworks), the output will contain gibberish symbols, e.g., `[1;31merror:[0m missing ...`, hence color codes should be disabled. There are two ways to do this:

 1. At build-time using `-DSNITCH_DEFAULT_WITH_COLOR=on/off` (CMake) or `-Dsnitch:default_with_color=true/false` (meson). This selects whether color codes are used or not when no specific command-line option is provided to the test executable. This is enabled by default, but you can turn it off if your typical output targets do not support color codes.
 2. At run-time using the `--color` (or `--colour-mode`) command-line option (see [the command-line API](#command-line-api) for more information). This allows enabling and disabling color codes for each test run, without rebuilding the tests. This is more useful if your workflow involves some targets which support color codes, and others that do not.


### Command-line API

_snitch_ offers the following command-line API:
 - positional arguments for filtering tests by name, see next section.
 - `-h,--help`: show command-line help.
 - `-l,--list-tests`: list all tests.
 - `   --list-tags`: list all tags.
 - `   --list-tests-with-tag`: list all tests with a given tag.
 - `   --list-reporters`: list all registered reporters.
 - `-r,--reporter <reporter[::key=value]*>`: choose which reporter to use to output the test events.
 - `-v,--verbosity <quiet|normal|high|full>`: select level of detail for test events.
 - `-o,--output <path>`: save test output to a file rather than the standard output.
 - `   --color <always|default|never>`: enable/disable colors in the default reporter.

The following options are provided for compatibility with _Catch2_:
 - `   --colour-mode <ansi|default|none>`: enable/disable colors in the default reporter.


### Selecting which tests to run

The command-line arguments (other than options starting with `--`) are used to select which tests to run. If no positional argument is given, all test cases will be run, except those that are explicitly hidden with special tags (see [Tags](#tags), and see also the note below on filtering hidden tests). Otherwise, each argument is a "filter" that is applied to the list of test cases.

A filter may contain any number of "wildcard" character, `*`, which can represent zero or more characters. For example:
 - `ab*` will include all test cases with names starting with `ab`.
 - `*cd` will include all test cases with names ending with `cd`.
 - `ab*cd` will include all test cases with names starting with `ab` and ending with `cd`.
 - `abcd` will only include the test case with name `abcd`.
 - `*` will include all test cases.

If a filter starts with `~`, the meaning of the filter is negated. For example `~ab*` will include all test cases with name not starting with `ab`.

A filter can contain white spaces, however be mindful that your shell will require the filter to be surrounded by quotes to treat it as a single command-line argument (e.g., `./snitch_app "some test"`).

By default, a filter applies to the test case name (which includes the test type for templated tests, using the format `name <type>`). However, if a filter starts with `[` or `~[`, then it applies to the test case tags instead. This behavior can be bypassed by escaping the bracket `\[`, in which case the filter applies to the test case name again (see note below on escaping).

Finally, if multiple filters are provided, they are combined using the following logic:
 - When provided as separate command-line arguments, e.g., `"<filter1>" "<filter2>"`, the filters are combined with an "AND" operation (tests must match both filters to be selected).
 - When provided as a single comma-separated command-line argument, e.g., `"<filter1>,<filter2>"`, the filters are combined with an "OR" operation (tests must match either of the filters to be selected).
 - For tag filters only, when multiple tags are specified in the same command-line argument, e.g., `"[<filter1>][<filter2>]"`, the tag filters are combined with an "AND" operation (test tags must match both filters to be selected).

Name and tag filters can be used in any combination. To summarize, here are some examples with the equivalent C++ boolean logic (where `f*` represents a filter):

| CLI test filters  | C++ boolean equivalent        |
|-------------------|-------------------------------|
| `f`               | `f`                           |
| `~f`              | `!f`                          |
| `f1 f2`           | `f1 && f2`                    |
| `f1 f2 f3 ...`    | `f1 && f2 && f3 && ...`       |
| `f1,f2`           | `f1 \|\| f2`                  |
| `f1,f2,f3,...`    | `f1 \|\| f2 \|\| f3 \|\| ...` |
| `f1,f2 f3`        | `(f1 \|\| f2) && f3`          |

**Note 1:** To match the actual characters `*`, `,`, `[`, `]`, or `\` in a test name, the character in the filter must be escaped using a backslash, like `\*`. In general, any character located after a single backslash will be interpreted as a regular character, with no special meaning. Be mindful that most shells (Bash, etc.) will also require the backslash itself be escaped to be interpreted as an actual backslash in _snitch_. The table below shows examples of how edge-cases are handled:

| Bash    | _snitch_ | matches                                     |
|---------|----------|---------------------------------------------|
| `\\`    | `\`      | nothing (ill-formed filter)                 |
| `\\*`   | `\*`     | any name which is exactly the `*` character |
| `\\\\`  | `\\`     | any name which is exactly the `\` character |
| `\\\\*` | `\\*`    | any name starting with the `\` character    |
| `[a*`   | `[a*`    | any tag starting with `[a`                  |
| `\\[a*` | `\[a*`   | any name starting with `[a`                 |

**Note 2:** Hidden test cases are treated differently from normal test cases. For a hidden test to be run, it must be *explicitly included* with the chosen filters. This means that the test case a) must not have been excluded by any filter, and b) must have matched at least one non-negated filter. For example, if a hidden test is named `abc`, it will not be run with the filter `~b*` ("all tests except those starting with `b`") even though its name would be a match; it was only matched "implicitly", by not being excluded. It will, however, be run with the filter `a*` ("all tests starting with `a`"), since this is an explicit match. This is somewhat subtle, but prevents more confusing results. If in doubt, hidden test cases can always be explicitly selected with the `[.]` filter tag, and explicitly excluded with the `~[.]` filter tag.


### Using your own main function

By default _snitch_ defines `main()` for you. To prevent this and provide your own `main()` function, when compiling _snitch_, `SNITCH_DEFINE_MAIN` must be set to `0`.

If using the header-only mode, this can be done in the file that defines the _snitch_ implementation:

```c++
#define SNITCH_IMPLEMENTATION
#define SNITCH_DEFINE_MAIN 0
#include <snitch_all.hpp>
```

If using CMake, this can be done by setting the option just before calling `FetchContent_Declare()`:

```cmake
set(SNITCH_DEFINE_MAIN OFF)
```

If using meson, then you can configure with `-D snitch:define_main=false`.

Here is a recommended `main()` function that replicates the default behavior of snitch:

```c++
int main(int argc, char* argv[]) {
    // Parse the command-line arguments.
    std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
    if (!args) {
        // Parsing failed, an error has been reported, just return.
        return 1;
    }

    // Configure snitch using command-line options.
    // You can then override the configuration below, or just remove this call to disable
    // command-line options entirely.
    snitch::tests.configure(*args);

    // Your own initialization code goes here.
    // ...

    // Actually run the tests.
    // This will apply any filtering specified on the command-line.
    return snitch::tests.run_tests(*args) ? 0 : 1;
}
```


### Exceptions

By default, _snitch_ assumes exceptions are enabled, and uses them in two cases:

 1. Obviously, in test macros that check exceptions being thrown (e.g., `REQUIRE_THROWS_AS(...)`).
 2. In `REQUIRE*()`, `FAIL()`, and `SKIP()` macros, to abort execution of the current test case and continue to the next one.

If _snitch_ detects that exceptions are not available (or is configured with exceptions disabled, by setting `SNITCH_WITH_EXCEPTIONS` to `0`), then

 1. Test macros that check exceptions being thrown will not be defined.
 2. `REQUIRE*()`, `FAIL()`, and `SKIP()` macros will simply use `std::terminate()` to abort execution. Consequently, the whole test application stops and the following test cases are not executed. If this is undesirable, use the alternative macros that do not abort execution: `CHECK*()`, `FAIL_CHECK()`, and `SKIP_CHECK()`, then do the control flow yourself (e.g., return from the test case).


### Header-only build

The recommended way to use _snitch_ is to build and consume it like any other library. This provides the best incremental build times, a standard way to include and link to the _snitch_ implementation, and a cleaner separation between your code and _snitch_ code, but this also requires a bit more set up (using a build generator like CMake, meson, or some other build system).

For extra convenience, _snitch_ is also provided as a header-only library. The main header is called `snitch_all.hpp`, and can be downloaded as an artifact from each release on GitHub. It is also produced by any local CMake or meson build, so you can also use it like any other library.

With CMake, just link to `snitch::snitch-header-only` instead of `snitch::snitch`.

With meson, the `snitch_dep` dependency works for both library and header-only usage.

`snitch_all.hpp` is the only header required to use the library; other headers may be provided for convenience functions (e.g., reporters for common CI frameworks) and these must still be included separately.

To use _snitch_ as header-only in your code, simply include `snitch_all.hpp` instead of `snitch.hpp`. Then, one of your files must include the _snitch_ implementation. This can be done with a `.cpp` file containing only the following:

```c++
#define SNITCH_IMPLEMENTATION
#include <snitch_all.hpp>
```

### IDE integrations

There are no IDE integrations created specifically for _snitch_. However, since _snitch_ implements most of the _Catch2_ command-line API, _Catch2_ integrations tend to work for _snitch_ test applications as well. See in particular:
 - [Visual Studio Code](https://code.visualstudio.com/) and the [C++ TestMate](https://marketplace.visualstudio.com/items?itemName=matepek.vscode-catch2-test-adapter) extension. Tested successfully on 26/08/2023.
 - [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) and the [_Catch2_ Test Adapter](https://marketplace.visualstudio.com/items?itemName=JohnnyHendriks.ext01) extension. Tested successfully on 27/08/2023.
 - [CLion](https://www.jetbrains.com/clion/) using the builtin _Catch2_ configuration. Tested partially on 20/08/2023 (I don't have a CLion license).

Feel free to report any issues you encounter using these IDE integrations; If you would like to contribute


### `clang-format` support

With its default configuration, `clang-format` will incorrectly format code using `SECTION()` if the section is the first statement inside a test case. This is because it does not know the semantic of this macro, and by default interprets it as a declaration rather than a control statement.

Fixing this requires `clang-format` version 13 at least, and requires adding the following to your `.clang-format` file:
```yaml
IfMacros: ['SECTION', 'SNITCH_SECTION']
SpaceBeforeParens: ControlStatementsExceptControlMacros
```

## Contributing

Please refer to the separate [contributing](CONTRIBUTING.md) page.


## Code of conduct

Please refer to the separate [code of conduct](CODE_OF_CONDUCT.md) page.


## Why the name _snitch_?

Libraries and programs sometimes do shady or downright illegal stuff (i.e., bugs, crashes). _snitch_ is a library like any other; it may have its own bugs and faults. But it's a snitch! It will tell you when other libraries and programs misbehave, with the hope that you will overlook its own wrongdoings.

The logo is a jay feather. Jays are known for alerting other animals of danger.
