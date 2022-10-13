#ifndef SNATCH_HPP
#define SNATCH_HPP

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <tuple>

#if !defined(SNATCH_MAX_TEST_CASES)
#    define SNATCH_MAX_TEST_CASES 5'000
#endif
#if !defined(SNATCH_MAX_EXPR_LENGTH)
#    define SNATCH_MAX_EXPR_LENGTH 1'024
#endif
#if !defined(SNATCH_MAX_MATCHER_MSG_LENGTH)
#    define SNATCH_MAX_MATCHER_MSG_LENGTH 1'024
#endif
#if !defined(SNATCH_MAX_TEST_NAME_LENGTH)
#    define SNATCH_MAX_TEST_NAME_LENGTH 1'024
#endif

// Testing framework configuration.
// --------------------------------

namespace testing {
// Maximum number of test cases in the whole program.
// A "test case" is created for each uses of the `*_TEST_CASE` macros,
// and for each type for the `TEMPLATE_LIST_TEST_CASE` macro.
constexpr std::size_t max_test_cases = SNATCH_MAX_TEST_CASES;
// Maximum length of a `CHECK(...)` or `REQUIRE(...)` expression,
// beyond which automatic variable printing is disabled.
constexpr std::size_t max_expr_length = SNATCH_MAX_EXPR_LENGTH;
// Maximum length of error messages that need dynamic formatting;
// most messages do not.
constexpr std::size_t max_matcher_msg_length = SNATCH_MAX_MATCHER_MSG_LENGTH;
// Maximum length of a full test case name.
// The full test case name includes the base name, plus any type.
constexpr std::size_t max_test_name_length = SNATCH_MAX_TEST_NAME_LENGTH;
} // namespace testing

// Forward declarations.
// ---------------------

namespace testing {
class registry;
}

// Implementation details.
// -----------------------

namespace testing::impl {
template<typename T>
constexpr std::string_view get_type_name() noexcept {
#if defined(__clang__)
    constexpr auto prefix   = std::string_view{"[T = "};
    constexpr auto suffix   = "]";
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
    constexpr auto prefix   = std::string_view{"with T = "};
    constexpr auto suffix   = "; ";
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
    constexpr auto prefix   = std::string_view{"get_type_name<"};
    constexpr auto suffix   = ">(void)";
    constexpr auto function = std::string_view{__FUNCSIG__};
#else
#    error Unsupported compiler
#endif

    const auto start = function.find(prefix) + prefix.size();
    const auto end   = function.find(suffix);
    const auto size  = end - start;

    return function.substr(start, size);
}
} // namespace testing::impl

namespace testing {
template<typename T>
constexpr std::string_view type_name = impl::get_type_name<T>();
}

namespace testing::impl {
template<typename T>
struct proxy;

template<typename... Args>
struct proxy<std::tuple<Args...>> {
    registry*        tests = nullptr;
    std::string_view name;
    std::string_view tags;

    template<typename F>
    const char* operator=(const F& func) noexcept;
};

struct test_case;

using test_ptr = void (*)(test_case&);

template<typename T, typename F>
constexpr test_ptr to_test_case_ptr(const F&) noexcept {
    return [](test_case& t) { F{}.template operator()<T>(t); };
}

enum class test_state { not_run, success, skipped, failed };

struct test_case {
    std::string_view name;
    std::string_view tags;
    std::string_view type;
    test_ptr         func  = nullptr;
    test_state       state = test_state::not_run;
    std::size_t      tests = 0;
};

template<typename ElemType>
class basic_small_vector {
    ElemType*   buffer_ptr  = nullptr;
    std::size_t buffer_size = 0;
    std::size_t data_size   = 0;

public:
    constexpr explicit basic_small_vector(ElemType* b, std::size_t bl) :
        buffer_ptr(b), buffer_size(bl) {}

