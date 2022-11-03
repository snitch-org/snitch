#include "testing.hpp"

TEST_CASE("small vector", "[utility]") {
    struct test_struct {
        int  i = 0;
        bool b = true;
    };

    constexpr std::size_t max_test_structs = 5u;

    using TestType = snatch::small_vector<test_struct, max_test_structs>;

    TestType v;

    SECTION("from empty") {
        CHECK(v.size() == 0u);
        CHECK(v.empty());
        CHECK(v.capacity() == max_test_structs);
        CHECK(v.available() == max_test_structs);

        SECTION("push_back") {
            v.push_back(test_struct{1, false});

            CHECK(v.size() == 1u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 1u);
            CHECK(v.back().i == 1);
            CHECK(v.back().b == false);
        }

        SECTION("clear") {
            v.clear();

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("resize zero") {
            v.resize(0u);

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("resize some") {
            v.resize(3u);

            CHECK(v.size() == 3u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 3u);

            // don't check values; undefined
        }

        SECTION("resize max") {
            v.resize(max_test_structs);

            CHECK(v.size() == max_test_structs);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 0u);

            // don't check values; undefined
        }

        SECTION("grow zero") {
            v.grow(0u);

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("grow some") {
            v.grow(3u);

            CHECK(v.size() == 3u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 3u);

            // don't check values; undefined
        }

        SECTION("grow max") {
            v.grow(max_test_structs);

            CHECK(v.size() == max_test_structs);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 0u);

            // don't check values; undefined
        }
    }

    SECTION("from non-empty") {
        v.push_back(test_struct{4, true});
        v.push_back(test_struct{6, false});

        CHECK(v.size() == 2u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_structs);
        CHECK(v.available() == max_test_structs - 2u);

        SECTION("push_back") {
            v.push_back(test_struct{1, false});

            CHECK(v.size() == 3u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 3u);
            CHECK(v.back().i == 1);
            CHECK(v.back().b == false);
        }

        SECTION("clear") {
            v.clear();

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("resize zero") {
            v.resize(0u);

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("resize some") {
            v.resize(2u);

            CHECK(v.size() == 2u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 2u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
        }

        SECTION("resize max") {
            v.resize(max_test_structs);

            CHECK(v.size() == max_test_structs);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 0u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
            // don't check the rest; undefined
        }

        SECTION("grow zero") {
            v.grow(0u);

            CHECK(v.size() == 2u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 2u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
        }

        SECTION("grow some") {
            v.grow(2u);

            CHECK(v.size() == 4u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 1u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
            // don't check the rest; undefined
        }

        SECTION("grow max") {
            v.grow(max_test_structs - 2u);

            CHECK(v.size() == max_test_structs);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 0u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
            // don't check the rest; undefined
        }
    }

    SECTION("from full") {
        v.push_back(test_struct{4, true});
        v.push_back(test_struct{6, false});
        v.push_back(test_struct{8, true});
        v.push_back(test_struct{10, true});
        v.push_back(test_struct{12, false});

        CHECK(v.size() == max_test_structs);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_structs);
        CHECK(v.available() == 0u);

        SECTION("clear") {
            v.clear();

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("resize zero") {
            v.resize(0u);

            CHECK(v.size() == 0u);
            CHECK(v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs);
        }

        SECTION("resize some") {
            v.resize(2u);

            CHECK(v.size() == 2u);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == max_test_structs - 2u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
        }

        SECTION("resize max") {
            v.resize(max_test_structs);

            CHECK(v.size() == max_test_structs);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 0u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[2].i == 8);
            CHECK(v[3].i == 10);
            CHECK(v[4].i == 12);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
            CHECK(v[2].b == true);
            CHECK(v[3].b == true);
            CHECK(v[4].b == false);
        }

        SECTION("grow zero") {
            v.grow(0u);

            CHECK(v.size() == max_test_structs);
            CHECK(!v.empty());
            CHECK(v.capacity() == max_test_structs);
            CHECK(v.available() == 0u);

            CHECK(v[0].i == 4);
            CHECK(v[1].i == 6);
            CHECK(v[2].i == 8);
            CHECK(v[3].i == 10);
            CHECK(v[4].i == 12);
            CHECK(v[0].b == true);
            CHECK(v[1].b == false);
            CHECK(v[2].b == true);
            CHECK(v[3].b == true);
            CHECK(v[4].b == false);
        }
    }

    SECTION("from initializer list") {
        v = {test_struct{1, true}, test_struct{2, false}, test_struct{5, false}};

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_structs);
        CHECK(v.available() == max_test_structs - 3u);

        CHECK(v[0].i == 1);
        CHECK(v[1].i == 2);
        CHECK(v[2].i == 5);
        CHECK(v[0].b == true);
        CHECK(v[1].b == false);
        CHECK(v[2].b == false);
    }
};

TEST_CASE("constexpr small vector test_struct", "[utility]") {
    struct test_struct {
        int  i = 0;
        bool b = true;
    };

    constexpr std::size_t max_test_structs = 5u;

    using TestType = snatch::small_vector<test_struct, max_test_structs>;

    SECTION("from initializer list") {
        constexpr TestType v = {test_struct{1, true}, test_struct{2, false}, test_struct{5, false}};

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_structs);
        CHECK(v.available() == max_test_structs - 3u);

        CHECK(v[0].i == 1);
        CHECK(v[1].i == 2);
        CHECK(v[2].i == 5);
        CHECK(v[0].b == true);
        CHECK(v[1].b == false);
        CHECK(v[2].b == false);
    }

    SECTION("from immediate lambda") {
        constexpr TestType v = []() {
            TestType v;
            v.push_back(test_struct{1, true});
            v.push_back(test_struct{2, false});
            v.push_back(test_struct{5, false});
            v.push_back(test_struct{6, false});
            v.pop_back();
            v.push_back(test_struct{7, false});
            v.grow(1u);
            v.resize(3u);
            return v;
        }();

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_structs);
        CHECK(v.available() == max_test_structs - 3u);

        CHECK(v[0].i == 1);
        CHECK(v[1].i == 2);
        CHECK(v[2].i == 5);
        CHECK(v[0].b == true);
        CHECK(v[1].b == false);
        CHECK(v[2].b == false);
    }
};

// This requires fixing https://github.com/cschreib/snatch/issues/17
// TEST_CASE("constexpr small vector int", "[utility]") {
//     constexpr std::size_t max_test_structs = 5u;

//     using TestType = snatch::small_vector<int, max_test_structs>;

//     SECTION("from initializer list") {
//         constexpr TestType v = {1, 2, 5};

//         CHECK(v.size() == 3u);
//         CHECK(!v.empty());
//         CHECK(v.capacity() == max_test_structs);
//         CHECK(v.available() == max_test_structs - 3u);

//         CHECK(v[0] == 1);
//         CHECK(v[1] == 2);
//         CHECK(v[2] == 3);
//     }

//     SECTION("from immediate lambda") {
//         constexpr TestType v = []() {
//             TestType v;
//             v.push_back(1);
//             v.push_back(2);
//             v.push_back(5);
//             v.push_back(6);
//             v.pop_back();
//             v.push_back(7);
//             v.grow(1u);
//             v.resize(3u);
//             return v;
//         }();

//         CHECK(v.size() == 3u);
//         CHECK(!v.empty());
//         CHECK(v.capacity() == max_test_structs);
//         CHECK(v.available() == max_test_structs - 3u);

//         CHECK(v[0] == 1);
//         CHECK(v[1] == 2);
//         CHECK(v[2] == 5);
//     }
// };
