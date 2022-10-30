#ifndef SNATCH_HPP
#define SNATCH_HPP

#if !defined(SNATCH_MAX_TEST_CASES)
#    define SNATCH_MAX_TEST_CASES 5'000
#endif
#if !defined(SNATCH_MAX_NESTED_SECTIONS)
#    define SNATCH_MAX_NESTED_SECTIONS 8
#endif
#if !defined(SNATCH_MAX_EXPR_LENGTH)
#    define SNATCH_MAX_EXPR_LENGTH 1'024
#endif
#if !defined(SNATCH_MAX_MESSAGE_LENGTH)
#    define SNATCH_MAX_MESSAGE_LENGTH 1'024
#endif
#if !defined(SNATCH_MAX_TEST_NAME_LENGTH)
#    define SNATCH_MAX_TEST_NAME_LENGTH 1'024
#endif
#if !defined(SNATCH_MAX_UNIQUE_TAGS)
#    define SNATCH_MAX_UNIQUE_TAGS 1'024
#endif
#if !defined(SNATCH_MAX_COMMAND_LINE_ARGS)
#    define SNATCH_MAX_COMMAND_LINE_ARGS 1'024
#endif
#if !defined(SNATCH_DEFINE_MAIN)
#    define SNATCH_DEFINE_MAIN 1
#endif
#if !defined(SNATCH_WITH_EXCEPTIONS)
#    define SNATCH_WITH_EXCEPTIONS 1
#endif
#if !defined(SNATCH_WITH_SHORTHAND_MACROS)
#    define SNATCH_WITH_SHORTHAND_MACROS 1
#endif
#if !defined(SNATCH_DEFAULT_WITH_COLOR)
#    define SNATCH_DEFAULT_WITH_COLOR 1
#endif

#if defined(_MSC_VER)
#    if defined(_KERNEL_MODE) || (defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS)
#        define SNATCH_EXCEPTIONS_NOT_AVAILABLE
#    endif
#elif defined(__clang__) || defined(__GNUC__)
#    if !defined(__EXCEPTIONS)
#        define SNATCH_EXCEPTIONS_NOT_AVAILABLE
#    endif
#endif

#if defined(SNATCH_EXCEPTIONS_NOT_AVAILABLE)
#    undef SNATCH_WITH_EXCEPTIONS
#    define SNATCH_WITH_EXCEPTIONS 0
#endif

#include <array> // for small_vector
#include <cstddef> // for std::size_t
#if SNATCH_WITH_EXCEPTIONS
#    include <exception> // for std::exception
#endif
#include <initializer_list> // for std::initializer_list
#include <optional> // for cli
#include <string_view> // for all strings
#include <tuple> // for typed tests
#include <type_traits> // for std::is_nothrow_*
#include <variant> // for events

// Testing framework configuration.
// --------------------------------

namespace snatch {
// Maximum number of test cases in the whole program.
// A "test case" is created for each uses of the `*_TEST_CASE` macros,
// and for each type for the `TEMPLATE_LIST_TEST_CASE` macro.
constexpr std::size_t max_test_cases = SNATCH_MAX_TEST_CASES;
// Maximum depth of nested sections in a test case (section in section in section ...).
constexpr std::size_t max_nested_sections = SNATCH_MAX_NESTED_SECTIONS;
// Maximum length of a `CHECK(...)` or `REQUIRE(...)` expression,
// beyond which automatic variable printing is disabled.
constexpr std::size_t max_expr_length = SNATCH_MAX_EXPR_LENGTH;
// Maximum length of error messages.
constexpr std::size_t max_message_length = SNATCH_MAX_MESSAGE_LENGTH;
// Maximum length of a full test case name.
// The full test case name includes the base name, plus any type.
constexpr std::size_t max_test_name_length = SNATCH_MAX_TEST_NAME_LENGTH;
// Maximum number of unique tags in the whole program.
constexpr std::size_t max_unique_tags = SNATCH_MAX_UNIQUE_TAGS;
// Maximum number of command line arguments.
constexpr std::size_t max_command_line_args = SNATCH_MAX_COMMAND_LINE_ARGS;
} // namespace snatch

