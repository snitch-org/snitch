#ifndef SNATCH_HPP
#define SNATCH_HPP

#include "snatch/snatch_config.hpp"

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
// Maximum number of captured expressions in a test case.
constexpr std::size_t max_captures = SNATCH_MAX_CAPTURES;
// Maximum length of a captured expression.
constexpr std::size_t max_capture_length = SNATCH_MAX_CAPTURE_LENGTH;
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

namespace snatch::matchers {
enum class match_status { failed, matched };
} // namespace snatch::matchers

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
    constexpr small_vector& operator=(small_vector&& other) noexcept      = default;
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
    constexpr small_string&    operator=(small_string&& other) noexcept      = default;
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
    constexpr char& operator[](std::size_t i) noexcept {
        return span()[i];
    }
    constexpr char operator[](std::size_t i) const noexcept {
        return const_cast<small_string*>(this)->span()[i];
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
    } else if constexpr (std::is_function_v<T>) {
        if (ptr != nullptr) {
            return append(ss, std::string_view("0x????????"));
        } else {
            return append(ss, std::string_view("nullptr"));
        }
    } else {
        return append(ss, static_cast<const void*>(ptr));
    }
}
template<std::size_t N>
[[nodiscard]] bool append(small_string_span ss, const char str[N]) noexcept {
    return append(ss, std::string_view(str));
}

template<typename T>
concept signed_integral = std::is_signed_v<T>;

template<typename T>
concept unsigned_integral = std::is_unsigned_v<T>;

template<typename T, typename U>
concept convertible_to = std::is_convertible_v<T, U>;

template<typename T>
concept enumeration = std::is_enum_v<T>;

template<signed_integral T>
[[nodiscard]] bool append(small_string_span ss, T value) noexcept {
    return snatch::append(ss, static_cast<std::ptrdiff_t>(value));
}

template<unsigned_integral T>
[[nodiscard]] bool append(small_string_span ss, T value) noexcept {
    return snatch::append(ss, static_cast<std::size_t>(value));
}

template<enumeration T>
[[nodiscard]] bool append(small_string_span ss, T value) noexcept {
    return append(ss, static_cast<std::underlying_type_t<T>>(value));
}

template<convertible_to<std::string_view> T>
[[nodiscard]] bool append(small_string_span ss, const T& value) noexcept {
    return snatch::append(ss, std::string_view(value));
}

template<typename T>
concept string_appendable = requires(small_string_span ss, T value) { append(ss, value); };

template<string_appendable T, string_appendable U, string_appendable... Args>
[[nodiscard]] bool append(small_string_span ss, T&& t, U&& u, Args&&... args) noexcept {
    return append(ss, std::forward<T>(t)) && append(ss, std::forward<U>(u)) &&
           (append(ss, std::forward<Args>(args)) && ...);
}

void truncate_end(small_string_span ss) noexcept;

template<string_appendable... Args>
bool append_or_truncate(small_string_span ss, Args&&... args) noexcept {
    if (!append(ss, std::forward<Args>(args)...)) {
        truncate_end(ss);
        return false;
    }

    return true;
}

[[nodiscard]] bool replace_all(
    small_string_span string, std::string_view pattern, std::string_view replacement) noexcept;

