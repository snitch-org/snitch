# snatch

Lightweight C++20 testing framework.

The goal of _snatch_ is to be a simple, cheap, non-invasive, and user-friendly testing framework. The design philosophy is to keep the testing API lean, including only what is strictly necessary to present clear messages when a test fails.

<!-- MarkdownTOC autolink="true" -->

- [Features](#features)
- [Example](#example)
- [Benchmark](#benchmark)
- [Documentation](#documentation)
    - [Test case macros](#test-case-macros)
    - [Test check macros](#test-check-macros)
    - [Matchers](#matchers)
    - [Using your own main function](#using-your-own-main-function)

<!-- /MarkdownTOC -->

## Features

 - No heap allocation from the testing framework.
 - No external dependency; just pure C++20 with the STL.
 - Simple reporting of test results to the standard output, with coloring for readability.
 - Limited subset of the [_Catch2_](https://github.com/catchorg/_Catch2_) API, including:
   - Simple test cases with `TEST_CASE(name, tags)`.
   - Typed test cases with `TEMPLATE_LIST_TEST_CASE(name, tags, types)`.
   - Pretty-printing check macros: `REQUIRE(expr)`, `CHECK(expr)`, `FAIL(msg)`, `FAIL_CHECK(msg)`.
   - Exception checking macros: `REQUIRE_THROWS_AS(expr, except)`, `CHECK_THROWS_AS(expr, except)`, `REQUIRE_THROWS_MATCHES(expr, exception, matcher)`, `CHECK_THROWS_MATCHES(expr, except, matcher)`.
   - Additional API not in _Catch2_, or different from _Catch2_:
     - Macro to mark a test as skipped: `SKIP(msg)`.
     - Matchers use a different API (see [Matchers](#matchers) below).
 - Optional `main()` with simple command-line API:
     - positional argument for filtering tests by name.
     - `-h,--help`: show command line help.
     - `-l,--list-tests`: list all tests.
     - `--list-tags`: list all tags.
     - `--verbose`: turn on verbose mode.
     - `--list-tests-with-tag`: list all tests with a given tag.
     - `--tags`: filter tests by tags instead of by name.
 - Exceptions are only used to abort a test when the `REQUIRE*` macros report a failure. If you cannot use exceptions; use the `CHECK*` macros instead.

If you need features that are not in the list above, please use _Catch2_.


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

using MyTypes = std::tuple<int, char, float>;
TEMPLATE_LIST_TEST_CASE("Template test case with test types specified inside std::tuple", "[template][list]", MyTypes)
{
    REQUIRE(sizeof(TestType) > 1); // will fail for 'char'
};
```

Output:

![Screenshot from 2022-10-16 16-16-50](https://user-images.githubusercontent.com/2236577/196043558-ed9ab329-5934-4bb3-a422-b48d6781cf96.png)


## Benchmark

The following benchmark was done using tests from another library ([observable_unique_ptr](https://github.com/cschreib/observable_unique_ptr)), which generates about 4000 test cases and 25000 checks. Building and running the tests was done without parallelism to simplify the comparison. The benchmarks were ran on a desktop with the following specs:

 - OS: Linux Mint 20.3, linux kernel 5.15.0-48-generic
 - CPU: AMD Ryzen 5 2600 (6 core)
 - RAM: 16GB
 - Storage: NVMe
 - Compiler: GCC 10.3.0

Results:

| Task                    | _Catch2_ | _snatch_ |
|-------------------------|----------|----------|
| Build framework         | 48s      | 0.7s     |
| Build tests             | 91s      | 67s      |
| Build tests + framework | 139s     | 68s      |
| Run tests               | 51ms     | 12ms     |


## Documentation

### Test case macros

`TEST_CASE(NAME, TAGS) { /* test body */ };`

This must be called at namespace, global, or class scope; not inside a function or another test case. This defines a new test case of name `NAME`. `NAME` must be a string literal, and may contain any character, up to a maximum length configured by `SNATCH_MAX_TEST_NAME_LENGTH` (default is `1024`). This name will be used to display test reports, and can be used to filter the tests. It is not required to be a unique name. `TAGS` specify which tag(s) are associated with this test case. This must be a string literal with the same limitations as `NAME`. Within this string, individual tags must be surrounded by square brackets, with no white-space between tags (although white space within a tag is allowed). Tags can be used to filter the tests (e.g., run all tests with a given tag). Finally, `test body` is the body of your test case. Within this scope, you can use the test macros listed [below](#test-check-macros).

`TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) { /* test code for TestType */ };`

This is similar to `TEST_CASE`, except that it declares a new test case for each of the types listed in `TYPES`. `TYPES` must be a `std::tuple`. Within the test body, the current type can be accessed as `TestType`.


### Test check macros


`REQUIRE(EXPR);`

This evaluates the expression `EXPR`, as in `if (EXPR)`, and reports a failure if `EXPR` evaluates to `false`. On failure, the current test case is stopped. Execution then continues with the next test case, if any.


`CHECK(EXPR);`

This is similar to `REQUIRE`, except that on failure the test case continues. Further failures may be reported in the same test case.


`FAIL(MSG);`

This reports a test failure with the message `MSG`. The current test case is stopped. Execution then continues with the next test case, if any.


`FAIL_CHECK(MSG);`

This is similar to `FAIL`, except that the test case continues. Further failures may be reported in the same test case.


`SKIP(MSG);`

This reports the current test case as "skipped". Any previously reported status for this test case is ignored. The current test case is stopped. Execution then continues with the next test case, if any.


`REQUIRE_THROWS_AS(EXPR, EXCEPT);`

This evaluates the expression `EXPR` inside a `try/catch` block, and attempts to catch an exception of type `EXCEPT`. If no exception is thrown, or an exception of a different type is thrown, then this reports a test failulre. On failure, the current test case is stopped. Execution then continues with the next test case, if any.


`CHECK_THROWS_AS(EXPR, EXCEPT);`

This is similar to `REQUIRE_THROWS_AS`, except that on failure the test case continues. Further failures may be reported in the same test case.


`REQUIRE_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`

This is similar to `REQUIRE_THROWS_AS`, but further checks the content of the exception that has been caught. The caught exception is given to the matcher object specified in `MATCHER` (see [Matchers](#matchers)). If the exception object is not a match, then this reports a test failure.


`CHECK_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`

This is similar to `REQUIRE_THROWS_MATCHES`, except that on failure the test case continues. Further failures may be reported in the same test case.


### Matchers

Matchers in _snatch_ work differently than in _Catch2_. The required interface is:

 - `matcher.match(obj)` must return `true` if `obj` is a match, `false` otherwise
 - `matcher.describe_fail(obj)` must return a `std::string_view` describing why `obj` is not a match. The lifetime of the string referenced by the string view must be equal or greater than the lifetime of the matcher (e.g., the string view can point to a temporary buffer stored inside the matcher).


### Using your own main function

When compiling _snatch_, `SNATCH_DEFINE_MAIN` must be set to `0`. If using CMake, this can be done with

```cmake
set(SNATCH_DEFINE_MAIN OFF)
```

just before calling `FetchContent_Declare()`. Then, in your own main function, call

```c++
bool success = snatch::tests.run_all_tests();
```

or any other method from the `snatch::registry` class. `snatch::tests` is a global registry, which owns all registered test cases.