// Forward declarations and public utilities.
// ------------------------------------------

namespace snatch {
class registry;

struct test_id {
    std::string_view name;
    std::string_view tags;
    std::string_view type;
};

struct section_id {
    std::string_view name;
    std::string_view description;
};
} // namespace snatch

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

// Public utilities.
// ------------------------------------------------

namespace snatch {
template<typename T>
constexpr std::string_view type_name = impl::get_type_name<T>();

[[noreturn]] void terminate_with(std::string_view msg) noexcept;

struct registry;
} // namespace snatch

// Public utilities: small_vector.
// -------------------------------

namespace snatch {
template<typename ElemType>
class small_vector_span {
    ElemType*    buffer_ptr  = nullptr;
    std::size_t  buffer_size = 0;
    std::size_t* data_size   = nullptr;

public:
    constexpr explicit small_vector_span(ElemType* b, std::size_t bl, std::size_t* s) noexcept :
        buffer_ptr(b), buffer_size(bl), data_size(s) {}

    constexpr std::size_t capacity() const noexcept {
        return buffer_size;
    }
    constexpr std::size_t available() const noexcept {
        return capacity() - size();
    }
    constexpr std::size_t size() const noexcept {
        return *data_size;
    }
    constexpr bool empty() const noexcept {
        return *data_size == 0;
    }
    constexpr void clear() noexcept {
        *data_size = 0;
    }
    constexpr void resize(std::size_t size) noexcept {
        if (size > buffer_size) {
            terminate_with("small vector is full");
        }

        *data_size = size;
    }
    constexpr void grow(std::size_t elem) noexcept {
        if (*data_size + elem > buffer_size) {
            terminate_with("small vector is full");
        }

        *data_size += elem;
    }
    constexpr ElemType&
    push_back(const ElemType& t) noexcept(std::is_nothrow_copy_assignable_v<ElemType>) {
        if (*data_size == buffer_size) {
            terminate_with("small vector is full");
        }

        ++*data_size;

        ElemType& elem = buffer_ptr[*data_size - 1];
        elem           = t;

        return elem;
    }
    constexpr ElemType&
    push_back(ElemType&& t) noexcept(std::is_nothrow_move_assignable_v<ElemType>) {
        if (*data_size == buffer_size) {
            terminate_with("small vector is full");
        }

        ++*data_size;
        ElemType& elem = buffer_ptr[*data_size - 1];
        elem           = std::move(t);

        return elem;
    }
    constexpr void pop_back() noexcept {
        if (*data_size == 0) {
            terminate_with("pop_back() called on empty vector");
        }

        --*data_size;
    }
    constexpr ElemType& back() noexcept {
        if (*data_size == 0) {
            terminate_with("back() called on empty vector");
        }

        return buffer_ptr[*data_size - 1];
    }
    constexpr const ElemType& back() const noexcept {
        if (*data_size == 0) {
            terminate_with("back() called on empty vector");
        }

        return buffer_ptr[*data_size - 1];
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
    constexpr ElemType& operator[](std::size_t i) noexcept {
        if (i >= size()) {
            terminate_with("operator[] called with incorrect index");
        }
        return buffer_ptr[i];
    }
    constexpr const ElemType& operator[](std::size_t i) const noexcept {
        if (i >= size()) {
            terminate_with("operator[] called with incorrect index");
        }
        return buffer_ptr[i];
    }
};

template<typename ElemType>
class small_vector_span<const ElemType> {
    const ElemType*    buffer_ptr  = nullptr;
    std::size_t        buffer_size = 0;
    const std::size_t* data_size   = nullptr;

public:
    constexpr explicit small_vector_span(
        const ElemType* b, std::size_t bl, const std::size_t* s) noexcept :
        buffer_ptr(b), buffer_size(bl), data_size(s) {}

