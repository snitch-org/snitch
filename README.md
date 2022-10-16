# snatch

Lightweight C++20 testing framework.

The goal of `snatch` is to be a simple, cheap, non-invasive, and user-friendly testing framework. The design philosophy is to keep the testing API lean, including only what is stricly necessary to present clear messages when a test fails.

## Features

 - No heap allocation from the testing framework.
 - No exceptions thrown by the testing framework (but your test code can use them and test them).
 - No external dependency; just pure C++20 with the STL.
 - Simple reporting of test results to the standard output, with coloring for readability.
 - Limited subset of the [Catch2](https://github.com/catchorg/Catch2) API, including:
   - Simple test cases with `TEST_CASE(name, tags)`.
   - Typed test cases with `TEMPLATE_LIST_TEST_CASE(name, tags, types)`.
   - Pretty-printing check macros: `REQUIRE(expr)`, `CHECK(expr)`, `FAIL(msg)`, `FAIL_CHECK(msg)`.
   - Exception checking macros: `REQUIRE_THROWS_AS(expr, except)`, `CHECK_THROWS_AS(expr, except)`, `REQUIRE_THROWS_MATCHES(expr, exception, matcher)`, `CHECK_THROWS_MATCHES(expr, except, matcher)`.
   - Additional API not in Catch2:
     - Macro to mark a test as skipped: `SKIP(msg)`.
 - Optional `main()` with simple command-line API:
     - positional argument for filtering tests by name.
     - `-h,--help`: show command line help.
     - `-l,--list-tests`: list all tests.
     - `--list-tags`: list all tags.
     - `--verbose`: turn on verbose mode.
     - `--list-tests-with-tag`: list all tests with a given tag.
     - `--tags`: filter tests by tags instead of by name.

If you need features that are not in the list above, please use Catch2.

## Example

This is the same example code as in the [Catch2 tutorials](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md):

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

And here is an example code for a typed test, also borrowed (and adapted) from the [Catch2 tutorials](https://github.com/catchorg/Catch2/blob/devel/docs/test-cases-and-sections.md#type-parametrised-test-cases):

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

| Task | Catch2 | snatch |
|------|--------|--------|
| Build framework | 48s | 0.7s |
| Build tests | 91s | 67s |
| Build tests + framework | 139s | 68s |
| Run tests | 51ms | 12ms |
