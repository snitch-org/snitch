#ifndef SNATCH_HPP
#define SNATCH_HPP

#include <array> // for small_vector
#include <cstddef> // for std::size_t
#include <exception> // for std::exception
#include <initializer_list> // for std::initializer_list
#include <string_view> // for all strings
#include <tuple> // for typed tests
#include <type_traits> // for std::is_nothrow_*

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
#if !defined(SNATCH_MAX_UNIQUE_TAGS)
#    define SNATCH_MAX_UNIQUE_TAGS 1'024
#endif

// Testing framework configuration.
// --------------------------------

namespace snatch {
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
// Maximum number of unique tags in the whole program.
constexpr std::size_t max_unique_tags = SNATCH_MAX_UNIQUE_TAGS;
} // namespace snatch

// Forward declarations.
// ---------------------

namespace snatch {
class registry;
}

// Implementation details.
// -----------------------

namespace snatch::impl {
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
} // namespace snatch::impl

namespace snatch {
template<typename T>
constexpr std::string_view type_name = impl::get_type_name<T>();
}

namespace snatch::impl {
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

[[noreturn]] void terminate_with(std::string_view msg) noexcept;

template<typename ElemType>
class basic_small_vector {
    ElemType*   buffer_ptr  = nullptr;
    std::size_t buffer_size = 0;
    std::size_t data_size   = 0;

public:
    constexpr explicit basic_small_vector(ElemType* b, std::size_t bl) noexcept :
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
        if (size > buffer_size) {
            impl::terminate_with("small vector is full");
        }