    constexpr std::size_t capacity() const noexcept {
        return buffer_size;
    }
    constexpr std::size_t available() const noexcept {
        return capacity() - size();
    }
    constexpr std::size_t size() const noexcept {
        return *data_size;
    }
    constexpr bool empty() const noexcept {
        return *data_size == 0;
    }
    constexpr const ElemType& back() const noexcept {
        if (*data_size == 0) {
            terminate_with("back() called on empty vector");
        }

        return buffer_ptr[*data_size - 1];
    }
    constexpr const ElemType* data() const noexcept {
        return buffer_ptr;
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
    constexpr const ElemType& operator[](std::size_t i) const noexcept {
        if (i >= size()) {
            terminate_with("operator[] called with incorrect index");
        }
        return buffer_ptr[i];
    }
};

template<typename ElemType, std::size_t MaxLength>
class small_vector {
    std::array<ElemType, MaxLength> data_buffer;
    std::size_t                     data_size = 0;

public:
    constexpr small_vector() noexcept                          = default;
    constexpr small_vector(const small_vector& other) noexcept = default;
    constexpr small_vector(small_vector&& other) noexcept      = default;
    constexpr small_vector(std::initializer_list<ElemType> list) noexcept(
        noexcept(span().push_back(std::declval<ElemType>()))) {
        for (const auto& e : list) {
            span().push_back(e);
        }
    }
    constexpr small_vector& operator=(const small_vector& other) noexcept = default;
    constexpr small_vector& operator=(small_vector&& other) noexcept = default;
    constexpr std::size_t   capacity() const noexcept {
        return MaxLength;
    }
    constexpr std::size_t available() const noexcept {
        return MaxLength - data_size;
    }
    constexpr std::size_t size() const noexcept {
        return data_size;
    }
    constexpr bool empty() const noexcept {
        return data_size == 0u;
    }
    constexpr void clear() noexcept {
        span().clear();
    }
    constexpr void resize(std::size_t size) noexcept {
        span().resize(size);
    }
    constexpr void grow(std::size_t elem) noexcept {
        span().grow(elem);
    }
    template<typename U = const ElemType&>
    constexpr ElemType& push_back(U&& t) noexcept(noexcept(this->span().push_back(t))) {
        return this->span().push_back(t);
    }
    constexpr void pop_back() noexcept {
        return span().pop_back();
    }
    constexpr ElemType& back() noexcept {
        return span().back();
    }
    constexpr const ElemType& back() const noexcept {
        return const_cast<small_vector*>(this)->span().back();
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
    constexpr small_vector_span<ElemType> span() noexcept {
        return small_vector_span<ElemType>(data_buffer.data(), MaxLength, &data_size);
    }
    constexpr small_vector_span<const ElemType> span() const noexcept {
        return small_vector_span<const ElemType>(data_buffer.data(), MaxLength, &data_size);
    }
    constexpr operator small_vector_span<ElemType>() noexcept {
        return span();
    }
    constexpr operator small_vector_span<const ElemType>() const noexcept {
        return span();
    }
    constexpr ElemType& operator[](std::size_t i) noexcept {
        return span()[i];
    }
    constexpr const ElemType& operator[](std::size_t i) const noexcept {
        return const_cast<small_vector*>(this)->span()[i];
    }
};
} // namespace snatch

// Public utilities: small_string.
// -------------------------------

namespace snatch {
using small_string_span = small_vector_span<char>;
using small_string_view = small_vector_span<const char>;

template<std::size_t MaxLength>
class small_string {
    std::array<char, MaxLength> data_buffer;
    std::size_t                 data_size = 0u;

public:
    constexpr small_string() noexcept                          = default;
    constexpr small_string(const small_string& other) noexcept = default;
    constexpr small_string(small_string&& other) noexcept      = default;
    constexpr small_string(std::string_view str) noexcept {
        resize(str.size());
        for (std::size_t i = 0; i < str.size(); ++i) {
            data_buffer[i] = str[i];
        }
    }
    constexpr small_string&    operator=(const small_string& other) noexcept = default;
    constexpr small_string&    operator=(small_string&& other) noexcept = default;
    constexpr std::string_view str() const noexcept {
        return std::string_view(data(), length());
    }
    constexpr std::size_t capacity() const noexcept {
        return MaxLength;
    }
    constexpr std::size_t available() const noexcept {
        return MaxLength - data_size;
    }
    constexpr std::size_t size() const noexcept {
        return data_size;
    }
    constexpr std::size_t length() const noexcept {
        return data_size;
    }
    constexpr bool empty() const noexcept {
        return data_size == 0u;
    }
    constexpr void clear() noexcept {
        span().clear();
    }
    constexpr void resize(std::size_t length) noexcept {
        span().resize(length);
    }
    constexpr void grow(std::size_t chars) noexcept {
        span().grow(chars);
    }
    constexpr char& push_back(char t) noexcept {
        return span().push_back(t);
    }
    constexpr void pop_back() noexcept {
        return span().pop_back();
    }
    constexpr char& back() noexcept {
        return span().back();
    }
    constexpr const char& back() const noexcept {
        return span().back();
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
    constexpr small_string_span span() noexcept {
        return small_string_span(data_buffer.data(), MaxLength, &data_size);
    }
    constexpr small_string_view span() const noexcept {
        return small_string_view(data_buffer.data(), MaxLength, &data_size);
    }
    constexpr operator small_string_span() noexcept {
        return span();
    }
    constexpr operator small_string_view() const noexcept {
        return span();
    }
    constexpr operator std::string_view() const noexcept {
        return std::string_view(data(), length());
    }
};

[[nodiscard]] bool append(small_string_span ss, std::string_view value) noexcept;

[[nodiscard]] bool append(small_string_span ss, const void* ptr) noexcept;
[[nodiscard]] bool append(small_string_span ss, std::nullptr_t) noexcept;
[[nodiscard]] bool append(small_string_span ss, std::size_t i) noexcept;
[[nodiscard]] bool append(small_string_span ss, std::ptrdiff_t i) noexcept;
[[nodiscard]] bool append(small_string_span ss, float f) noexcept;
[[nodiscard]] bool append(small_string_span ss, double f) noexcept;
[[nodiscard]] bool append(small_string_span ss, bool value) noexcept;
template<typename T>
[[nodiscard]] bool append(small_string_span ss, T* ptr) noexcept {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        return append(ss, std::string_view(ptr));
    } else {
        return append(ss, static_cast<const void*>(ptr));
    }
}
template<std::size_t N>
[[nodiscard]] bool append(small_string_span ss, const char str[N]) noexcept {
    return append(ss, std::string_view(str));
}

template<typename T, typename U, typename... Args>
[[nodiscard]] bool append(small_string_span ss, T&& t, U&& u, Args&&... args) noexcept {
    return append(ss, std::forward<T>(t)) && append(ss, std::forward<U>(u)) &&
           (append(ss, std::forward<Args>(args)) && ...);
}

void truncate_end(small_string_span ss) noexcept;

template<typename... Args>
bool append_or_truncate(small_string_span ss, Args&&... args) noexcept {
    if (!append(ss, std::forward<Args>(args)...)) {
        truncate_end(ss);
        return false;
    }

    return true;
}

[[nodiscard]] bool replace_all(
    small_string_span string, std::string_view pattern, std::string_view replacement) noexcept;
} // namespace snatch

// Public utilities: small_function.
// ---------------------------------

namespace snatch {
template<typename... Args>
struct overload : Args... {
    using Args::operator()...;
};

template<typename... Args>
overload(Args...) -> overload<Args...>;

template<auto T>
struct constant {
    static constexpr auto value = T;
};

template<typename T>
class small_function {
    static_assert(!std::is_same_v<T, T>, "incorrect template parameter for small_function");
};

template<typename Ret, typename... Args>
class small_function<Ret(Args...) noexcept> {
    using function_ptr            = Ret (*)(Args...) noexcept;
    using function_data_ptr       = Ret (*)(void*, Args...) noexcept;
    using function_const_data_ptr = Ret (*)(const void*, Args...) noexcept;