template<typename T, typename U>
concept matcher_for = requires(const T& m, const U& value) {
                          { m.match(value) } -> convertible_to<bool>;
                          {
                              m.describe_match(value, matchers::match_status{})
                              } -> convertible_to<std::string_view>;
                      };
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

    constexpr small_function(function_ptr ptr) noexcept : data{ptr} {}

    template<convertible_to<function_ptr> T>
    constexpr small_function(T&& obj) noexcept : data{static_cast<function_ptr>(obj)} {}

    template<typename T, auto M>
    constexpr small_function(T& obj, constant<M>) noexcept :
        data{function_and_data_ptr{
            &obj, [](void* ptr, Args... args) noexcept {
                if constexpr (std::is_same_v<Ret, void>) {
                    (static_cast<T*>(ptr)->*constant<M>::value)(std::move(args)...);
                } else {
                    return (static_cast<T*>(ptr)->*constant<M>::value)(std::move(args)...);
                }
            }}} {}

    template<typename T, auto M>
    constexpr small_function(const T& obj, constant<M>) noexcept :
        data{function_and_const_data_ptr{
            &obj, [](const void* ptr, Args... args) noexcept {
                if constexpr (std::is_same_v<Ret, void>) {
                    (static_cast<const T*>(ptr)->*constant<M>::value)(std::move(args)...);
                } else {
                    return (static_cast<const T*>(ptr)->*constant<M>::value)(std::move(args)...);
                }
            }}} {}

    template<typename T>
    constexpr small_function(T& obj) noexcept : small_function(obj, constant<&T::operator()>{}) {}

    template<typename T>
    constexpr small_function(const T& obj) noexcept :
        small_function(obj, constant<&T::operator()>{}) {}

    // Prevent inadvertently using temporary stateful lambda; not supported at the moment.
    template<typename T>
    constexpr small_function(T&& obj) noexcept = delete;

    // Prevent inadvertently using temporary object; not supported at the moment.
    template<typename T, auto M>
    constexpr small_function(T&& obj, constant<M>) noexcept = delete;

    template<typename... CArgs>
    constexpr Ret operator()(CArgs&&... args) const noexcept {
        if constexpr (std::is_same_v<Ret, void>) {
            std::visit(
                overload{
                    [](std::monostate) {
                        terminate_with("small_function called without an implementation");
                    },
                    [&](function_ptr f) { (*f)(std::forward<CArgs>(args)...); },
                    [&](const function_and_data_ptr& f) {
                        (*f.ptr)(f.data, std::forward<CArgs>(args)...);
                    },
                    [&](const function_and_const_data_ptr& f) {
                        (*f.ptr)(f.data, std::forward<CArgs>(args)...);
                    }},
                data);
        } else {
            return std::visit(
                overload{
                    [](std::monostate) -> Ret {
                        terminate_with("small_function called without an implementation");
                    },
                    [&](function_ptr f) { return (*f)(std::forward<CArgs>(args)...); },
                    [&](const function_and_data_ptr& f) {
                        return (*f.ptr)(f.data, std::forward<CArgs>(args)...);
                    },
                    [&](const function_and_const_data_ptr& f) {
                        return (*f.ptr)(f.data, std::forward<CArgs>(args)...);
                    }},
                data);
        }
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
    test_id    id;
    test_ptr   func  = nullptr;
    test_state state = test_state::not_run;
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

using capture_state = small_vector<small_string<max_capture_length>, max_captures>;

struct test_run {
    registry&     reg;
    test_case&    test;
    section_state sections;
    capture_state captures;
    std::size_t   asserts = 0;
#if SNATCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct section_entry_checker {
    section_id section;
    test_run&  state;
    bool       entered = false;

    ~section_entry_checker() noexcept;

    explicit operator bool() noexcept;
};

struct operator_less {
    static constexpr std::string_view inverse = " >= ";
    template<typename T, typename U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return lhs < rhs;
    }
};

struct operator_greater {
    static constexpr std::string_view inverse = " <= ";
    template<typename T, typename U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return lhs > rhs;
    }
};

struct operator_less_equal {
    static constexpr std::string_view inverse = " > ";
    template<typename T, typename U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return lhs <= rhs;
    }
};

struct operator_greater_equal {
    static constexpr std::string_view inverse = " < ";
    template<typename T, typename U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return lhs >= rhs;
    }
};

struct operator_equal {
    static constexpr std::string_view inverse = " != ";
    template<typename T, typename U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return lhs == rhs;
    }
};

struct operator_not_equal {
    static constexpr std::string_view inverse = " == ";
    template<typename T, typename U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return lhs != rhs;
    }
};

struct expression {
    std::string_view              expected;
    small_string<max_expr_length> actual;

    template<string_appendable T>
    [[nodiscard]] bool append_value(T&& value) noexcept {
        return append(actual, std::forward<T>(value));
    }

    template<typename T>
    [[nodiscard]] bool append_value(T&&) noexcept {
        return append(actual, "?");
    }
};

template<typename T, typename O, typename U>
struct extracted_binary_expression {
    expression& expr;
    const T&    lhs;
    const U&    rhs;

#define EXPR_OPERATOR(OP)                                                                          \
    template<typename V>                                                                           \
    bool operator OP(const V&) noexcept {                                                          \
        static_assert(                                                                             \
            !std::is_same_v<V, V>,                                                                 \
            "cannot chain expression in this way; please decompose it into multiple checks");      \
        return false;                                                                              \
    }