    constexpr std::size_t capacity() const noexcept {
        return buffer_size;
    }
    constexpr std::size_t available() const noexcept {
        return capacity() - size();
    }
    constexpr std::size_t size() const noexcept {
        return data_size;
    }
    constexpr bool empty() const noexcept {
        return data_size == 0;
    }
    constexpr void clear() noexcept {
        data_size = 0;
    }
    constexpr void resize(std::size_t size) noexcept {
        data_size = size;
    }
    constexpr void grow(std::size_t elem) noexcept {
        data_size += elem;
    }
    constexpr ElemType* data() noexcept {
        return buffer_ptr;
    }
    constexpr const ElemType* data() const noexcept {
        return buffer_ptr;
    }
    constexpr ElemType* begin() noexcept {
        return data();
    }
    constexpr ElemType* end() noexcept {
        return begin() + size();
    }
    constexpr const ElemType* begin() const noexcept {
        return data();
    }
    constexpr const ElemType* end() const noexcept {
        return begin() + size();
    }
    constexpr const ElemType* cbegin() const noexcept {
        return data();
    }
    constexpr const ElemType* cend() const noexcept {
        return begin() + size();
    }
};

template<typename ElemType, std::size_t MaxLength>
class small_vector {
    std::array<ElemType, MaxLength> data_buffer;
    basic_small_vector<ElemType>    vector =
        basic_small_vector<ElemType>(data_buffer.data(), MaxLength);

public:
    constexpr std::size_t capacity() const noexcept {
        return MaxLength;
    }
    constexpr std::size_t available() const noexcept {
        return vector.available();
    }
    constexpr std::size_t size() const noexcept {
        return vector.size();
    }
    constexpr bool empty() const noexcept {
        return vector.empty();
    }
    constexpr void clear() noexcept {
        vector.clear();
    }
    constexpr void resize(std::size_t size) noexcept {
        vector.resize(size);
    }
    constexpr void grow(std::size_t elem) noexcept {
        vector.grow(elem);
    }
    constexpr ElemType* data() noexcept {
        return data_buffer.data();
    }
    constexpr const ElemType* data() const noexcept {
        return data_buffer.data();
    }
    constexpr ElemType* begin() noexcept {
        return data();
    }
    constexpr ElemType* end() noexcept {
        return begin() + size();
    }
    constexpr const ElemType* begin() const noexcept {
        return data();
    }
    constexpr const ElemType* end() const noexcept {
        return begin() + size();
    }
    constexpr const ElemType* cbegin() const noexcept {
        return data();
    }
    constexpr const ElemType* cend() const noexcept {
        return begin() + size();
    }
    constexpr operator basic_small_vector<ElemType> &() noexcept {
        return vector;
    }
    constexpr operator const basic_small_vector<ElemType> &() const noexcept {
        return vector;
    }
};

using basic_small_string = basic_small_vector<char>;

template<std::size_t MaxLength>
class small_string {
    std::array<char, MaxLength> data_buffer;
    basic_small_string          vector = basic_small_string(data_buffer.data(), MaxLength);

public:
    constexpr std::string_view str() const noexcept {
        return std::string_view(data(), length());
    }
    constexpr std::size_t capacity() const noexcept {
        return MaxLength;
    }
    constexpr std::size_t available() const noexcept {
        return vector.available();
    }
    constexpr std::size_t size() const noexcept {
        return vector.size();
    }
    constexpr std::size_t length() const noexcept {
        return vector.size();
    }
    constexpr bool empty() const noexcept {
        return vector.empty();
    }
    constexpr void clear() noexcept {
        vector.clear();
    }
    constexpr void resize(std::size_t length) noexcept {
        vector.resize(length);
    }
    constexpr void grow(std::size_t chars) noexcept {
        vector.grow(chars);
    }
    constexpr char* data() noexcept {
        return data_buffer.data();
    }
    constexpr const char* data() const noexcept {
        return data_buffer.data();
    }
    constexpr char* begin() noexcept {
        return data();
    }
    constexpr char* end() noexcept {
        return begin() + length();
    }
    constexpr const char* begin() const noexcept {
        return data();
    }
    constexpr const char* end() const noexcept {
        return begin() + length();
    }
    constexpr const char* cbegin() const noexcept {
        return data();
    }
    constexpr const char* cend() const noexcept {
        return begin() + length();
    }
    constexpr operator basic_small_string&() noexcept {
        return vector;
    }
    constexpr operator const basic_small_string&() const noexcept {
        return vector;
    }
};

[[nodiscard]] bool append(basic_small_string& ss, std::string_view value) noexcept;

[[nodiscard]] bool append(basic_small_string& ss, const void* ptr) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, std::nullptr_t) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, std::size_t i) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, std::ptrdiff_t i) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, float f) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, double f) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, bool value) noexcept;
[[nodiscard]] bool append(basic_small_string& ss, const std::string& str) noexcept;
template<typename T>
[[nodiscard]] bool append(basic_small_string& ss, T* ptr) noexcept {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        return append(ss, std::string_view(ptr));
    } else {
        return append(ss, static_cast<const void*>(ptr));
    }
}
template<std::size_t N>
[[nodiscard]] bool append(basic_small_string& ss, const char str[N]) noexcept {
    return append(ss, std::string_view(str));
}

