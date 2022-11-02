#include "testing.hpp"

using namespace std::literals;

TEST_CASE("small string", "[utility]") {
    constexpr std::size_t max_length = 5u;

    using TestType = snatch::small_string<max_length>;

    TestType v;

    SECTION("from empty") {
        CHECK(v.size() == 0u);
        CHECK(v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length);

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

    SECTION("from non-empty") {
        v.push_back('a');
        v.push_back('b');

        CHECK(v.size() == 2u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length - 2u);

        SECTION("push_back") {
            v.push_back('c');

            CHECK(v.size() == 3u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == max_length - 3u);
            CHECK(v.back() == 'c');
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

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
        }

        SECTION("resize max") {
            v.resize(max_length);

            CHECK(v.size() == max_length);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == 0u);

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
            // don't check the rest; undefined
        }

        SECTION("grow zero") {
            v.grow(0u);

            CHECK(v.size() == 2u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == max_length - 2u);

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
        }

        SECTION("grow some") {
            v.grow(2u);

            CHECK(v.size() == 4u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == 1u);

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
            // don't check the rest; undefined
        }

        SECTION("grow max") {
            v.grow(max_length - 2u);

            CHECK(v.size() == max_length);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == 0u);

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
            // don't check the rest; undefined
        }
    }

    SECTION("from full") {
        v.push_back('a');
        v.push_back('b');
        v.push_back('c');
        v.push_back('d');
        v.push_back('e');

        CHECK(v.size() == max_length);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == 0u);

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

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
        }

        SECTION("resize max") {
            v.resize(max_length);

            CHECK(v.size() == max_length);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == 0u);

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
            CHECK(v.begin()[2] == 'c');
            CHECK(v.begin()[3] == 'd');
            CHECK(v.begin()[4] == 'e');
        }

        SECTION("grow zero") {
            v.grow(0u);

            CHECK(v.size() == max_length);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_length);
            CHECK(v.available() == 0u);

            CHECK(v.begin()[0] == 'a');
            CHECK(v.begin()[1] == 'b');
            CHECK(v.begin()[2] == 'c');
            CHECK(v.begin()[3] == 'd');
            CHECK(v.begin()[4] == 'e');
        }
    }

    SECTION("from string view") {
        v = "abc"sv;

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_length);
        CHECK(v.available() == max_length - 3u);

        CHECK(v.begin()[0] == 'a');
        CHECK(v.begin()[1] == 'b');
        CHECK(v.begin()[2] == 'c');
    }
};

// This requires fixing https://github.com/cschreib/snatch/issues/17
// TEST_CASE("constexpr small string", "[utility]") {
//     constexpr std::size_t max_length = 5u;

//     using TestType = snatch::small_string<max_length>;

//     SECTION("from string view") {
//         constexpr TestType v = "abc"sv;

//         CHECK(v.size() == 3u);
//         CHECK(!v.empty());
//         CHECK(v.capacity() == max_length);
//         CHECK(v.available() == max_length - 3u);

//         CHECK(v.data()[0] == 'a');
//         CHECK(v.data()[1] == 'b');
//         CHECK(v.data()[2] == 'c');
//     }

//     SECTION("from immediate lambda") {
//         constexpr TestType v = []() {
//             TestType v;
//             v.push_back('a');
//             v.push_back('b');
//             v.push_back('c');
//             v.push_back('d');
//             v.pop_back();
//             v.push_back('e');
//             v.grow(1u);
//             v.resize(3u);
//             return v;
//         }();

//         CHECK(v.size() == 3u);
//         CHECK(!v.empty());
//         CHECK(v.capacity() == max_length);
//         CHECK(v.available() == max_length - 3u);

//         CHECK(v.data()[0] == 'a');
//         CHECK(v.data()[1] == 'b');
//         CHECK(v.data()[2] == 'c');
//     }
// };