    EXPR_OPERATOR(<=)
    EXPR_OPERATOR(<)
    EXPR_OPERATOR(>=)
    EXPR_OPERATOR(>)
    EXPR_OPERATOR(==)
    EXPR_OPERATOR(!=)
    EXPR_OPERATOR(&&)
    EXPR_OPERATOR(||)

#undef EXPR_OPERATOR

    explicit operator bool() const noexcept {
        if (!O{}(lhs, rhs)) {
            if constexpr (matcher_for<T, U>) {
                using namespace snatch::matchers;
                constexpr auto status = std::is_same_v<O, operator_equal> ? match_status::failed
                                                                          : match_status::matched;
                if (!expr.append_value(lhs.describe_match(rhs, status))) {
                    expr.actual.clear();
                }
            } else if constexpr (matcher_for<U, T>) {
                using namespace snatch::matchers;
                constexpr auto status = std::is_same_v<O, operator_equal> ? match_status::failed
                                                                          : match_status::matched;
                if (!expr.append_value(rhs.describe_match(lhs, status))) {
                    expr.actual.clear();
                }
            } else {
                if (!expr.append_value(lhs) || !expr.append_value(O::inverse) ||
                    !expr.append_value(rhs)) {
                    expr.actual.clear();
                }
            }

            return true;
        }

        return false;
    }
};

template<typename T>
struct extracted_unary_expression {
    expression& expr;
    const T&    lhs;

#define EXPR_OPERATOR(OP, OP_TYPE)                                                                 \
    template<typename U>                                                                           \
    constexpr extracted_binary_expression<T, OP_TYPE, U> operator OP(const U& rhs)                 \
        const noexcept {                                                                           \
        return {expr, lhs, rhs};                                                                   \
    }

    EXPR_OPERATOR(<, operator_less)
    EXPR_OPERATOR(>, operator_greater)
    EXPR_OPERATOR(<=, operator_less_equal)
    EXPR_OPERATOR(>=, operator_greater_equal)
    EXPR_OPERATOR(==, operator_equal)
    EXPR_OPERATOR(!=, operator_not_equal)

#undef EXPR_OPERATOR

    explicit operator bool() const noexcept {
        if (!static_cast<bool>(lhs)) {
            if (!expr.append_value(lhs)) {
                expr.actual.clear();
            }

            return true;
        }

        return false;
    }
};

struct expression_extractor {
    expression& expr;

    template<typename T>
    constexpr extracted_unary_expression<T> operator<=(const T& lhs) const noexcept {
        return {expr, lhs};
    }
};

struct scoped_capture {
    capture_state& captures;
    std::size_t    count = 0;

    ~scoped_capture() noexcept {
        captures.resize(captures.size() - count);
    }
};

std::string_view extract_next_name(std::string_view& names) noexcept;

small_string<max_capture_length>& add_capture(test_run& state) noexcept;

template<string_appendable T>
void add_capture(test_run& state, std::string_view& names, const T& arg) noexcept {
    auto& capture = add_capture(state);
    append_or_truncate(capture, extract_next_name(names), " := ", arg);
}

template<string_appendable... Args>
scoped_capture add_captures(test_run& state, std::string_view names, const Args&... args) noexcept {
    (add_capture(state, names, args), ...);
    return {state.captures, sizeof...(args)};
}

template<string_appendable... Args>
scoped_capture add_info(test_run& state, const Args&... args) noexcept {
    auto& capture = add_capture(state);
    append_or_truncate(capture, args...);
    return {state.captures, 1};
}

void stdout_print(std::string_view message) noexcept;

struct abort_exception {};

template<typename T>
concept exception_with_what = requires(const T& e) {
                                  { e.what() } -> convertible_to<std::string_view>;
                              };
} // namespace snatch::impl

// Sections and captures.
// ---------

namespace snatch {
using section_info = small_vector_span<const section_id>;
using capture_info = small_vector_span<const std::string_view>;
} // namespace snatch

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
    bool             success         = true;
    std::size_t      run_count       = 0;
    std::size_t      fail_count      = 0;
    std::size_t      skip_count      = 0;
    std::size_t      assertion_count = 0;
};

struct test_case_started {
    const test_id& id;
};

struct test_case_ended {
    const test_id& id;
#if SNATCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct assertion_failed {
    const test_id&            id;
    section_info              sections;
    capture_info              captures;
    const assertion_location& location;
    std::string_view          message;
};