template<typename T, typename U, typename... Args>
[[nodiscard]] bool append(basic_small_string& ss, T&& t, U&& u, Args&&... args) noexcept {
    if (!append(ss, std::forward<T>(t))) {
        return false;
    }
    return append(ss, std::forward<U>(u), std::forward<Args>(args)...);
}

void truncate_end(basic_small_string& ss) noexcept;

struct expression {
    small_string<max_expr_length> data;
    bool                          failed = false;

    template<typename T>
    void append(T&& value) noexcept {
        using TD = std::decay_t<T>;
        if constexpr (std::is_integral_v<TD>) {
            if constexpr (std::is_signed_v<TD>) {
                if (!impl::append(data, static_cast<std::ptrdiff_t>(value))) {
                    failed = true;
                }
            } else {
                if (!impl::append(data, static_cast<std::size_t>(value))) {
                    failed = true;
                }
            }
        } else if constexpr (
            std::is_pointer_v<TD> || std::is_floating_point_v<TD> || std::is_same_v<TD, bool> ||
            std::is_convertible_v<T, const char*> || std::is_convertible_v<T, std::string> ||
            std::is_convertible_v<T, std::string_view>) {
            if (!impl::append(data, value)) {
                failed = true;
            }
        } else {
            failed = true;
        }
    }

#define EXPR_OPERATOR(OP, INVERSE_OP)                                                              \
    template<typename T>                                                                           \
    expression& operator OP(const T& value) noexcept {                                             \
        if (!data.empty()) {                                                                       \
            append(" " #INVERSE_OP " ");                                                           \
        }                                                                                          \
        append(value);                                                                             \
        return *this;                                                                              \
    }

    EXPR_OPERATOR(<=, >)
    EXPR_OPERATOR(<, >=)
    EXPR_OPERATOR(>=, <)
    EXPR_OPERATOR(>, <=)
    EXPR_OPERATOR(==, !=)
    EXPR_OPERATOR(!=, ==)

#undef EXPR_OPERATOR
};
} // namespace testing::impl

// Test registry.
// --------------

namespace testing {
class registry {
    std::array<impl::test_case, max_test_cases> test_list;
    std::size_t                                 test_count = 0;

public:
    bool verbose = false;

    impl::proxy<std::tuple<>> add(std::string_view name, std::string_view tags) noexcept {
        return {this, name, tags};
    }

    template<typename T>
    impl::proxy<T> add_with_types(std::string_view name, std::string_view tags) noexcept {
        return {this, name, tags};
    }

    void register_test(
        std::string_view name,
        std::string_view tags,
        std::string_view type,
        impl::test_ptr   func) noexcept;

    template<typename... Args, typename F>
    void register_type_tests(std::string_view name, std::string_view tags, const F& func) noexcept {
        (register_test(name, tags, impl::get_type_name<Args>(), impl::to_test_case_ptr<Args>(func)),
         ...);
    }

    void print_location(
        const impl::test_case& current_case, const char* filename, int line_number) const noexcept;

    void print_failure() const noexcept;
    void print_skip() const noexcept;
    void print_details(std::string_view message) const noexcept;
    void print_details_expr(
        std::string_view        check,
        std::string_view        exp_str,
        const impl::expression& exp) const noexcept;