    struct function_and_data_ptr {
        void*             data = nullptr;
        function_data_ptr ptr;
    };

    struct function_and_const_data_ptr {
        const void*             data = nullptr;
        function_const_data_ptr ptr;
    };

    using data_type = std::
        variant<std::monostate, function_ptr, function_and_data_ptr, function_and_const_data_ptr>;

    data_type data;

public:
    constexpr small_function() = default;

    constexpr small_function(function_ptr ptr) noexcept : data{ptr} {};

    template<typename T, auto M>
    constexpr small_function(T& obj, constant<M>) noexcept :
        data{function_and_data_ptr{&obj, [](void* ptr, Args... args) noexcept {
                                       (static_cast<T*>(ptr)->*constant<M>::value)(
                                           std::move(args)...);
                                   }}} {};

    template<typename T, auto M>
    constexpr small_function(const T& obj, constant<M>) noexcept :
        data{function_and_const_data_ptr{&obj, [](const void* ptr, Args... args) noexcept {
                                             (static_cast<const T*>(ptr)->*constant<M>::value)(
                                                 std::move(args)...);
                                         }}} {};

    template<typename... CArgs>
    constexpr Ret operator()(CArgs&&... args) const noexcept {
        return std::visit(
            overload{
                [](std::monostate) {},
                [&](function_ptr f) { return (*f)(std::forward<CArgs>(args)...); },
                [&](const function_and_data_ptr& f) {
                    return (*f.ptr)(f.data, std::forward<CArgs>(args)...);
                },
                [&](const function_and_const_data_ptr& f) {
                    return (*f.ptr)(f.data, std::forward<CArgs>(args)...);
                }},
            data);
    }

