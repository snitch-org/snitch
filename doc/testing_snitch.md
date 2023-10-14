# Testing _snitch_

<!-- MarkdownTOC autolink="true" -->

- [Why](#why)
- [The _snitch_ test suite](#the-snitch-test-suite)
    - [Run-time unit tests](#run-time-unit-tests)
    - [Compile-time unit tests](#compile-time-unit-tests)
    - [Approval tests](#approval-tests)
        - [How to update approval test data](#how-to-update-approval-test-data)
    - [Install tests](#install-tests)
- [How to run the tests](#how-to-run-the-tests)
    - [Running tests locally](#running-tests-locally)
        - [Run-time unit tests](#run-time-unit-tests-1)
        - [Compile-time unit tests](#compile-time-unit-tests-1)
        - [Approval tests](#approval-tests-1)
        - [Install tests](#install-tests-1)
    - [Automated testing pipeline](#automated-testing-pipeline)

<!-- /MarkdownTOC -->

## Why

If you are trying to [contribute](/CONTRIBUTING.md) to _snitch_, you should run the _snitch_ test suite to ensure your changes have not accidentally broken any existing feature. You should also ensure that any new feature you add is properly tested, so others don't break it with their changes later on.


## The _snitch_ test suite

_snitch_ has its own test suite, which contains numerous tests. This test suite is composed of:
 - [run-time unit tests](#run-time-unit-tests) (in `tests/runtime_tests`)
 - [compile-time unit tests](#compile-time-unit-tests) (not implemented yet)
 - [approval tests](#approval-tests) (in `tests/approval_tests`)
 - [install tests](#install-tests) (in `test/install_tests`)


### Run-time unit tests

The purpose of these tests is to check the detailed run-time behavior of _snitch_. This includes testing all foreseeable edge cases, happy path and sad path, etc. These tests contribute to the test coverage analysis.

The run-time unit tests are roughly organized by theme into separate files in the `tests/runtime_tests` directory. New files can be added there if necessary. There is no enforced mapping between test files and library source files. Likewise, there is no enforced mapping between test case names and tested classes / functions; this is left to the best judgment of the contributor.

Run-time unit tests come in two flavors: using _doctest_ to test _snitch_, or using _snitch_ to test itself. The latter is less safe, since a bug in _snitch_ might accidentally make these tests pass (e.g., if the test cases are not running at all), but it usually provides nicer test results on failure.

The *test* code must be written using the _snitch_ "shorthand" API (e.g., `CHECK(...)`). Wrapper macros are used to wrap _doctest_ macros so the same API can be used in both flavors. The *tested* code must be written with the _snitch_ "prefixed" API (e.g., `SNITCH_CHECK(...)`), otherwise it would turn into _doctest_ code in the _doctest_-flavored run. For example:

```c++
// Use the shorthand TEST_CASE macro to create a new test
// (this will use either snitch or doctest)
TEST_CASE("test register a test case") {
    snitch::registry r;

    // Use prefixed macro for the code to be tested
    // (this will always use snitch)
    r.add({"a new test case", "[tag1][tag2]"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK(1 == 2);
    });

    // Use the shorthand CHECK macro to actually check something
    // (this will use either snitch or doctest)
    CHECK(r.test_cases.begin() != r.test_cases.end());
}
```


### Compile-time unit tests

Not implemented yet. The purpose of these tests will be to check how _snitch_ behaves at compile-time. In particular, to ensure that some specific usage will *not* compile. This cannot be achieved with the regular run-time tests, because it would prevent the whole test suite from compiling at all. Compile-time tests are better implemented at the build-system level, since the build system knows what compiler to use, what options, etc.


### Approval tests

These tests provide a more manageable way to check the output of _snitch_ for each reporter. Rather than use handwritten checks ("this line must contain this string, and then this other string"), which would be painful to write to achieve 100% coverage, the approach is to capture the output and save it to a file. If the output is satisfactory, we commit it as the new "expected" output, and future test runs just need to check that the output is compatible with the "expected" output. These tests contribute to the test coverage analysis.

Since some elements of the output are variable (e.g., test duration, file names, line numbers), the output goes through a "blanking" phase before being compared to the "expected" output. This uses regular expressions to ignore parts of the output, and replace them with a placeholder `*`.

Therefore, when approval tests are run, we end up with three sets of outputs in different directories:
 - `actual`: contains the actual raw output of _snitch_ for this run
 - `blanked`: contains the actual output after blanking
 - `expected`: contains the expected output, to be compared against

Only the `expected` directory is committed to the repository.

The approval tests are organized as separate files, one per reporter, in the `tests/approval_tests` directory. The test data is stored in `test/approval_tests/data`.


#### How to update approval test data

If an approval test fails for a "good" reason (i.e., it is expected to fail because we have intentionally modified the output, or added something that wasn't there before, or fixed a bug), then the "expected" data needs to be updated. To do so, follow the instructions below:
 - Run the approval tests; they fail.
 - Copy the content of the `blanked` directory into the `expected` directory.
 - Review the diff (e.g., `git diff` or through whatever git interface you use).
 - If the diff is as intended, commit the new `expected` directory.
 - Run the approval tests again: they should now pass.


### Install tests

The purpose of the install tests is to check that _snitch_ can be used as a library, once installed on a destination folder, rather than from the safe confines of the development repository. The tests themselves are not checking deep and complex features; the intention is only to check that the library is installed correctly, is fully standalone, and that other programs can be created by linking against it.

The install tests are located in the `tests/install_tests` directory.


## How to run the tests

### Running tests locally

#### Run-time unit tests

With CMake:
```cmake
# Using doctest to test snitch:
cmake --build build --target snitch_runtime_tests_run
# Using snitch to test itself:
cmake --build build --target snitch_runtime_tests_self_run
```

If you don't use CMake: after building the tests, locate the `snitch_runtime_tests` executable in your build directory and simply run it. The current working directory is irrelevant for these tests.


#### Compile-time unit tests

Not implemented yet.


#### Approval tests

With CMake:
```cmake
cmake --build build --target snitch_approval_tests_run
```

If you don't use CMake: after building the tests, open a terminal window in the `tests/approval_tests` directory, and run the `snitch_approval_tests` executable.


#### Install tests

These are a bit cumbersome to run locally, so the recommendation is to let the automated testing pipeline run them for you.


### Automated testing pipeline

In case this document is out-of-date or you have any issue running the tests on your platform, please refer to the [CI scripts](/.github/workflows). The commands in these scripts will always be up-to-date, and you can execute them locally to run the tests on your own machine. Otherwise, GitHub will run these CI scripts once you open your PR, and then for each following commit you push to the branch once the PR is open. This consumes some resources, so please be considerate and try to push commits in bulk, once you think the code should pass tests, rather than one-by-one (i.e., avoid the "commit+push" workflow).

The automated testing pipeline will run *all* tests, for *all* supported platforms. It will also include other checks, such as test coverage checks. It is necessary for this pipeline to pass before a PR can be merged.

The pipeline starts with the "install" tests, which are fairly cheap to run and will catch compilation errors in the _snitch_ headers, issues with basic testing features, or issues in CMake scripts. If these tests pass, the pipeline moves on to run the rest of the tests, which are more expensive.