    void run(impl::test_case& t) noexcept;
    void set_state(impl::test_case& t, impl::test_state s) noexcept;

    bool run_all_tests() noexcept;
    bool run_tests_matching_name(std::string_view name) noexcept;
    bool run_tests_with_tag(std::string_view tag) noexcept;

    void list_all_tests() const noexcept;
    void list_all_tags() const noexcept;
    void list_tests_with_tag(std::string_view tag) const noexcept;

    impl::test_case*       begin() noexcept;
    impl::test_case*       end() noexcept;
    const impl::test_case* begin() const noexcept;
    const impl::test_case* end() const noexcept;
};

extern registry tests;
} // namespace testing

// Implementation details.
// -----------------------

namespace testing::impl {
template<typename... Args>
template<typename F>
const char* proxy<std::tuple<Args...>>::operator=(const F& func) noexcept {
    if constexpr (sizeof...(Args) > 0) {
        tests->template register_type_tests<Args...>(name, tags, func);
    } else {
        tests->register_test(name, tags, {}, func);
    }
    return name.data();
}
} // namespace testing::impl

// Builtin matchers.
// -----------------

namespace testing::matchers {
struct contains_substring {
    mutable impl::small_string<max_matcher_msg_length> description;
    std::string_view                                   pattern;

    explicit contains_substring(std::string_view pattern) noexcept;

    bool match(std::string_view message) const noexcept;

    std::string_view describe_fail(std::string_view message) const noexcept;
};

struct with_what_contains : private contains_substring {
    explicit with_what_contains(std::string_view pattern) noexcept;

    template<typename E>
    bool match(const E& e) const noexcept {
        return contains_substring::match(e.what());
    }

    template<typename E>
    std::string_view describe_fail(const E& e) const noexcept {
        return contains_substring::describe_fail(e.what());
    }
};
} // namespace testing::matchers

// Test macros.
// ------------

#define TESTING_CONCAT_IMPL(x, y) x##y
#define TESTING_MACRO_CONCAT(x, y) TESTING_CONCAT_IMPL(x, y)
#define TESTING_EXPR(x) testing::impl::expression{} <= x

#define TEST_CASE(NAME, TAGS)                                                                      \
    static const char* TESTING_MACRO_CONCAT(test_id_, __COUNTER__) =                               \
        testing::tests.add(NAME, TAGS) =                                                           \
            [](testing::impl::test_case & CURRENT_CASE [[maybe_unused]]) -> void

#define TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)                                                 \
    static const char* TESTING_MACRO_CONCAT(test_id_, __COUNTER__) =                               \
        testing::tests.add_with_types<TYPES>(NAME, TAGS) = []<typename TestType>(                  \
            testing::impl::test_case & CURRENT_CASE [[maybe_unused]]) -> void

#define REQUIRE(EXP)                                                                               \
    do {                                                                                           \
        ++CURRENT_CASE.tests;                                                                      \
        if (!(EXP)) {                                                                              \
            const auto EXP2 = TESTING_EXPR(EXP);                                                   \
            testing::tests.print_failure();                                                        \
            testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                       \
            testing::tests.print_details_expr("REQUIRE", #EXP, EXP2);                              \
            throw testing::impl::test_state::failed;                                               \
        }                                                                                          \
    } while (0)

#define CHECK(EXP)                                                                                 \
    do {                                                                                           \
        ++CURRENT_CASE.tests;                                                                      \
        if (!(EXP)) {                                                                              \
            const auto EXP2 = TESTING_EXPR(EXP);                                                   \
            testing::tests.print_failure();                                                        \
            testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                       \
            testing::tests.print_details_expr("CHECK", #EXP, EXP2);                                \
            testing::tests.set_state(CURRENT_CASE, testing::impl::test_state::failed);             \
        }                                                                                          \
    } while (0)

#define FAIL(MESSAGE)                                                                              \
    do {                                                                                           \
        testing::tests.print_failure();                                                            \
        testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                           \
        testing::tests.print_details(MESSAGE);                                                     \
        throw testing::impl::test_state::failed;                                                   \
    } while (0)

#define FAIL_CHECK(MESSAGE)                                                                        \
    do {                                                                                           \
        testing::tests.print_failure();                                                            \
        testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                           \
        testing::tests.print_details(MESSAGE);                                                     \
        testing::tests.set_state(CURRENT_CASE, testing::impl::test_state::failed);                 \
    } while (0)

#define SKIP(MESSAGE)                                                                              \
    do {                                                                                           \
        testing::tests.print_skip();                                                               \
        testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                           \
        testing::tests.print_details(MESSAGE);                                                     \
        throw testing::impl::test_state::skipped;                                                  \
    } while (0)

#define REQUIRE_THROWS_AS(EXPRESSION, EXCEPTION)                                                   \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL(#EXCEPTION " expected but no exception thrown");                                  \
        } catch (const EXCEPTION&) {                                                               \
            /* success */                                                                          \
        } catch (...) {                                                                            \
            testing::tests.print_failure();                                                        \
            testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                       \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                testing::tests.print_details(                                                      \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                testing::tests.print_details(e.what());                                            \
            } catch (...) {                                                                        \
                testing::tests.print_details(#EXCEPTION                                            \
                                             " expected but other unknown exception thrown");      \
            }                                                                                      \
            throw testing::impl::test_state::failed;                                               \
        }                                                                                          \
    } while (0)