    constexpr bool empty() const noexcept {
        return std::holds_alternative<std::monostate>(data);
    }
};
} // namespace snatch

// Implementation details.
// -----------------------

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

struct test_run;

using test_ptr = void (*)(test_run&);

template<typename T, typename F>
constexpr test_ptr to_test_case_ptr(const F&) noexcept {
    return [](test_run& t) { F{}.template operator()<T>(t); };
}

enum class test_state { not_run, success, skipped, failed };

struct test_case {
    test_id     id;
    test_ptr    func     = nullptr;
    test_state  state    = test_state::not_run;
    std::size_t asserts  = 0;
    float       duration = 0.0f;
};

struct section_nesting_level {
    std::size_t current_section_id  = 0;
    std::size_t previous_section_id = 0;
    std::size_t max_section_id      = 0;
};

struct section_state {
    small_vector<section_id, max_nested_sections>            current_section;
    small_vector<section_nesting_level, max_nested_sections> levels;
    std::size_t                                              depth         = 0;
    bool                                                     leaf_executed = false;
};

struct test_run {
    registry&     reg;
    test_case&    test;
    section_state sections;
};

struct section_entry_checker {
    section_id section;
    test_run&  state;
    bool       entered = false;

    ~section_entry_checker() noexcept;

    explicit operator bool() noexcept;
};

struct expression {
    std::string_view              content;
    small_string<max_expr_length> data;
    bool                          failed = false;