struct test_case_skipped {
    const test_id&            id;
    section_info              sections;
    capture_info              captures;
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

std::optional<input> parse_arguments(int argc, const char* const argv[]) noexcept;

std::optional<cli::argument> get_option(const cli::input& args, std::string_view name) noexcept;

std::optional<cli::argument>
get_positional_argument(const cli::input& args, std::string_view name) noexcept;
} // namespace snatch::cli

// Test registry.
// --------------

namespace snatch {
class registry {
    small_vector<impl::test_case, max_test_cases> test_list;

    void print_location(
        const impl::test_case&     current_case,
        const impl::section_state& sections,
        const impl::capture_state& captures,
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

    impl::test_run run(impl::test_case& test) noexcept;

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
    std::string_view substring_pattern;

    explicit contains_substring(std::string_view pattern) noexcept;

    bool match(std::string_view message) const noexcept;

    small_string<max_message_length>
    describe_match(std::string_view message, match_status status) const noexcept;
};

template<typename T, std::size_t N>
struct is_any_of {
    small_vector<T, N> list;

    template<typename... Args>
    explicit is_any_of(const Args&... args) noexcept : list({args...}) {}

    bool match(const T& value) const noexcept {
        for (const auto& v : list) {
            if (v == value) {
                return true;
            }
        }

        return false;
    }

    small_string<max_message_length>
    describe_match(const T& value, match_status status) const noexcept {
        small_string<max_message_length> description_buffer;
        append_or_truncate(
            description_buffer, "'", value, "' was ",
            (status == match_status::failed ? "not " : ""), "found in {");

        bool first = true;
        for (const auto& v : list) {
            if (!first) {
                append_or_truncate(description_buffer, ", '", v, "'");
            } else {
                append_or_truncate(description_buffer, "'", v, "'");
            }
            first = false;
        }
        append_or_truncate(description_buffer, "}");

        return description_buffer;
    }
};

template<typename T, typename... Args>
is_any_of(T, Args...) -> is_any_of<T, sizeof...(Args) + 1>;

struct with_what_contains : private contains_substring {
    explicit with_what_contains(std::string_view pattern) noexcept;

    template<snatch::impl::exception_with_what E>
    bool match(const E& e) const noexcept {
        return contains_substring::match(e.what());
    }

