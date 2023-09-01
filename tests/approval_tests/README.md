# Approval tests

## Introduction

These tests provide a more manageable way to check the output of *snitch* for each reporter. Rather than use hand-written checks ("this line must contain this string, and then this other string"), which would be painful to write to achieve 100% coverage, the approach is to capture the output and save it to a file. If the output is satisfactory, we commit it as the new "expected" output, and future test runs just need to check that the output is compatible with the "expected" output.

Since some elements of the output are variable (e.g., test duration, file names, line numbers), the output goes through a "blanking" phase before being compared to the "expected" output. This uses regular expressions to ignore parts of the output, and replace them with a placeholder `*`.

Therefore, when approval tests are run, we end up with three sets of outputs in different directories:
 - `actual`: contains the actual raw output of *snitch* for this run
 - `blanked`: contains the actual output after blanking
 - `expected`: contains the expected output, to be compared against

Only the `expected` directory is committed to the repository.


## How to update

If an approval test fails for a "good" reason (i.e., it is expected to fail because we have intentionally modified the output, or added something that wasn't there before, or fixed a bug), then the "expected" data needs to be updated. To do so, follow the instructions below:
 - Run the approval tests; they fail.
 - Copy the content of the `blanked` directory into the `expected` directory.
 - Review the diff (e.g., `git diff` or through whatever git interface you use).
 - If the diff is as intended, commit the new `expected` directory.
 - Run the approval tests again: they should now pass.
