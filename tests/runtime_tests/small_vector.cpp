#include "testing.hpp"

namespace {
constexpr std::size_t max_test_elements = 5u;

struct test_struct {
    int  i = 0;
    bool b = true;
};

using vector_type     = snitch::small_vector<test_struct, max_test_elements>;
using span_type       = snitch::small_vector_span<test_struct>;
using const_span_type = snitch::small_vector_span<const test_struct>;
} // namespace

TEMPLATE_TEST_CASE("small vector", "[utility]", vector_type, span_type, const_span_type) {
    constexpr bool is_const = std::is_same_v<TestType, const_span_type>;

    auto create_type = [&](auto create_vector) -> TestType {
        // Cannot use is_const here for MSVC because of their bug
        // https://developercommunity.visualstudio.com/t/Capture-of-constexpr-variable-not-workin/10190629
        if constexpr (std::is_same_v<TestType, vector_type>) {
            return create_vector();
        } else {
            static vector_type v;
            v = create_vector();
            if constexpr (std::is_same_v<TestType, const_span_type>) {
                return std::as_const(v).span();
            } else {
                return v.span();
            }
        }
    };

    SECTION("from empty") {
        auto create_vector = []() { return vector_type{}; };

        TestType v = create_type(create_vector);

        CHECK(v.size() == 0u);
        CHECK(v.empty());
        CHECK(v.capacity() == max_test_elements);
        CHECK(v.available() == max_test_elements);
        CHECK(v.begin() == v.end());
        CHECK(v.cbegin() == v.cend());

        if constexpr (!is_const) {
            SECTION("push_back") {
                v.push_back(test_struct{1, false});

                CHECK(v.size() == 1u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 1u);
                CHECK(v.back().i == 1);
                CHECK(v.back().b == false);
            }

            SECTION("clear") {
                v.clear();

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("resize zero") {
                v.resize(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("resize some") {
                v.resize(3u);

                CHECK(v.size() == 3u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 3u);

                // don't check values; undefined
            }

            SECTION("resize max") {
                v.resize(max_test_elements);

                CHECK(v.size() == max_test_elements);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == 0u);

                // don't check values; undefined
            }

            SECTION("grow zero") {
                v.grow(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("grow some") {
                v.grow(3u);

                CHECK(v.size() == 3u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 3u);

                // don't check values; undefined
            }

            SECTION("grow max") {
                v.grow(max_test_elements);

                CHECK(v.size() == max_test_elements);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == 0u);

                // don't check values; undefined
            }
        }
    }

    SECTION("from non-empty") {
        auto create_vector = []() {
            vector_type v;
            v.push_back(test_struct{4, true});
            v.push_back(test_struct{6, false});
            return v;
        };

        TestType v = create_type(create_vector);

        CHECK(v.size() == 2u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_elements);
        CHECK(v.available() == max_test_elements - 2u);
        CHECK(v.end() == v.begin() + 2u);
        CHECK(v.cend() == v.cbegin() + 2u);

        if constexpr (!is_const) {
            SECTION("push_back") {
                v.push_back(test_struct{1, false});

                CHECK(v.size() == 3u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 3u);
                CHECK(v.back().i == 1);
                CHECK(v.back().b == false);
            }

            SECTION("pop_back") {
                v.pop_back();

                CHECK(v.size() == 1u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 1u);
                CHECK(v.back().i == 4);
                CHECK(v.back().b == true);
            }

            SECTION("clear") {
                v.clear();

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("resize zero") {
                v.resize(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("resize some") {
                v.resize(2u);

                CHECK(v.size() == 2u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 2u);

                CHECK(v[0].i == 4);
                CHECK(v[1].i == 6);
                CHECK(v[0].b == true);
                CHECK(v[1].b == false);
            }

            SECTION("resize max") {
                v.resize(max_test_elements);

                CHECK(v.size() == max_test_elements);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
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
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 2u);

                CHECK(v[0].i == 4);
                CHECK(v[1].i == 6);
                CHECK(v[0].b == true);
                CHECK(v[1].b == false);
            }

            SECTION("grow some") {
                v.grow(2u);

                CHECK(v.size() == 4u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == 1u);

                CHECK(v[0].i == 4);
                CHECK(v[1].i == 6);
                CHECK(v[0].b == true);
                CHECK(v[1].b == false);
                // don't check the rest; undefined
            }

            SECTION("grow max") {
                v.grow(max_test_elements - 2u);

                CHECK(v.size() == max_test_elements);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == 0u);

                CHECK(v[0].i == 4);
                CHECK(v[1].i == 6);
                CHECK(v[0].b == true);
                CHECK(v[1].b == false);
                // don't check the rest; undefined
            }
        }
    }

    SECTION("from full") {
        auto create_vector = []() {
            vector_type v;
            v.push_back(test_struct{4, true});
            v.push_back(test_struct{6, false});
            v.push_back(test_struct{8, true});
            v.push_back(test_struct{10, true});
            v.push_back(test_struct{12, false});
            return v;
        };

        TestType v = create_type(create_vector);

        CHECK(v.size() == max_test_elements);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_elements);
        CHECK(v.available() == 0u);
        CHECK(v.end() == v.begin() + max_test_elements);
        CHECK(v.cend() == v.cbegin() + max_test_elements);

        if constexpr (!is_const) {
            SECTION("pop_back") {
                v.pop_back();

                CHECK(v.size() == 4u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 4u);
                CHECK(v.back().i == 10);
                CHECK(v.back().b == true);
            }

            SECTION("clear") {
                v.clear();

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("resize zero") {
                v.resize(0u);

                CHECK(v.size() == 0u);
                CHECK(v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements);
            }

            SECTION("resize some") {
                v.resize(2u);

                CHECK(v.size() == 2u);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
                CHECK(v.available() == max_test_elements - 2u);

                CHECK(v[0].i == 4);
                CHECK(v[1].i == 6);
                CHECK(v[0].b == true);
                CHECK(v[1].b == false);
            }

            SECTION("resize max") {
                v.resize(max_test_elements);

                CHECK(v.size() == max_test_elements);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
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

                CHECK(v.size() == max_test_elements);
                CHECK(!v.empty());
                CHECK(v.capacity() == max_test_elements);
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
    }

    SECTION("from initializer list") {
        auto create_vector = []() {
            return vector_type{test_struct{1, true}, test_struct{2, false}, test_struct{5, false}};
        };

        TestType v = create_type(create_vector);

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_elements);
        CHECK(v.available() == max_test_elements - 3u);
        CHECK(v.end() == v.begin() + 3u);
        CHECK(v.cend() == v.cbegin() + 3u);

        CHECK(v[0].i == 1);
        CHECK(v[1].i == 2);
        CHECK(v[2].i == 5);
        CHECK(v[0].b == true);
        CHECK(v[1].b == false);
        CHECK(v[2].b == false);
    }
}

TEST_CASE("constexpr small vector test_struct", "[utility]") {
    using TestType = vector_type;

    SECTION("from initializer list") {
        constexpr TestType v = {test_struct{1, true}, test_struct{2, false}, test_struct{5, false}};

        CHECK(v.size() == 3u);
        CHECK(!v.empty());
        CHECK(v.capacity() == max_test_elements);
        CHECK(v.available() == max_test_elements - 3u);
        CHECK(v.end() == v.begin() + 3u);
        CHECK(v.cend() == v.cbegin() + 3u);

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
        CHECK(v.capacity() == max_test_elements);
        CHECK(v.available() == max_test_elements - 3u);
        CHECK(v.end() == v.begin() + 3u);
        CHECK(v.cend() == v.cbegin() + 3u);

        CHECK(v[0].i == 1);
        CHECK(v[1].i == 2);
        CHECK(v[2].i == 5);
        CHECK(v[0].b == true);
        CHECK(v[1].b == false);
        CHECK(v[2].b == false);
    }
}

// This requires fixing https://github.com/cschreib/snitch/issues/17
// TEST_CASE("constexpr small vector int", "[utility]") {
//     using TestType = snitch::small_vector<int, max_test_elements>;

//     SECTION("from initializer list") {
//         constexpr TestType v = {1, 2, 5};

//         CHECK(v.size() == 3u);
//         CHECK(!v.empty());
//         CHECK(v.capacity() == max_test_elements);
//         CHECK(v.available() == max_test_elements - 3u);
//         CHECK(v.end() == v.begin() + 3u);
//         CHECK(v.cend() == v.cbegin() + 3u);

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
//         CHECK(v.capacity() == max_test_elements);
//         CHECK(v.available() == max_test_elements - 3u);
//         CHECK(v.end() == v.begin() + 3u);
//         CHECK(v.cend() == v.cbegin() + 3u);

//         CHECK(v[0] == 1);
//         CHECK(v[1] == 2);
//         CHECK(v[2] == 5);
//     }
// };
