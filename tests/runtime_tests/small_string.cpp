#include "testing.hpp"

using namespace std::literals;

namespace {
constexpr std::size_t max_length = 5u;

using string_type = snitch::small_string<max_length>;
using span_type   = snitch::small_string_span;
using view_type   = snitch::small_string_view;
} // namespace

TEMPLATE_TEST_CASE("small string", "[utility]", string_type, span_type, view_type) {
    constexpr bool is_string = std::is_same_v<TestType, string_type>;
    constexpr bool is_const  = std::is_same_v<TestType, view_type>;

    auto create_type = [&](auto create_string) -> TestType {
        // Cannot use is_string and is_const here for MSVC because of their bug
        // https://developercommunity.visualstudio.com/t/Capture-of-constexpr-variable-not-workin/10190629
        if constexpr (std::is_same_v<TestType, string_type>) {
            return create_string();
        } else {
            static string_type v;
            v = create_string();
            if constexpr (std::is_same_v<TestType, view_type>) {
                return std::as_const(v).span();
            } else {
                return v.span();
            }
        }
    };

    SECTION("from empty") {
        auto create_string = []() { return string_type{}; };

        TestType v = create_type(create_string);

        CHECK(v.size() == 0u);
        CHECK(v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length);
        CHECK(v.begin() == v.end());
        CHECK(v.cbegin() == v.cend());

        if constexpr (!is_const) {
            SECTION("push_back") {
                v.push_back('a');

                CHECK(v.size() == 1u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 1u);
                CHECK(v.back() == 'a');
            }

            SECTION("clear") {
                v.clear();

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("resize zero") {
                v.resize(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("resize some") {
                v.resize(3u);

                CHECK(v.size() == 3u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 3u);
            }

            SECTION("resize max") {
                v.resize(max_length);

                CHECK(v.size() == max_length);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 0u);
            }

            SECTION("grow zero") {
                v.grow(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("grow some") {
                v.grow(3u);

                CHECK(v.size() == 3u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 3u);
            }

            SECTION("grow max") {
                v.grow(max_length);

                CHECK(v.size() == max_length);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 0u);
            }
        }
    }

    SECTION("from non-empty") {
        auto create_string = []() {
            string_type v;
            v.push_back('a');
            v.push_back('b');
            return v;
        };

        TestType v = create_type(create_string);

        CHECK(v.size() == 2u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length - 2u);
        CHECK(v.end() == v.begin() + 2u);
        CHECK(v.cend() == v.cbegin() + 2u);

        if constexpr (!is_const) {
            SECTION("push_back") {
                v.push_back('c');

                CHECK(v.size() == 3u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 3u);
                CHECK(v.back() == 'c');
            }

            SECTION("pop_back") {
                v.pop_back();

                CHECK(v.size() == 1u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 1u);
                CHECK(v.back() == 'a');
            }

            SECTION("clear") {
                v.clear();

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("resize zero") {
                v.resize(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("resize some") {
                v.resize(2u);

                CHECK(v.size() == 2u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 2u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
            }

            SECTION("resize max") {
                v.resize(max_length);

                CHECK(v.size() == max_length);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 0u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
                // don't check the rest; undefined
            }

            SECTION("grow zero") {
                v.grow(0u);

                CHECK(v.size() == 2u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 2u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
            }

            SECTION("grow some") {
                v.grow(2u);

                CHECK(v.size() == 4u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 1u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
                // don't check the rest; undefined
            }

            SECTION("grow max") {
                v.grow(max_length - 2u);

                CHECK(v.size() == max_length);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 0u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
                // don't check the rest; undefined
            }
        }
    }

    SECTION("from full") {
        auto create_string = []() {
            string_type v;
            v.push_back('a');
            v.push_back('b');
            v.push_back('c');
            v.push_back('d');
            v.push_back('e');
            return v;
        };

        TestType v = create_type(create_string);

        CHECK(v.size() == max_length);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == 0u);
        CHECK(v.end() == v.begin() + max_length);
        CHECK(v.cend() == v.cbegin() + max_length);

        if constexpr (!is_const) {
            SECTION("pop_back") {
                v.pop_back();

                CHECK(v.size() == 4u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 4u);
                CHECK(v.back() == 'd');
            }

            SECTION("clear") {
                v.clear();

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("resize zero") {
                v.resize(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length);
            }

            SECTION("resize some") {
                v.resize(2u);

                CHECK(v.size() == 2u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == max_length - 2u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
            }

            SECTION("resize max") {
                v.resize(max_length);

                CHECK(v.size() == max_length);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 0u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
                CHECK(v[2] == 'c');
                CHECK(v[3] == 'd');
                CHECK(v[4] == 'e');
            }

            SECTION("grow zero") {
                v.grow(0u);

                CHECK(v.size() == max_length);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_length);
                CHECK(v.available() == 0u);

                CHECK(v[0] == 'a');
                CHECK(v[1] == 'b');
                CHECK(v[2] == 'c');
                CHECK(v[3] == 'd');
                CHECK(v[4] == 'e');
            }
        }
    }

    SECTION("from string view") {
        auto create_string = []() { return string_type("abc"sv); };

        TestType v = create_type(create_string);

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length - 3u);
        CHECK(v.end() == v.begin() + 3u);
        CHECK(v.cend() == v.cbegin() + 3u);

        CHECK(v[0] == 'a');
        CHECK(v[1] == 'b');
        CHECK(v[2] == 'c');
    }

    SECTION("to string view") {
        if constexpr (is_string) {
            auto create_string = []() { return string_type("abc"sv); };

            TestType         v  = create_type(create_string);
            std::string_view sv = v;

            CHECK(sv.size() == 3u);
            CHECK(!sv.empty());
            CHECK(sv.end() == sv.begin() + 3u);

            CHECK(sv[0] == 'a');
            CHECK(sv[1] == 'b');
            CHECK(sv[2] == 'c');
        }
    }
}

TEST_CASE("constexpr small string", "[utility]") {
    SECTION("from string view") {
        constexpr string_type v = "abc"sv;

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length - 3u);
        CHECK(v.end() == v.begin() + 3u);
        CHECK(v.cend() == v.cbegin() + 3u);

        CHECK(v[0] == 'a');
        CHECK(v[1] == 'b');
        CHECK(v[2] == 'c');
    }

    SECTION("from immediate lambda") {
        constexpr string_type v = []() {
            string_type v;
            v.push_back('a');
            v.push_back('b');
            v.push_back('c');
            v.push_back('d');
            v.pop_back();
            v.push_back('e');
            v.grow(1u);
            v.resize(3u);
            return v;
        }();

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length - 3u);
        CHECK(v.end() == v.begin() + 3u);
        CHECK(v.cend() == v.cbegin() + 3u);

        CHECK(v[0] == 'a');
        CHECK(v[1] == 'b');
        CHECK(v[2] == 'c');
    }
}