    template<snatch::impl::exception_with_what E>
    small_string<max_message_length>
    describe_match(const E& e, match_status status) const noexcept {
        return contains_substring::describe_match(e.what(), status);
    }
};

template<typename T, matcher_for<T> M>
bool operator==(const T& value, const M& m) noexcept {
    return m.match(value);
}

template<typename T, matcher_for<T> M>
bool operator==(const M& m, const T& value) noexcept {
    return m.match(value);
}
} // namespace snatch::matchers

// Compiler warning handling.
// --------------------------

// clang-format off
#if defined(__clang__)
#    define SNATCH_WARNING_PUSH _Pragma("clang diagnostic push")
#    define SNATCH_WARNING_POP _Pragma("clang diagnostic pop")
#    define SNATCH_WARNING_DISABLE_PARENTHESES _Pragma("clang diagnostic ignored \"-Wparentheses\"")
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON
#elif defined(__GNUC__)
#    define SNATCH_WARNING_PUSH _Pragma("GCC diagnostic push")
#    define SNATCH_WARNING_POP _Pragma("GCC diagnostic pop")
#    define SNATCH_WARNING_DISABLE_PARENTHESES _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON
#elif defined(_MSC_VER)
#    define SNATCH_WARNING_PUSH _Pragma("warning(push)")
#    define SNATCH_WARNING_POP _Pragma("warning(pop)")
#    define SNATCH_WARNING_DISABLE_PARENTHESES
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON _Pragma("warning(disable: 4127)")
#else
#    define SNATCH_WARNING_PUSH
#    define SNATCH_WARNING_POP
#    define SNATCH_WARNING_DISABLE_PARENTHESES
#    define SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON
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

#define SNATCH_EXPR(TYPE, EXP)                                                                     \
    auto SNATCH_CURRENT_EXPRESSION = snatch::impl::expression{TYPE "(" #EXP ")", {}};              \
    snatch::impl::expression_extractor{SNATCH_CURRENT_EXPRESSION} <= EXP

// Public test macros.
// -------------------

#define SNATCH_TEST_CASE(NAME, TAGS)                                                               \
    static const char* SNATCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snatch::tests.add(NAME, TAGS) =                                                            \
            [](snatch::impl::test_run & SNATCH_CURRENT_TEST [[maybe_unused]]) -> void

#define SNATCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)                                          \
    static const char* SNATCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snatch::tests.add_with_types<TYPES>(NAME, TAGS) = []<typename TestType>(                   \
            snatch::impl::test_run & SNATCH_CURRENT_TEST [[maybe_unused]]) -> void

#define SNATCH_TEMPLATE_TEST_CASE(NAME, TAGS, ...)                                                 \
    static const char* SNATCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snatch::tests.add_with_types<std::tuple<__VA_ARGS__>>(NAME, TAGS) = []<typename TestType>( \
            snatch::impl::test_run & SNATCH_CURRENT_TEST [[maybe_unused]]) -> void

#define SNATCH_SECTION1(NAME)                                                                      \
    if (snatch::impl::section_entry_checker SNATCH_MACRO_CONCAT(section_id_, __COUNTER__){         \
            {(NAME), {}}, SNATCH_CURRENT_TEST})

#define SNATCH_SECTION2(NAME, DESCRIPTION)                                                         \
    if (snatch::impl::section_entry_checker SNATCH_MACRO_CONCAT(section_id_, __COUNTER__){         \
            {(NAME), (DESCRIPTION)}, SNATCH_CURRENT_TEST})

#define SNATCH_SECTION(...)                                                                        \
    SNATCH_MACRO_DISPATCH2(__VA_ARGS__, SNATCH_SECTION2, SNATCH_SECTION1)(__VA_ARGS__)

#define SNATCH_CAPTURE(...)                                                                        \
    auto SNATCH_MACRO_CONCAT(capture_id_, __COUNTER__) =                                           \
        snatch::impl::add_captures(SNATCH_CURRENT_TEST, #__VA_ARGS__, __VA_ARGS__)

#define SNATCH_INFO(...)                                                                           \
    auto SNATCH_MACRO_CONCAT(capture_id_, __COUNTER__) =                                           \
        snatch::impl::add_info(SNATCH_CURRENT_TEST, __VA_ARGS__)

#define SNATCH_REQUIRE(EXP)                                                                        \
    do {                                                                                           \
        ++SNATCH_CURRENT_TEST.asserts;                                                             \
        SNATCH_WARNING_PUSH                                                                        \
        SNATCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if (SNATCH_EXPR("REQUIRE", EXP)) {                                                         \
            SNATCH_CURRENT_TEST.reg.report_failure(                                                \
                SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, SNATCH_CURRENT_EXPRESSION);             \
            SNATCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
        SNATCH_WARNING_POP                                                                         \
    } while (0)

#define SNATCH_CHECK(EXP)                                                                          \
    do {                                                                                           \
        ++SNATCH_CURRENT_TEST.asserts;                                                             \
        SNATCH_WARNING_PUSH                                                                        \
        SNATCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNATCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if (SNATCH_EXPR("CHECK", EXP)) {                                                           \
            SNATCH_CURRENT_TEST.reg.report_failure(                                                \
                SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, SNATCH_CURRENT_EXPRESSION);             \
        }                                                                                          \
        SNATCH_WARNING_POP                                                                         \
    } while (0)

#define SNATCH_FAIL(MESSAGE)                                                                       \
    do {                                                                                           \
        ++SNATCH_CURRENT_TEST.asserts;                                                             \
        SNATCH_CURRENT_TEST.reg.report_failure(                                                    \
            SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
        SNATCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNATCH_FAIL_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        ++SNATCH_CURRENT_TEST.asserts;                                                             \
        SNATCH_CURRENT_TEST.reg.report_failure(                                                    \
            SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
    } while (0)

#define SNATCH_SKIP(MESSAGE)                                                                       \
    do {                                                                                           \
        SNATCH_CURRENT_TEST.reg.report_skipped(                                                    \
            SNATCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
        SNATCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNATCH_REQUIRE_THAT(EXPR, MATCHER)                                                         \
    do {                                                                                           \
        ++SNATCH_CURRENT_TEST.asserts;                                                             \
        const auto& SNATCH_TEMP_VALUE   = (EXPR);                                                  \
        const auto& SNATCH_TEMP_MATCHER = (MATCHER);                                               \
        if (!SNATCH_TEMP_MATCHER.match(SNATCH_TEMP_VALUE)) {                                       \
            SNATCH_CURRENT_TEST.reg.report_failure(                                                \
                SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                SNATCH_TEMP_MATCHER.describe_fail(SNATCH_TEMP_VALUE));                             \
            SNATCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
    } while (0)

#define SNATCH_CHECK_THAT(EXPR, MATCHER)                                                           \
    do {                                                                                           \
        ++SNATCH_CURRENT_TEST.asserts;                                                             \
        const auto& SNATCH_TEMP_VALUE   = (EXPR);                                                  \
        const auto& SNATCH_TEMP_MATCHER = (MATCHER);                                               \
        if (!SNATCH_TEMP_MATCHER.match(SNATCH_TEMP_VALUE)) {                                       \
            SNATCH_CURRENT_TEST.reg.report_failure(                                                \
                SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                SNATCH_TEMP_MATCHER.describe_fail(SNATCH_TEMP_VALUE));                             \
        }                                                                                          \
    } while (0)

// clang-format off
#if SNATCH_WITH_SHORTHAND_MACROS
#    define TEST_CASE(NAME, TAGS)                      SNATCH_TEST_CASE(NAME, TAGS)
#    define TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) SNATCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)
#    define TEMPLATE_TEST_CASE(NAME, TAGS, ...)        SNATCH_TEMPLATE_TEST_CASE(NAME, TAGS, __VA_ARGS__)
#    define SECTION(...)                               SNATCH_SECTION(__VA_ARGS__)
#    define CAPTURE(...)                               SNATCH_CAPTURE(__VA_ARGS__)
#    define INFO(...)                                  SNATCH_INFO(__VA_ARGS__)
#    define REQUIRE(EXP)                               SNATCH_REQUIRE(EXP)
#    define CHECK(EXP)                                 SNATCH_CHECK(EXP)
#    define FAIL(MESSAGE)                              SNATCH_FAIL(MESSAGE)
#    define FAIL_CHECK(MESSAGE)                        SNATCH_FAIL_CHECK(MESSAGE)
#    define SKIP(MESSAGE)                              SNATCH_SKIP(MESSAGE)
#    define REQUIRE_THAT(EXP, MATCHER)                 SNATCH_REQUIRE(EXP, MATCHER)
#    define CHECK_THAT(EXP, MATCHER)                   SNATCH_CHECK(EXP, MATCHER)
#endif
// clang-format on

#if SNATCH_WITH_EXCEPTIONS

#    define SNATCH_REQUIRE_THROWS_AS(EXPRESSION, EXCEPTION)                                        \
        do {                                                                                       \
            try {                                                                                  \
                ++SNATCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNATCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #EXCEPTION " expected but no exception thrown");                               \
                SNATCH_TESTING_ABORT;                                                              \
            } catch (const EXCEPTION&) {                                                           \
                /* success */                                                                      \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
                SNATCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } while (0)

#    define SNATCH_CHECK_THROWS_AS(EXPRESSION, EXCEPTION)                                          \
        do {                                                                                       \
            try {                                                                                  \
                ++SNATCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNATCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #EXCEPTION " expected but no exception thrown");                               \
            } catch (const EXCEPTION&) {                                                           \
                /* success */                                                                      \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
            }                                                                                      \
        } while (0)

#    define SNATCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                          \
        do {                                                                                       \
            try {                                                                                  \
                ++SNATCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNATCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #EXCEPTION " expected but no exception thrown");                               \
                SNATCH_TESTING_ABORT;                                                              \
            } catch (const EXCEPTION& e) {                                                         \
                if (!(MATCHER).match(e)) {                                                         \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        (MATCHER).describe_match(e, snatch::matchers::match_status::failed));      \
                    SNATCH_TESTING_ABORT;                                                          \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
                SNATCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } while (0)

#    define SNATCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, MATCHER)                            \
        do {                                                                                       \
            try {                                                                                  \
                ++SNATCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNATCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #EXCEPTION " expected but no exception thrown");                               \
            } catch (const EXCEPTION& e) {                                                         \
                if (!(MATCHER).match(e)) {                                                         \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        (MATCHER).describe_match(e, snatch::matchers::match_status::failed));      \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNATCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNATCH_CURRENT_TEST.reg.report_failure(                                        \
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
