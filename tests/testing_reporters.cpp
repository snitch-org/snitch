// clang-format off
#include "testing.hpp"
#include "testing_reporters.hpp"
// clang-format on

namespace {
#if SNITCH_WITH_EXCEPTIONS
void throw_something(bool do_throw) {
    if (do_throw) {
        throw std::runtime_error("I threw");
    }
}
int throw_unexpectedly() {
    throw std::runtime_error("unexpected error");
}
#endif

constexpr int some_very_long_name_that_forces_lines_to_wrap = 1;
} // namespace

SNITCH_WARNING_PUSH
SNITCH_WARNING_DISABLE_UNREACHABLE

void register_tests_for_reporters(snitch::registry& r) {
    // To avoid unnecessary changes to the approval test data, please add new tests only at the
    // end of this list.

    r.add({"test pass", "[tag2][tag1]"}, SNITCH_CURRENT_LOCATION, []() {});
    r.add({"test fail", "[tag2][tag1]"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_CHECK(1 == 2); });

    r.add({"test mayfail good pass", "[tag2][tag1][!mayfail]"}, SNITCH_CURRENT_LOCATION, []() {});
    r.add({"test mayfail bad pass", "[tag2][tag1][!mayfail]"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK(1 == 2);
    });
    r.add(
        {"test shouldfail good fail", "[tag2][tag1][!shouldfail]"}, SNITCH_CURRENT_LOCATION,
        []() {});
    r.add({"test shouldfail bad pass", "[tag2][tag1][!shouldfail]"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK(1 == 2);
    });

    r.add({"test no tags pass"}, SNITCH_CURRENT_LOCATION, []() {});
    r.add({"test no tags fail"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_CHECK(1 == 2); });

    r.add_with_types<int, float>(
        {"typed test no tags pass"}, SNITCH_CURRENT_LOCATION, []<typename T>() {});
    r.add_with_types<int, float>(
        {"typed test no tags fail"}, SNITCH_CURRENT_LOCATION,
        []<typename T>() { SNITCH_CHECK(1 == 2); });

    r.add_with_types<int, float>(
        {"typed test with tags pass", "[tag1]"}, SNITCH_CURRENT_LOCATION, []<typename T>() {});
    r.add_with_types<int, float>(
        {"typed test with tags fail", "[tag1]"}, SNITCH_CURRENT_LOCATION,
        []<typename T>() { SNITCH_CHECK(1 == 2); });

    r.add_fixture(
        {"MyClass", "test fixture pass", "[tag with space]"}, SNITCH_CURRENT_LOCATION, []() {});
    r.add_fixture(
        {"MyClass", "test fixture fail", "[tag with space]"}, SNITCH_CURRENT_LOCATION,
        []() { SNITCH_CHECK(1 == 2); });

    r.add(
        {"test SUCCEED pass"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_SUCCEED("something good"); });
    r.add(
        {"test FAIL fail"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_FAIL_CHECK("something bad"); });
    r.add({"test expression pass"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_CHECK(1 == 1); });
    r.add({"test expression fail"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_CHECK(1 == 2); });
    r.add({"test long expression pass"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK(
            some_very_long_name_that_forces_lines_to_wrap ==
            some_very_long_name_that_forces_lines_to_wrap);
    });
    r.add({"test long expression fail"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK(
            some_very_long_name_that_forces_lines_to_wrap !=
            some_very_long_name_that_forces_lines_to_wrap);
    });
    r.add({"test too long expression pass"}, SNITCH_CURRENT_LOCATION, []() {
        std::string super_long_string(2 * snitch::max_message_length, 'a');
        SNITCH_CHECK(super_long_string == super_long_string);
    });
    r.add({"test too long expression fail"}, SNITCH_CURRENT_LOCATION, []() {
        std::string super_long_string(2 * snitch::max_message_length, 'a');
        SNITCH_CHECK(super_long_string != super_long_string);
    });
    r.add({"test too long message pass"}, SNITCH_CURRENT_LOCATION, []() {
        std::string super_long_string(2 * snitch::max_message_length, 'a');
        SNITCH_FAIL(super_long_string);
    });
    r.add({"test too long message fail"}, SNITCH_CURRENT_LOCATION, []() {
        std::string super_long_string(2 * snitch::max_message_length, 'a');
        SNITCH_FAIL(super_long_string);
    });

#if SNITCH_WITH_EXCEPTIONS
    r.add({"test NOTHROW pass"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK_NOTHROW(throw_something(false));
    });
    r.add({"test NOTHROW fail"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK_NOTHROW(throw_something(true));
    });
    r.add({"test THROW pass"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK_THROWS_MATCHES(
            throw_something(true), std::runtime_error,
            snitch::matchers::with_what_contains{"I threw"});
    });
    r.add({"test THROW fail"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK_THROWS_MATCHES(
            throw_something(false), std::runtime_error,
            snitch::matchers::with_what_contains{"I threw"});
        SNITCH_CHECK_THROWS_MATCHES(
            throw_something(true), std::system_error,
            snitch::matchers::with_what_contains{"I threw"});
        SNITCH_CHECK_THROWS_MATCHES(
            throw_something(true), std::runtime_error,
            snitch::matchers::with_what_contains{"I throws"});
    });

    r.add({"test unexpected throw fail"}, SNITCH_CURRENT_LOCATION, []() {
        // make sure the throw is on a new line
        throw_unexpectedly();
    });
    r.add({"test unexpected throw in section fail"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_SECTION("section 1") {
            SNITCH_SECTION("section 2") {
                throw_unexpectedly();
            }
        }
    });
    r.add({"test unexpected throw in check fail"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_CHECK(throw_unexpectedly() == 0);
    });
    r.add({"test unexpected throw in check & section fail"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_SECTION("section 1") {
            SNITCH_CHECK(throw_unexpectedly() == 0);
        }
    });
    r.add(
        {"test unexpected throw in check & section mayfail", "[!mayfail]"}, SNITCH_CURRENT_LOCATION,
        []() {
            SNITCH_SECTION("section 1") {
                SNITCH_CHECK(throw_unexpectedly() == 0);
            }
        });
#endif

    r.add({"test SKIP"}, SNITCH_CURRENT_LOCATION, []() { SNITCH_SKIP("not interesting"); });

    r.add({"test INFO"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_INFO("info");
        SNITCH_FAIL_CHECK("failure");
    });

    r.add({"test multiple INFO"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_FAIL_CHECK("failure 1");
        SNITCH_INFO("info 1");
        SNITCH_FAIL_CHECK("failure 2");
        {
            SNITCH_INFO("info 2");
            SNITCH_FAIL_CHECK("failure 3");
        }
        SNITCH_FAIL_CHECK("failure 4");
    });

    r.add({"test SECTION"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_SECTION("section") {
            SNITCH_FAIL_CHECK("failure");
        }
    });

    r.add({"test SECTION mayfail", "[!mayfail]"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_SECTION("section") {
            SNITCH_FAIL_CHECK("failure");
        }
    });

    r.add({"test multiple SECTION"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_SECTION("section 1") {
            SNITCH_FAIL_CHECK("failure 1");
        }
        SNITCH_SECTION("section 2") {
            SNITCH_FAIL_CHECK("failure 2");
            SNITCH_SECTION("section 2.1") {
                SNITCH_FAIL_CHECK("failure 3");
            }
            SNITCH_SECTION("section 2.2") {
                SNITCH_FAIL_CHECK("failure 4");
                SNITCH_SECTION("section 2.2.1") {
                    SNITCH_FAIL_CHECK("failure 5");
                }
                SNITCH_FAIL_CHECK("failure 6");
            }
        }
        SNITCH_FAIL_CHECK("failure 7");
    });

    r.add({"test SECTION & INFO"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_INFO("info 1");
        SNITCH_SECTION("section 1") {
            SNITCH_INFO("info 2");
            SNITCH_FAIL_CHECK("failure 1");
        }
        SNITCH_SECTION("section 2") {
            SNITCH_INFO("info 3");
            SNITCH_FAIL_CHECK("failure 2");
        }
        SNITCH_FAIL_CHECK("failure 3");
    });

    r.add({"test SECTION & CAPTURE"}, SNITCH_CURRENT_LOCATION, []() {
        int i = 1;
        SNITCH_CAPTURE(i);
        SNITCH_SECTION("section 1") {
            int j = 2;
            SNITCH_CAPTURE(j);
            SNITCH_FAIL_CHECK("failure 1");
        }
        SNITCH_SECTION("section 2") {
            int j = 3;
            SNITCH_CAPTURE(j);
            SNITCH_FAIL_CHECK("failure 2");
        }
        SNITCH_FAIL_CHECK("failure 3");
    });

    r.add({"test SKIP in SECTION"}, SNITCH_CURRENT_LOCATION, []() {
        SNITCH_SECTION("section 1") {
            SNITCH_SECTION("section 2") {
                SNITCH_SKIP("stopping here");
                SNITCH_SECTION("section 3") {
                    SNITCH_FAIL_CHECK("failure 1");
                }
            }
        }
        SNITCH_SECTION("section 2") {
            SNITCH_FAIL_CHECK("failure 2");
        }
    });
}

SNITCH_WARNING_POP

namespace {
template<typename F>
void regex_replace(std::string& line, const std::regex& r, F&& func) {
    std::smatch matches;
    if (!std::regex_search(line, matches, r)) {
        return;
    }

    std::ptrdiff_t offset = 0;
    for (std::size_t m = matches.size() == 1 ? 0 : 1; m < matches.size(); ++m) {
        const auto        spos     = matches[m].first - line.begin() + offset;
        const auto        epos     = matches[m].second - line.begin() + offset;
        const std::size_t old_size = line.size();

        func(line, spos, epos);

        offset += static_cast<std::ptrdiff_t>(line.size()) - static_cast<std::ptrdiff_t>(old_size);
    }
}
} // namespace

void regex_blank(std::string& line, const std::regex& ignore) {
    regex_replace(line, ignore, [](std::string& l, std::size_t s, std::size_t e) {
        if (s == e) {
            l.insert(l.begin() + s, '*');
        } else {
            l[s] = '*';
            l.erase(s + 1, e - s - 1);
        }
    });
}

void regex_blank(std::string& line, const std::vector<std::regex>& ignores) {
    for (const auto& r : ignores) {
        regex_blank(line, r);
    }
}