        data_size = size;
    }
    constexpr void grow(std::size_t elem) noexcept {
        if (data_size + elem > buffer_size) {
            impl::terminate_with("small vector is full");
        }

        data_size += elem;
    }
    constexpr ElemType&
    push_back(const ElemType& t) noexcept(std::is_nothrow_copy_assignable_v<ElemType>) {
        if (data_size == buffer_size) {
            impl::terminate_with("small vector is full");
        }

        ++data_size;

        ElemType& elem = buffer_ptr[data_size - 1];
        elem           = t;

        return elem;
    }
    constexpr ElemType&
    push_back(ElemType&& t) noexcept(std::is_nothrow_move_assignable_v<ElemType>) {
        if (data_size == buffer_size) {
            impl::terminate_with("small vector is full");
        }

        ++data_size;
        ElemType& elem = buffer_ptr[data_size - 1];
        elem           = std::move(t);

        return elem;
    }
    constexpr ElemType& back() noexcept {
        if (data_size == 0) {
            impl::terminate_with("back() called on empty vector");
        }

        return buffer_ptr[data_size - 1];
    }
    constexpr const ElemType& back() const noexcept {
        if (data_size == 0) {
            impl::terminate_with("back() called on empty vector");
        }

        return buffer_ptr[data_size - 1];
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
    constexpr small_vector() noexcept = default;
    constexpr small_vector(const small_vector& other) noexcept {
        data_buffer = other.data_buffer;
        vector.resize(other.vector.size());
    }
    constexpr small_vector(small_vector&& other) noexcept {
        data_buffer = other.data_buffer;
        vector.resize(other.vector.size());
    }
    constexpr small_vector(std::initializer_list<ElemType> list) noexcept(
        noexcept(vector.push_back(std::declval<ElemType>()))) {
        for (const auto& e : list) {
            push_back(e);
        }
    }
    constexpr small_vector& operator=(const small_vector& other) noexcept {
        data_buffer = other.data_buffer;
        vector.resize(other.vector.size());
        return *this;
    }
    constexpr small_vector& operator=(small_vector&& other) noexcept {
        data_buffer = other.data_buffer;
        vector.resize(other.vector.size());
        return *this;
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
    template<typename U>
    constexpr ElemType& push_back(U&& t) noexcept(noexcept(vector.push_back(t))) {
        return vector.push_back(t);
    }
    constexpr ElemType& back() noexcept {
        return vector.back();
    }
    constexpr const ElemType& back() const noexcept {
        return vector.back();
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
    constexpr ElemType& operator[](std::size_t i) noexcept {
        if (i >= size()) {
            impl::terminate_with("operator[] called with incorrect index");
        }
        return data()[i];
    }
    constexpr const ElemType& operator[](std::size_t i) const noexcept {
        if (i >= size()) {
            impl::terminate_with("operator[] called with incorrect index");
        }
        return data()[i];
    }
};

using basic_small_string = basic_small_vector<char>;

template<std::size_t MaxLength>
class small_string {
    std::array<char, MaxLength> data_buffer;
    basic_small_string          vector = basic_small_string(data_buffer.data(), MaxLength);

public:
    constexpr small_string() noexcept = default;
    constexpr small_string(const small_string& other) noexcept {
        vector.resize(other.vector.size());
        data_buffer = other.data_buffer;
    }
    constexpr small_string(small_string&& other) noexcept {
        vector.resize(other.vector.size());
        data_buffer = other.data_buffer;
    }
    constexpr small_string(std::string_view str) noexcept {
        vector.resize(str.size());
        for (std::size_t i = 0; i < str.size(); ++i) {
            data_buffer[i] = str[i];
        }
    }
    constexpr small_string& operator=(const small_string& other) noexcept {
        data_buffer = other.data_buffer;
        vector.resize(other.vector.size());
        return *this;
    }
    constexpr small_string& operator=(small_string&& other) noexcept {
        data_buffer = other.data_buffer;
        vector.resize(other.vector.size());
        return *this;
    }
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
    constexpr char& push_back(char t) noexcept {
        return vector.push_back(t);
    }
    constexpr char& back() noexcept {
        return vector.back();
    }
    constexpr const char& back() const noexcept {
        return vector.back();
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
} // namespace snatch::impl

// Test registry.
// --------------

namespace snatch {
class registry {
    impl::small_vector<impl::test_case, max_test_cases> test_list;

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
} // namespace snatch

// Implementation details.
// -----------------------

namespace snatch::impl {
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
} // namespace snatch::impl

// Builtin matchers.
// -----------------

namespace snatch::matchers {
struct contains_substring {
    mutable impl::small_string<max_matcher_msg_length> description_buffer;
    std::string_view                                   substring_pattern;

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
} // namespace snatch::matchers

// Compiler warning handling.
// --------------------------

// clang-format off
#if defined(__clang__)
#    define WARNING_PUSH _Pragma("clang diagnostic push")
#    define WARNING_POP _Pragma("clang diagnostic pop")
#    define WARNING_DISABLE_PARENTHESES _Pragma("clang diagnostic ignored \"-Wparentheses\"")
#    define WARNING_DISABLE_CONSTANT_COMPARISON do {} while (0)
#elif defined(__GNUC__)
#    define WARNING_PUSH _Pragma("GCC diagnostic push")
#    define WARNING_POP _Pragma("GCC diagnostic pop")
#    define WARNING_DISABLE_PARENTHESES _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#    define WARNING_DISABLE_CONSTANT_COMPARISON do {} while (0)
#elif defined(_MSC_VER)
#    define WARNING_PUSH _Pragma("warning(push)")
#    define WARNING_POP _Pragma("warning(pop)")
#    define WARNING_DISABLE_PARENTHESES do {} while (0)
#    define WARNING_DISABLE_CONSTANT_COMPARISON _Pragma("warning(disable: 4127)")
#else
#    define WARNING_PUSH do {} while (0)
#    define WARNING_POP do {} while (0)
#    define WARNING_DISABLE_PARENTHESES do {} while (0)
#    define WARNING_DISABLE_CONSTANT_COMPARISON do {} while (0)
#endif
// clang-format on

// Test macros.
// ------------

#define TESTING_CONCAT_IMPL(x, y) x##y
#define TESTING_MACRO_CONCAT(x, y) TESTING_CONCAT_IMPL(x, y)
#define TESTING_EXPR(x) snatch::impl::expression{} <= x

#define TEST_CASE(NAME, TAGS)                                                                      \
    static const char* TESTING_MACRO_CONCAT(test_id_, __COUNTER__) =                               \
        snatch::tests.add(NAME, TAGS) =                                                            \
            [](snatch::impl::test_case & CURRENT_CASE [[maybe_unused]]) -> void

#define TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)                                                 \
    static const char* TESTING_MACRO_CONCAT(test_id_, __COUNTER__) =                               \
        snatch::tests.add_with_types<TYPES>(NAME, TAGS) =                                          \
            []<typename TestType>(snatch::impl::test_case & CURRENT_CASE [[maybe_unused]]) -> void

#define REQUIRE(EXP)                                                                               \
    do {                                                                                           \
        ++CURRENT_CASE.tests;                                                                      \
        WARNING_PUSH;                                                                              \
        WARNING_DISABLE_PARENTHESES;                                                               \
        WARNING_DISABLE_CONSTANT_COMPARISON;                                                       \
        if (!(EXP)) {                                                                              \
            const auto EXP2 = TESTING_EXPR(EXP);                                                   \
            snatch::tests.print_failure();                                                         \
            snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                        \
            snatch::tests.print_details_expr("REQUIRE", #EXP, EXP2);                               \
            throw snatch::impl::test_state::failed;                                                \
        }                                                                                          \
        WARNING_POP;                                                                               \
    } while (0)

#define CHECK(EXP)                                                                                 \
    do {                                                                                           \
        ++CURRENT_CASE.tests;                                                                      \
        WARNING_PUSH;                                                                              \
        WARNING_DISABLE_PARENTHESES;                                                               \
        WARNING_DISABLE_CONSTANT_COMPARISON;                                                       \
        if (!(EXP)) {                                                                              \
            const auto EXP2 = TESTING_EXPR(EXP);                                                   \
            snatch::tests.print_failure();                                                         \
            snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                        \
            snatch::tests.print_details_expr("CHECK", #EXP, EXP2);                                 \
            snatch::tests.set_state(CURRENT_CASE, snatch::impl::test_state::failed);               \
        }                                                                                          \
        WARNING_POP;                                                                               \
    } while (0)

#define FAIL(MESSAGE)                                                                              \
    do {                                                                                           \
        snatch::tests.print_failure();                                                             \
        snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                            \
        snatch::tests.print_details(MESSAGE);                                                      \
        throw snatch::impl::test_state::failed;                                                    \
    } while (0)

#define FAIL_CHECK(MESSAGE)                                                                        \
    do {                                                                                           \
        snatch::tests.print_failure();                                                             \
        snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                            \
        snatch::tests.print_details(MESSAGE);                                                      \
        snatch::tests.set_state(CURRENT_CASE, snatch::impl::test_state::failed);                   \
    } while (0)

#define SKIP(MESSAGE)                                                                              \
    do {                                                                                           \
        snatch::tests.print_skip();                                                                \
        snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                            \
        snatch::tests.print_details(MESSAGE);                                                      \
        throw snatch::impl::test_state::skipped;                                                   \
    } while (0)

#define REQUIRE_THROWS_AS(EXPRESSION, EXCEPTION)                                                   \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL(#EXCEPTION " expected but no exception thrown");                                  \
        } catch (const EXCEPTION&) {                                                               \
            /* success */                                                                          \
        } catch (...) {                                                                            \
            snatch::tests.print_failure();                                                         \
            snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                        \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                snatch::tests.print_details(                                                       \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                snatch::tests.print_details(e.what());                                             \
            } catch (...) {                                                                        \
                snatch::tests.print_details(#EXCEPTION                                             \
                                            " expected but other unknown exception thrown");       \
            }                                                                                      \
            throw snatch::impl::test_state::failed;                                                \
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
            snatch::tests.print_failure();                                                         \
            snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                        \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                snatch::tests.print_details(                                                       \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                snatch::tests.print_details(e.what());                                             \
            } catch (...) {                                                                        \
                snatch::tests.print_details(#EXCEPTION                                             \
                                            " expected but other unknown exception thrown");       \
            }                                                                                      \
            snatch::tests.set_state(CURRENT_CASE, snatch::impl::test_state::failed);               \
        }                                                                                          \
    } while (0)

#define REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                                     \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL(#EXCEPTION " expected but no exception thrown");                                  \
        } catch (const EXCEPTION& e) {                                                             \
            if (!(MATCHER).match(e)) {                                                             \
                snatch::tests.print_failure();                                                     \
                snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                    \
                snatch::tests.print_details("could not match caught " #EXCEPTION                   \
                                            " with expected content:");                            \
                snatch::tests.print_details((MATCHER).describe_fail(e));                           \
                throw snatch::impl::test_state::failed;                                            \
            }                                                                                      \
        } catch (...) {                                                                            \
            snatch::tests.print_failure();                                                         \
            snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                        \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                snatch::tests.print_details(                                                       \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                snatch::tests.print_details(e.what());                                             \
            } catch (...) {                                                                        \
                snatch::tests.print_details(#EXCEPTION                                             \
                                            " expected but other unknown exception thrown");       \
            }                                                                                      \
            throw snatch::impl::test_state::failed;                                                \
        }                                                                                          \
    } while (0)

#define CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                                       \
    do {                                                                                           \
        try {                                                                                      \
            EXPRESSION;                                                                            \
            FAIL_CHECK(#EXCEPTION " expected but no exception thrown");                            \
        } catch (const EXCEPTION& e) {                                                             \
            if (!(MATCHER).match(e)) {                                                             \
                snatch::tests.print_failure();                                                     \
                snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                    \
                snatch::tests.print_details("could not match caught " #EXCEPTION                   \
                                            " with expected content:");                            \
                snatch::tests.print_details((MATCHER).describe_fail(e));                           \
                snatch::tests.set_state(CURRENT_CASE, snatch::impl::test_state::failed);           \
            }                                                                                      \
        } catch (...) {                                                                            \
            snatch::tests.print_failure();                                                         \
            snatch::tests.print_location(CURRENT_CASE, __FILE__, __LINE__);                        \
            try {                                                                                  \
                throw;                                                                             \
            } catch (const std::exception& e) {                                                    \
                snatch::tests.print_details(                                                       \
                    #EXCEPTION " expected but other std::exception thrown; message:");             \
                snatch::tests.print_details(e.what());                                             \
            } catch (...) {                                                                        \
                snatch::tests.print_details(#EXCEPTION                                             \
                                            " expected but other unknown exception thrown");       \
            }                                                                                      \
            snatch::tests.set_state(CURRENT_CASE, snatch::impl::test_state::failed);               \
        }                                                                                          \
    } while (0)

#endif