    template<typename T>
    void append(T&& value) noexcept {
        using TD = std::decay_t<T>;
        if constexpr (std::is_integral_v<TD>) {
            if constexpr (std::is_signed_v<TD>) {
                if (!snatch::append(data, static_cast<std::ptrdiff_t>(value))) {
                    failed = true;
                }
            } else {
                if (!snatch::append(data, static_cast<std::size_t>(value))) {
                    failed = true;
                }
            }
        } else if constexpr (std::is_enum_v<TD>) {
            append(static_cast<std::underlying_type_t<TD>>(value));
        } else if constexpr (
            std::is_pointer_v<TD> || std::is_floating_point_v<TD> || std::is_same_v<TD, bool> ||
            std::is_convertible_v<T, const void*> || std::is_convertible_v<T, std::string_view>) {
            if (!snatch::append(data, value)) {
                failed = true;
            }
        } else {
            append("?");
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

void stdout_print(std::string_view message) noexcept;

struct abort_exception {};
} // namespace snatch::impl

// Sections.
// ---------

namespace snatch {
using section_info = small_vector_span<const section_id>;
}

// Events.
// -------

namespace snatch {
struct assertion_location {
    std::string_view file;
    std::size_t      line = 0u;
};

namespace event {
struct test_run_started {
    std::string_view name;
};

struct test_run_ended {
    std::string_view name;
};

struct test_case_started {
    const test_id& id;
};

struct test_case_ended {
    const test_id& id;
    float          duration = 0.0f;
};

struct assertion_failed {
    const test_id&            id;
    section_info              sections;
    const assertion_location& location;
    std::string_view          message;
};

struct test_case_skipped {
    const test_id&            id;
    section_info              sections;
    const assertion_location& location;
    std::string_view          message;
};

using data = std::variant<
    test_run_started,
    test_run_ended,
    test_case_started,
    test_case_ended,
    assertion_failed,
    test_case_skipped>;
}; // namespace event
} // namespace snatch

// Command line interface.
// -----------------------

namespace snatch::cli {
struct argument {
    std::string_view                name;
    std::optional<std::string_view> value_name;
    std::optional<std::string_view> value;
};

struct input {
    std::string_view                              executable;
    small_vector<argument, max_command_line_args> arguments;
};

std::optional<input> parse_arguments(int argc, char* argv[]) noexcept;
} // namespace snatch::cli

// Test registry.
// --------------

namespace snatch {
class registry {
    small_vector<impl::test_case, max_test_cases> test_list;

    void print_location(
        const impl::test_case&     current_case,
        const impl::section_state& sections,
        const assertion_location&  location) const noexcept;

    void print_failure() const noexcept;
    void print_skip() const noexcept;
    void print_details(std::string_view message) const noexcept;
    void print_details_expr(const impl::expression& exp) const noexcept;

public:
    enum class verbosity { quiet, normal, high } verbose = verbosity::normal;
    bool with_color                                      = true;

    using print_function  = small_function<void(std::string_view) noexcept>;
    using report_function = small_function<void(const registry&, const event::data&) noexcept>;

    print_function  print_callback = &snatch::impl::stdout_print;
    report_function report_callback;

    template<typename... Args>
    void print(Args&&... args) const noexcept {
        small_string<max_message_length> message;
        append_or_truncate(message, std::forward<Args>(args)...);
        this->print_callback(message);
    }

    impl::proxy<std::tuple<>> add(std::string_view name, std::string_view tags) noexcept {
        return {this, name, tags};
    }

    template<typename T>
    impl::proxy<T> add_with_types(std::string_view name, std::string_view tags) noexcept {
        return {this, name, tags};
    }

    void register_test(const test_id& id, impl::test_ptr func) noexcept;

    template<typename... Args, typename F>
    void
    register_typed_tests(std::string_view name, std::string_view tags, const F& func) noexcept {
        (register_test(
             {name, tags, impl::get_type_name<Args>()}, impl::to_test_case_ptr<Args>(func)),
         ...);
    }

    void report_failure(
        impl::test_run&           state,
        const assertion_location& location,
        std::string_view          message) const noexcept;

    void report_failure(
        impl::test_run&           state,
        const assertion_location& location,
        std::string_view          message1,
        std::string_view          message2) const noexcept;

    void report_failure(
        impl::test_run&           state,
        const assertion_location& location,
        const impl::expression&   exp) const noexcept;

    void report_skipped(
        impl::test_run&           state,
        const assertion_location& location,
        std::string_view          message) const noexcept;

    void run(impl::test_case& test) noexcept;

    bool run_all_tests(std::string_view run_name) noexcept;
    bool run_tests_matching_name(std::string_view run_name, std::string_view name_filter) noexcept;
    bool run_tests_with_tag(std::string_view run_name, std::string_view tag_filter) noexcept;

    bool run_tests(const cli::input& args) noexcept;

    void configure(const cli::input& args) noexcept;

    void list_all_tests() const noexcept;
    void list_all_tags() const noexcept;
    void list_tests_with_tag(std::string_view tag) const noexcept;

    impl::test_case*       begin() noexcept;
    impl::test_case*       end() noexcept;
    const impl::test_case* begin() const noexcept;
    const impl::test_case* end() const noexcept;
};

extern constinit registry tests;
} // namespace snatch

// Implementation details.
// -----------------------

namespace snatch::impl {
template<typename... Args>
template<typename F>
const char* proxy<std::tuple<Args...>>::operator=(const F& func) noexcept {
    if constexpr (sizeof...(Args) > 0) {
        tests->template register_typed_tests<Args...>(name, tags, func);
    } else {
        tests->register_test({name, tags, {}}, func);
    }
    return name.data();
}
} // namespace snatch::impl

// Matchers.
// ---------

namespace snatch::matchers {
struct contains_substring {
    mutable small_string<max_message_length> description_buffer;
    std::string_view                         substring_pattern;

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
#    define SNATCH_WARNING_PUSH _Pragma("clang diagnostic push")
#    define SNATCH_WARNING_POP _Pragma("clang diagnostic pop")
#    define SNATCH_WARNING_DISABLE_PARENTHESES _Pragma("clang diagnostic ignored \"-Wparentheses\"")
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON do {} while (0)
#elif defined(__GNUC__)
#    define SNATCH_WARNING_PUSH _Pragma("GCC diagnostic push")
#    define SNATCH_WARNING_POP _Pragma("GCC diagnostic pop")
#    define SNATCH_WARNING_DISABLE_PARENTHESES _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON do {} while (0)
#elif defined(_MSC_VER)
#    define SNATCH_WARNING_PUSH _Pragma("warning(push)")
#    define SNATCH_WARNING_POP _Pragma("warning(pop)")
#    define SNATCH_WARNING_DISABLE_PARENTHESES do {} while (0)
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON _Pragma("warning(disable: 4127)")
#else
#    define SNATCH_WARNING_PUSH do {} while (0)
#    define SNATCH_WARNING_POP do {} while (0)
#    define SNATCH_WARNING_DISABLE_PARENTHESES do {} while (0)
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON do {} while (0)
#endif
// clang-format on

// Internal test macros.
// ---------------------

#if SNATCH_WITH_EXCEPTIONS
#    define SNATCH_TESTING_ABORT                                                                   \
        throw snatch::impl::abort_exception {}
#else
#    define SNATCH_TESTING_ABORT return
#endif

#define SNATCH_CONCAT_IMPL(x, y) x##y
#define SNATCH_MACRO_CONCAT(x, y) SNATCH_CONCAT_IMPL(x, y)
#define SNATCH_MACRO_DISPATCH2(_1, _2, NAME, ...) NAME

#define SNATCH_EXPR(type, x) snatch::impl::expression{type "(" #x ")", {}, false} <= x

// Public test macros.
// -------------------

#define SNATCH_TEST_CASE(NAME, TAGS)                                                               \
    static const char* SNATCH_MACRO_CONCAT(test_id_, __COUNTER__) =                                \
        snatch::tests.add(NAME, TAGS) =                                                            \
            [](snatch::impl::test_run & SNATCH_CURRENT_TEST [[maybe_unused]]) -> void

#define SNATCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)                                          \
    static const char* SNATCH_MACRO_CONCAT(test_id_, __COUNTER__) =                                \
        snatch::tests.add_with_types<TYPES>(NAME, TAGS) = []<typename TestType>(                   \
            snatch::impl::test_run & SNATCH_CURRENT_TEST [[maybe_unused]]) -> void

#define SNATCH_SECTION1(NAME)                                                                      \
    if (snatch::impl::section_entry_checker SNATCH_MACRO_CONCAT(section_id_, __COUNTER__){         \
            {(NAME), {}}, SNATCH_CURRENT_TEST})

#define SNATCH_SECTION2(NAME, DESCRIPTION)                                                         \
    if (snatch::impl::section_entry_checker SNATCH_MACRO_CONCAT(section_id_, __COUNTER__){         \
            {(NAME), (DESCRIPTION)}, SNATCH_CURRENT_TEST})

#define SNATCH_SECTION(...)                                                                        \
    SNATCH_MACRO_DISPATCH2(__VA_ARGS__, SNATCH_SECTION2, SNATCH_SECTION1)(__VA_ARGS__)

#define SNATCH_REQUIRE(EXP)                                                                        \
    do {                                                                                           \
        ++SNATCH_CURRENT_CASE.asserts;                                                             \
        SNATCH_WARNING_PUSH                                                                        \
        SNATCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if (!(EXP)) {                                                                              \
            snatch::tests.report_failure(                                                          \
                SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, SNATCH_EXPR("REQUIRE", EXP));           \
            SNATCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
        SNATCH_WARNING_POP                                                                         \
    } while (0)

#define SNATCH_CHECK(EXP)                                                                          \
    do {                                                                                           \
        ++SNATCH_CURRENT_CASE.asserts;                                                             \
        SNATCH_WARNING_PUSH                                                                        \
        SNATCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if (!(EXP)) {                                                                              \
            snatch::tests.report_failure(                                                          \
                SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, SNATCH_EXPR("CHECK", EXP));             \
        }                                                                                          \
        SNATCH_WARNING_POP                                                                         \
    } while (0)

#define SNATCH_FAIL(MESSAGE)                                                                       \
    do {                                                                                           \
        snatch::tests.report_failure(SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));        \
        SNATCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNATCH_FAIL_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        snatch::tests.report_failure(SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));        \
    } while (0)

#define SNATCH_SKIP(MESSAGE)                                                                       \
    do {                                                                                           \
        snatch::tests.report_skipped(SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));        \
        SNATCH_TESTING_ABORT;                                                                      \
    } while (0)

// clang-format off
#if SNATCH_WITH_SHORTHAND_MACROS
#    define TEST_CASE(NAME, TAGS)                      SNATCH_TEST_CASE(NAME, TAGS)
#    define TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) SNATCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)
#    define SECTION(...)                               SNATCH_SECTION(__VA_ARGS__)
#    define REQUIRE(EXP)                               SNATCH_REQUIRE(EXP)
#    define CHECK(EXP)                                 SNATCH_CHECK(EXP)
#    define FAIL(MESSAGE)                              SNATCH_FAIL(MESSAGE)
#    define FAIL_CHECK(MESSAGE)                        SNATCH_FAIL_CHECK(MESSAGE)
#    define SKIP(MESSAGE)                              SNATCH_SKIP(MESSAGE)
#endif
// clang-format on

#if SNATCH_WITH_EXCEPTIONS

#    define SNATCH_REQUIRE_THROWS_AS(EXPRESSION, EXCEPTION)                                        \
        do {                                                                                       \
            try {                                                                                  \
                EXPRESSION;                                                                        \
                SNATCH_FAIL(#EXCEPTION " expected but no exception thrown");                       \
            } catch (const EXCEPTION&) {                                                           \
                /* success */                                                                      \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
                SNATCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } while (0)

#    define SNATCH_CHECK_THROWS_AS(EXPRESSION, EXCEPTION)                                          \
        do {                                                                                       \
            try {                                                                                  \
                EXPRESSION;                                                                        \
                SNATCH_FAIL_CHECK(#EXCEPTION " expected but no exception thrown");                 \
            } catch (const EXCEPTION&) {                                                           \
                /* success */                                                                      \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
            }                                                                                      \
        } while (0)

#    define SNATCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                          \
        do {                                                                                       \
            try {                                                                                  \
                EXPRESSION;                                                                        \
                SNATCH_FAIL(#EXCEPTION " expected but no exception thrown");                       \
            } catch (const EXCEPTION& e) {                                                         \
                if (!(MATCHER).match(e)) {                                                         \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        (MATCHER).describe_fail(e));                                               \
                    SNATCH_TESTING_ABORT;                                                          \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
                SNATCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } while (0)

#    define SNATCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                            \
        do {                                                                                       \
            try {                                                                                  \
                EXPRESSION;                                                                        \
                SNATCH_FAIL_CHECK(#EXCEPTION " expected but no exception thrown");                 \
            } catch (const EXCEPTION& e) {                                                         \
                if (!(MATCHER).match(e)) {                                                         \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        (MATCHER).describe_fail(e));                                               \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    snatch::tests.report_failure(                                                  \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
            }                                                                                      \
        } while (0)

// clang-format off
#if SNATCH_WITH_SHORTHAND_MACROS
#    define REQUIRE_THROWS_AS(EXPRESSION, EXCEPTION)               SNATCH_REQUIRE_THROWS_AS(EXPRESSION, EXCEPTION)
#    define CHECK_THROWS_AS(EXPRESSION, EXCEPTION)                 SNATCH_CHECK_THROWS_AS(EXPRESSION, EXCEPTION)
#    define REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER) SNATCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)
#    define CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)   SNATCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)
#endif
// clang-format on

#endif

#endif