#define CHECK_THROWS_AS(EXPRESSION, EXCEPTION)                                                     \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL_CHECK(#EXCEPTION " expected but no exception thrown");                            \
        } catch (const EXCEPTION&) {                                                               \
            /* success */                                                                          \
        } catch (...) {                                                                            \
            testing::tests.print_failure();                                                        \
            testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                       \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                testing::tests.print_details(                                                      \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                testing::tests.print_details(e.what());                                            \
            } catch (...) {                                                                        \
                testing::tests.print_details(#EXCEPTION                                            \
                                             " expected but other unknown exception thrown");      \
            }                                                                                      \
            testing::tests.set_state(CURRENT_CASE, testing::impl::test_state::failed);             \
        }                                                                                          \
    } while (0)

#define REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                                     \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL(#EXCEPTION " expected but no exception thrown");                                  \
        } catch (const EXCEPTION& e) {                                                             \
            if (!(MATCHER).match(e)) {                                                             \
                testing::tests.print_failure();                                                    \
                testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                   \
                testing::tests.print_details("could not match caught " #EXCEPTION                  \
                                             " with expected content:");                           \
                testing::tests.print_details((MATCHER).describe_fail(e));                          \
                throw testing::impl::test_state::failed;                                           \
            }                                                                                      \
        } catch (...) {                                                                            \
            testing::tests.print_failure();                                                        \
            testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                       \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                testing::tests.print_details(                                                      \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                testing::tests.print_details(e.what());                                            \
            } catch (...) {                                                                        \
                testing::tests.print_details(#EXCEPTION                                            \
                                             " expected but other unknown exception thrown");      \
            }                                                                                      \
            throw testing::impl::test_state::failed;                                               \
        }                                                                                          \
    } while (0)

#define CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                                       \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL_CHECK(#EXCEPTION " expected but no exception thrown");                            \
        } catch (const EXCEPTION& e) {                                                             \
            if (!(MATCHER).match(e)) {                                                             \
                testing::tests.print_failure();                                                    \
                testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                   \
                testing::tests.print_details("could not match caught " #EXCEPTION                  \
                                             " with expected content:");                           \
                testing::tests.print_details((MATCHER).describe_fail(e));                          \
                testing::tests.set_state(CURRENT_CASE, testing::impl::test_state::failed);         \
            }                                                                                      \
        } catch (...) {                                                                            \
            testing::tests.print_failure();                                                        \
            testing::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                       \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                testing::tests.print_details(                                                      \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                testing::tests.print_details(e.what());                                            \
            } catch (...) {                                                                        \
                testing::tests.print_details(#EXCEPTION                                            \
                                             " expected but other unknown exception thrown");      \
            }                                                                                      \
            testing::tests.set_state(CURRENT_CASE, testing::impl::test_state::failed);             \
        }                                                                                          \
    } while (0)

#endif
