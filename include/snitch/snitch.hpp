#ifndef SNITCH_HPP
#define SNITCH_HPP

#include "snitch/snitch_config.hpp"

#include <array> // for small_vector
#include <cstddef> // for std::size_t
#if SNITCH_WITH_EXCEPTIONS
#    include <exception> // for std::exception
#endif
#include <compare> // for std::partial_ordering
#include <initializer_list> // for std::initializer_list
#include <optional> // for cli
#include <string_view> // for all strings
#include <type_traits> // for std::is_nothrow_*
#include <utility> // for std::forward, std::move
#include <variant> // for events and small_function

// Testing framework configuration.
// --------------------------------

namespace snitch {
// Maximum number of test cases in the whole program.
// A "test case" is created for each uses of the `*_TEST_CASE` macros,
// and for each type for the `TEMPLATE_LIST_TEST_CASE` macro.
constexpr std::size_t max_test_cases = SNITCH_MAX_TEST_CASES;
// Maximum depth of nested sections in a test case (section in section in section ...).
constexpr std::size_t max_nested_sections = SNITCH_MAX_NESTED_SECTIONS;
// Maximum length of a `CHECK(...)` or `REQUIRE(...)` expression,
// beyond which automatic variable printing is disabled.
constexpr std::size_t max_expr_length = SNITCH_MAX_EXPR_LENGTH;
// Maximum length of error messages.
constexpr std::size_t max_message_length = SNITCH_MAX_MESSAGE_LENGTH;
// Maximum length of a full test case name.
// The full test case name includes the base name, plus any type.
constexpr std::size_t max_test_name_length = SNITCH_MAX_TEST_NAME_LENGTH;
// Maximum length of a tag, including brackets.
constexpr std::size_t max_tag_length = SNITCH_MAX_TAG_LENGTH;
// Maximum number of captured expressions in a test case.
constexpr std::size_t max_captures = SNITCH_MAX_CAPTURES;
// Maximum length of a captured expression.
constexpr std::size_t max_capture_length = SNITCH_MAX_CAPTURE_LENGTH;
// Maximum number of unique tags in the whole program.
constexpr std::size_t max_unique_tags = SNITCH_MAX_UNIQUE_TAGS;
// Maximum number of command line arguments.
constexpr std::size_t max_command_line_args = SNITCH_MAX_COMMAND_LINE_ARGS;
} // namespace snitch

// Forward declarations and public utilities.
// ------------------------------------------

namespace snitch {
class registry;

struct test_id {
    std::string_view name = {};
    std::string_view tags = {};
    std::string_view type = {};
};

struct section_id {
    std::string_view name        = {};
    std::string_view description = {};
};
} // namespace snitch

namespace snitch::matchers {
enum class match_status { failed, matched };
} // namespace snitch::matchers

// Implementation details.
// -----------------------

namespace snitch::impl {
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
} // namespace snitch::impl

// Public utilities.
// ------------------------------------------------

namespace snitch {
template<typename T>
constexpr std::string_view type_name = impl::get_type_name<T>();

template<typename... Args>
struct type_list {};

[[noreturn]] void terminate_with(std::string_view msg) noexcept;
} // namespace snitch

// Public utilities: small_vector.
// -------------------------------

namespace snitch {
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
    constexpr small_vector_span() noexcept = default;

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
    std::array<ElemType, MaxLength> data_buffer = {};
    std::size_t                     data_size   = 0;

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
} // namespace snitch

// Public utilities: small_string.
// -------------------------------

namespace snitch {
using small_string_span = small_vector_span<char>;
using small_string_view = small_vector_span<const char>;

template<std::size_t MaxLength>
class small_string {
    std::array<char, MaxLength> data_buffer = {};
    std::size_t                 data_size   = 0u;

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
    return snitch::append(ss, static_cast<std::ptrdiff_t>(value));
}

template<unsigned_integral T>
[[nodiscard]] bool append(small_string_span ss, T value) noexcept {
    return snitch::append(ss, static_cast<std::size_t>(value));
}

template<enumeration T>
[[nodiscard]] bool append(small_string_span ss, T value) noexcept {
    return append(ss, static_cast<std::underlying_type_t<T>>(value));
}

template<convertible_to<std::string_view> T>
[[nodiscard]] bool append(small_string_span ss, const T& value) noexcept {
    return snitch::append(ss, std::string_view(value));
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

[[nodiscard]] bool is_match(std::string_view string, std::string_view regex) noexcept;

enum class filter_result { included, excluded, not_included, not_excluded };

[[nodiscard]] filter_result
is_filter_match_name(std::string_view name, std::string_view filter) noexcept;

[[nodiscard]] filter_result
is_filter_match_tags(std::string_view tags, std::string_view filter) noexcept;

[[nodiscard]] filter_result is_filter_match_id(const test_id& id, std::string_view filter) noexcept;

template<typename T, typename U>
concept matcher_for = requires(const T& m, const U& value) {
                          { m.match(value) } -> convertible_to<bool>;
                          {
                              m.describe_match(value, matchers::match_status{})
                              } -> convertible_to<std::string_view>;
                      };
} // namespace snitch

// Public utilities: small_function.
// ---------------------------------

namespace snitch {
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
} // namespace snitch

// Implementation details.
// -----------------------

namespace snitch::impl {
struct test_state;

using test_ptr = void (*)();

template<typename T, typename F>
constexpr test_ptr to_test_case_ptr(const F&) noexcept {
    return []() { F{}.template operator()<T>(); };
}

enum class test_case_state { not_run, success, skipped, failed };

struct test_case {
    test_id         id    = {};
    test_ptr        func  = nullptr;
    test_case_state state = test_case_state::not_run;
};

struct section_nesting_level {
    std::size_t current_section_id  = 0;
    std::size_t previous_section_id = 0;
    std::size_t max_section_id      = 0;
};

struct section_state {
    small_vector<section_id, max_nested_sections>            current_section = {};
    small_vector<section_nesting_level, max_nested_sections> levels          = {};
    std::size_t                                              depth           = 0;
    bool                                                     leaf_executed   = false;
};

using capture_state = small_vector<small_string<max_capture_length>, max_captures>;

struct test_state {
    registry&     reg;
    test_case&    test;
    section_state sections    = {};
    capture_state captures    = {};
    std::size_t   asserts     = 0;
    bool          may_fail    = false;
    bool          should_fail = false;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

test_state& get_current_test() noexcept;
test_state* try_get_current_test() noexcept;
void        set_current_test(test_state* current) noexcept;

struct section_entry_checker {
    section_id  section = {};
    test_state& state;
    bool        entered = false;

    ~section_entry_checker() noexcept;

    explicit operator bool() noexcept;
};

#define DEFINE_OPERATOR(OP, NAME, DISP, DISP_INV)                                                  \
    struct operator_##NAME {                                                                       \
        static constexpr std::string_view actual  = DISP;                                          \
        static constexpr std::string_view inverse = DISP_INV;                                      \
                                                                                                   \
        template<typename T, typename U>                                                           \
        constexpr bool operator()(const T& lhs, const U& rhs) const noexcept                       \
            requires requires(const T& lhs, const U& rhs) { lhs OP rhs; }                          \
        {                                                                                          \
            return lhs OP rhs;                                                                     \
        }                                                                                          \
    }

DEFINE_OPERATOR(<, less, " < ", " >= ");
DEFINE_OPERATOR(>, greater, " > ", " <= ");
DEFINE_OPERATOR(<=, less_equal, " <= ", " > ");
DEFINE_OPERATOR(>=, greater_equal, " >= ", " < ");
DEFINE_OPERATOR(==, equal, " == ", " != ");
DEFINE_OPERATOR(!=, not_equal, " != ", " == ");

#undef DEFINE_OPERATOR

struct expression {
    std::string_view              expected = {};
    small_string<max_expr_length> actual   = {};

    template<string_appendable T>
    [[nodiscard]] bool append_value(T&& value) noexcept {
        return append(actual, std::forward<T>(value));
    }

    template<typename T>
    [[nodiscard]] bool append_value(T&&) noexcept {
        return append(actual, "?");
    }
};

template<bool CheckMode>
struct invalid_expression {
    // This is an invalid expression; any further operator should produce another invalid
    // expression. We don't want to decompose these operators, but we need to declare them
    // so the expression compiles until cast to bool. This enable conditional decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    invalid_expression<CheckMode> operator OP(const V&) noexcept {                                 \
        return {};                                                                                 \
    }

    EXPR_OPERATOR_INVALID(<=)
    EXPR_OPERATOR_INVALID(<)
    EXPR_OPERATOR_INVALID(>=)
    EXPR_OPERATOR_INVALID(>)
    EXPR_OPERATOR_INVALID(==)
    EXPR_OPERATOR_INVALID(!=)
    EXPR_OPERATOR_INVALID(&&)
    EXPR_OPERATOR_INVALID(||)
    EXPR_OPERATOR_INVALID(=)
    EXPR_OPERATOR_INVALID(+=)
    EXPR_OPERATOR_INVALID(-=)
    EXPR_OPERATOR_INVALID(*=)
    EXPR_OPERATOR_INVALID(/=)
    EXPR_OPERATOR_INVALID(%=)
    EXPR_OPERATOR_INVALID(^=)
    EXPR_OPERATOR_INVALID(&=)
    EXPR_OPERATOR_INVALID(|=)
    EXPR_OPERATOR_INVALID(<<=)
    EXPR_OPERATOR_INVALID(>>=)
    EXPR_OPERATOR_INVALID(^)
    EXPR_OPERATOR_INVALID(|)
    EXPR_OPERATOR_INVALID(&)

#undef EXPR_OPERATOR_INVALID

    explicit operator bool() const noexcept
        requires(!CheckMode)
    {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression, but just in case:
        static_assert(CheckMode != CheckMode, "snitch bug: cannot decompose chained expression");
        return false;
    }
};

template<bool CheckMode, bool Expected, typename T, typename O, typename U>
struct extracted_binary_expression {
    expression& expr;
    const T&    lhs;
    const U&    rhs;

    // This is a binary expression; any further operator should produce an invalid
    // expression, since we can't/won't decompose complex expressions. We don't want to decompose
    // these operators, but we need to declare them so the expression compiles until cast to bool.
    // This enable conditional decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    invalid_expression<CheckMode> operator OP(const V&) noexcept {                                 \
        return {};                                                                                 \
    }

    EXPR_OPERATOR_INVALID(<=)
    EXPR_OPERATOR_INVALID(<)
    EXPR_OPERATOR_INVALID(>=)
    EXPR_OPERATOR_INVALID(>)
    EXPR_OPERATOR_INVALID(==)
    EXPR_OPERATOR_INVALID(!=)
    EXPR_OPERATOR_INVALID(&&)
    EXPR_OPERATOR_INVALID(||)
    EXPR_OPERATOR_INVALID(=)
    EXPR_OPERATOR_INVALID(+=)
    EXPR_OPERATOR_INVALID(-=)
    EXPR_OPERATOR_INVALID(*=)
    EXPR_OPERATOR_INVALID(/=)
    EXPR_OPERATOR_INVALID(%=)
    EXPR_OPERATOR_INVALID(^=)
    EXPR_OPERATOR_INVALID(&=)
    EXPR_OPERATOR_INVALID(|=)
    EXPR_OPERATOR_INVALID(<<=)
    EXPR_OPERATOR_INVALID(>>=)
    EXPR_OPERATOR_INVALID(^)
    EXPR_OPERATOR_INVALID(|)
    EXPR_OPERATOR_INVALID(&)

#undef EXPR_OPERATOR_INVALID

    explicit operator bool() const noexcept
        requires(!CheckMode || requires(const T& lhs, const U& rhs) { O{}(lhs, rhs); })
    {
        if (O{}(lhs, rhs) != Expected) {
            if constexpr (matcher_for<T, U>) {
                using namespace snitch::matchers;
                constexpr auto status = std::is_same_v<O, operator_equal> == Expected
                                            ? match_status::failed
                                            : match_status::matched;
                if (!expr.append_value(lhs.describe_match(rhs, status))) {
                    expr.actual.clear();
                }
            } else if constexpr (matcher_for<U, T>) {
                using namespace snitch::matchers;
                constexpr auto status = std::is_same_v<O, operator_equal> == Expected
                                            ? match_status::failed
                                            : match_status::matched;
                if (!expr.append_value(rhs.describe_match(lhs, status))) {
                    expr.actual.clear();
                }
            } else {
                if (!expr.append_value(lhs) ||
                    !(Expected ? expr.append_value(O::inverse) : expr.append_value(O::actual)) ||
                    !expr.append_value(rhs)) {
                    expr.actual.clear();
                }
            }

            return true;
        }

        return false;
    }
};

template<bool CheckMode, bool Expected, typename T>
struct extracted_unary_expression {
    expression& expr;
    const T&    lhs;

    // Operators we want to decompose.
#define EXPR_OPERATOR(OP, OP_TYPE)                                                                 \
    template<typename U>                                                                           \
    constexpr extracted_binary_expression<CheckMode, Expected, T, OP_TYPE, U> operator OP(         \
        const U& rhs) const noexcept {                                                             \
        return {expr, lhs, rhs};                                                                   \
    }

    EXPR_OPERATOR(<, operator_less)
    EXPR_OPERATOR(>, operator_greater)
    EXPR_OPERATOR(<=, operator_less_equal)
    EXPR_OPERATOR(>=, operator_greater_equal)
    EXPR_OPERATOR(==, operator_equal)
    EXPR_OPERATOR(!=, operator_not_equal)

#undef EXPR_OPERATOR

    // We don't want to decompose the following operators, but we need to declare them so the
    // expression compiles until cast to bool. This enable conditional decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    invalid_expression<CheckMode> operator OP(const V&) noexcept {                                 \
        return {};                                                                                 \
    }

    EXPR_OPERATOR_INVALID(&&)
    EXPR_OPERATOR_INVALID(||)
    EXPR_OPERATOR_INVALID(=)
    EXPR_OPERATOR_INVALID(+=)
    EXPR_OPERATOR_INVALID(-=)
    EXPR_OPERATOR_INVALID(*=)
    EXPR_OPERATOR_INVALID(/=)
    EXPR_OPERATOR_INVALID(%=)
    EXPR_OPERATOR_INVALID(^=)
    EXPR_OPERATOR_INVALID(&=)
    EXPR_OPERATOR_INVALID(|=)
    EXPR_OPERATOR_INVALID(<<=)
    EXPR_OPERATOR_INVALID(>>=)
    EXPR_OPERATOR_INVALID(^)
    EXPR_OPERATOR_INVALID(|)
    EXPR_OPERATOR_INVALID(&)

#undef EXPR_OPERATOR_INVALID

    explicit operator bool() const noexcept
        requires(!CheckMode || requires(const T& lhs) { static_cast<bool>(lhs); })
    {
        if (static_cast<bool>(lhs) != Expected) {
            if (!expr.append_value(lhs)) {
                expr.actual.clear();
            }

            return true;
        }

        return false;
    }
};

template<bool CheckMode, bool Expected>
struct expression_extractor {
    expression& expr;

    template<typename T>
    constexpr extracted_unary_expression<CheckMode, Expected, T>
    operator<=(const T& lhs) const noexcept {
        return {expr, lhs};
    }
};

template<typename T>
constexpr bool is_decomposable = requires(const T& t) { static_cast<bool>(t); };

struct scoped_capture {
    capture_state& captures;
    std::size_t    count = 0;

    ~scoped_capture() noexcept {
        captures.resize(captures.size() - count);
    }
};

std::string_view extract_next_name(std::string_view& names) noexcept;

small_string<max_capture_length>& add_capture(test_state& state) noexcept;

template<string_appendable T>
void add_capture(test_state& state, std::string_view& names, const T& arg) noexcept {
    auto& capture = add_capture(state);
    append_or_truncate(capture, extract_next_name(names), " := ", arg);
}

template<string_appendable... Args>
scoped_capture
add_captures(test_state& state, std::string_view names, const Args&... args) noexcept {
    (add_capture(state, names, args), ...);
    return {state.captures, sizeof...(args)};
}

template<string_appendable... Args>
scoped_capture add_info(test_state& state, const Args&... args) noexcept {
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
} // namespace snitch::impl

// Sections and captures.
// ---------

namespace snitch {
using section_info = small_vector_span<const section_id>;
using capture_info = small_vector_span<const std::string_view>;
} // namespace snitch

// Events.
// -------

namespace snitch {
struct assertion_location {
    std::string_view file = {};
    std::size_t      line = 0u;
};

enum class test_case_state { success, failed, skipped };

namespace event {
struct test_run_started {
    std::string_view name = {};
};

struct test_run_ended {
    std::string_view name            = {};
    bool             success         = true;
    std::size_t      run_count       = 0;
    std::size_t      fail_count      = 0;
    std::size_t      skip_count      = 0;
    std::size_t      assertion_count = 0;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct test_case_started {
    const test_id& id;
};

struct test_case_ended {
    const test_id&  id;
    test_case_state state           = test_case_state::success;
    std::size_t     assertion_count = 0;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct assertion_failed {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    std::string_view          message  = {};
    bool                      expected = false;
    bool                      allowed  = false;
};

struct test_case_skipped {
    const test_id&            id;
    section_info              sections = {};
    capture_info              captures = {};
    const assertion_location& location;
    std::string_view          message = {};
};

using data = std::variant<
    test_run_started,
    test_run_ended,
    test_case_started,
    test_case_ended,
    assertion_failed,
    test_case_skipped>;
}; // namespace event
} // namespace snitch

// Command line interface.
// -----------------------

namespace snitch::cli {
struct argument {
    std::string_view                name       = {};
    std::optional<std::string_view> value_name = {};
    std::optional<std::string_view> value      = {};
};

struct input {
    std::string_view                              executable = {};
    small_vector<argument, max_command_line_args> arguments  = {};
};

extern small_function<void(std::string_view) noexcept> console_print;

std::optional<input> parse_arguments(int argc, const char* const argv[]) noexcept;

std::optional<cli::argument> get_option(const cli::input& args, std::string_view name) noexcept;

std::optional<cli::argument>
get_positional_argument(const cli::input& args, std::string_view name) noexcept;

void for_each_positional_argument(
    const cli::input&                                      args,
    std::string_view                                       name,
    const small_function<void(std::string_view) noexcept>& callback) noexcept;
} // namespace snitch::cli

// Test registry.
// --------------

namespace snitch {
class registry {
    small_vector<impl::test_case, max_test_cases> test_list;

    void print_location(
        const impl::test_case&     current_case,
        const impl::section_state& sections,
        const impl::capture_state& captures,
        const assertion_location&  location) const noexcept;

    void print_failure() const noexcept;
    void print_expected_failure() const noexcept;
    void print_skip() const noexcept;
    void print_details(std::string_view message) const noexcept;
    void print_details_expr(const impl::expression& exp) const noexcept;

public:
    enum class verbosity { quiet, normal, high } verbose = verbosity::normal;
    bool with_color                                      = true;

    using print_function  = small_function<void(std::string_view) noexcept>;
    using report_function = small_function<void(const registry&, const event::data&) noexcept>;

    print_function  print_callback = &snitch::impl::stdout_print;
    report_function report_callback;

    template<typename... Args>
    void print(Args&&... args) const noexcept {
        small_string<max_message_length> message;
        append_or_truncate(message, std::forward<Args>(args)...);
        this->print_callback(message);
    }

    const char* add(const test_id& id, impl::test_ptr func) noexcept;

    template<typename... Args, typename F>
    const char*
    add_with_types(std::string_view name, std::string_view tags, const F& func) noexcept {
        return (
            add({name, tags, impl::get_type_name<Args>()}, impl::to_test_case_ptr<Args>(func)),
            ...);
    }

    template<typename T, typename F>
    const char*
    add_with_type_list(std::string_view name, std::string_view tags, const F& func) noexcept {
        return [&]<template<typename...> typename TL, typename... Args>(type_list<TL<Args...>>) {
            return this->add_with_types<Args...>(name, tags, func);
        }(type_list<T>{});
    }

    void report_failure(
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message) const noexcept;

    void report_failure(
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message1,
        std::string_view          message2) const noexcept;

    void report_failure(
        impl::test_state&         state,
        const assertion_location& location,
        const impl::expression&   exp) const noexcept;

    void report_skipped(
        impl::test_state&         state,
        const assertion_location& location,
        std::string_view          message) const noexcept;

    impl::test_state run(impl::test_case& test) noexcept;

    bool run_tests(std::string_view run_name) noexcept;

    bool run_selected_tests(
        std::string_view                                     run_name,
        const small_function<bool(const test_id&) noexcept>& filter) noexcept;

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
} // namespace snitch

// Matchers.
// ---------

namespace snitch::matchers {
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

    template<snitch::impl::exception_with_what E>
    bool match(const E& e) const noexcept {
        return contains_substring::match(e.what());
    }

    template<snitch::impl::exception_with_what E>
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
} // namespace snitch::matchers

// Compiler warning handling.
// --------------------------

// clang-format off
#if defined(__clang__)
#    define SNITCH_WARNING_PUSH _Pragma("clang diagnostic push")
#    define SNITCH_WARNING_POP _Pragma("clang diagnostic pop")
#    define SNITCH_WARNING_DISABLE_PARENTHESES _Pragma("clang diagnostic ignored \"-Wparentheses\"")
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON
#elif defined(__GNUC__)
#    define SNITCH_WARNING_PUSH _Pragma("GCC diagnostic push")
#    define SNITCH_WARNING_POP _Pragma("GCC diagnostic pop")
#    define SNITCH_WARNING_DISABLE_PARENTHESES _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON
#elif defined(_MSC_VER)
#    define SNITCH_WARNING_PUSH _Pragma("warning(push)")
#    define SNITCH_WARNING_POP _Pragma("warning(pop)")
#    define SNITCH_WARNING_DISABLE_PARENTHESES
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON _Pragma("warning(disable: 4127)")
#else
#    define SNITCH_WARNING_PUSH
#    define SNITCH_WARNING_POP
#    define SNITCH_WARNING_DISABLE_PARENTHESES
#    define SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON
#endif
// clang-format on

// Internal test macros.
// ---------------------

#if SNITCH_WITH_EXCEPTIONS
#    define SNITCH_TESTING_ABORT                                                                   \
        throw snitch::impl::abort_exception {}
#else
#    define SNITCH_TESTING_ABORT std::terminate()
#endif

#define SNITCH_CONCAT_IMPL(x, y) x##y
#define SNITCH_MACRO_CONCAT(x, y) SNITCH_CONCAT_IMPL(x, y)

#define SNITCH_EXPR_TRUE(TYPE, ...)                                                                \
    auto SNITCH_CURRENT_EXPRESSION = snitch::impl::expression{TYPE "(" #__VA_ARGS__ ")"};          \
    snitch::impl::expression_extractor<false, true>{SNITCH_CURRENT_EXPRESSION} <= __VA_ARGS__

#define SNITCH_EXPR_FALSE(TYPE, ...)                                                               \
    auto SNITCH_CURRENT_EXPRESSION = snitch::impl::expression{TYPE "(" #__VA_ARGS__ ")"};          \
    snitch::impl::expression_extractor<false, false>{SNITCH_CURRENT_EXPRESSION} <= __VA_ARGS__

#define SNITCH_DECOMPOSABLE(...)                                                                   \
    snitch::impl::is_decomposable<                                                                 \
        decltype(snitch::impl::expression_extractor<true, true>{std::declval<snitch::impl::expression&>()} <= __VA_ARGS__)>

// Public test macros: test cases.
// -------------------------------

#define SNITCH_TEST_CASE_IMPL(ID, ...)                                                             \
    static void        ID();                                                                       \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add({__VA_ARGS__}, &ID);                                                     \
    void ID()

#define SNITCH_TEST_CASE(...)                                                                      \
    SNITCH_TEST_CASE_IMPL(SNITCH_MACRO_CONCAT(test_fun_, __COUNTER__), __VA_ARGS__)

#define SNITCH_TEMPLATE_LIST_TEST_CASE_IMPL(ID, NAME, TAGS, TYPES)                                 \
    template<typename TestType>                                                                    \
    static void        ID();                                                                       \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_with_type_list<TYPES>(                                                   \
            NAME, TAGS, []<typename TestType>() { ID<TestType>(); });                              \
    template<typename TestType>                                                                    \
    void ID()

#define SNITCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)                                          \
    SNITCH_TEMPLATE_LIST_TEST_CASE_IMPL(                                                           \
        SNITCH_MACRO_CONCAT(test_fun_, __COUNTER__), NAME, TAGS, TYPES)

#define SNITCH_TEMPLATE_TEST_CASE_IMPL(ID, NAME, TAGS, ...)                                        \
    template<typename TestType>                                                                    \
    static void        ID();                                                                       \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_with_types<__VA_ARGS__>(                                                 \
            NAME, TAGS, []<typename TestType>() { ID<TestType>(); });                              \
    template<typename TestType>                                                                    \
    void ID()

#define SNITCH_TEMPLATE_TEST_CASE(NAME, TAGS, ...)                                                 \
    SNITCH_TEMPLATE_TEST_CASE_IMPL(                                                                \
        SNITCH_MACRO_CONCAT(test_fun_, __COUNTER__), NAME, TAGS, __VA_ARGS__)

#define SNITCH_TEST_CASE_METHOD_IMPL(ID, FIXTURE, ...)                                             \
    namespace {                                                                                    \
    struct ID : FIXTURE {                                                                          \
        void test_fun();                                                                           \
    };                                                                                             \
    }                                                                                              \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add({__VA_ARGS__}, []() { ID{}.test_fun(); });                               \
    void ID::test_fun()

#define SNITCH_TEST_CASE_METHOD(FIXTURE, ...)                                                      \
    SNITCH_TEST_CASE_METHOD_IMPL(                                                                  \
        SNITCH_MACRO_CONCAT(test_fixture_, __COUNTER__), FIXTURE, __VA_ARGS__)

#define SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD_IMPL(ID, FIXTURE, NAME, TAGS, TYPES)                 \
    namespace {                                                                                    \
    template<typename TestType>                                                                    \
    struct ID : FIXTURE<TestType> {                                                                \
        void test_fun();                                                                           \
    };                                                                                             \
    }                                                                                              \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_with_types<TYPES>(                                                       \
            NAME, TAGS, []() < typename TestType > { ID<TestType>{}.test_fun(); });                \
    template<typename TestType>                                                                    \
    void ID<TestType>::test_fun()

#define SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES)                          \
    SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD_IMPL(                                                    \
        SNITCH_MACRO_CONCAT(test_fixture_, __COUNTER__), FIXTURE, NAME, TAGS, TYPES)

#define SNITCH_TEMPLATE_TEST_CASE_METHOD_IMPL(ID, FIXTURE, NAME, TAGS, ...)                        \
    namespace {                                                                                    \
    template<typename TestType>                                                                    \
    struct ID : FIXTURE<TestType> {                                                                \
        void test_fun();                                                                           \
    };                                                                                             \
    }                                                                                              \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_with_types<__VA_ARGS__>(                                                 \
            NAME, TAGS, []() < typename TestType > { ID<TestType>{}.test_fun(); });                \
    template<typename TestType>                                                                    \
    void ID<TestType>::test_fun()

#define SNITCH_TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, ...)                                 \
    SNITCH_TEMPLATE_TEST_CASE_METHOD_IMPL(                                                         \
        SNITCH_MACRO_CONCAT(test_fixture_, __COUNTER__), FIXTURE, NAME, TAGS, __VA_ARGS__)

// Public test macros: utilities.
// ------------------------------

#define SNITCH_SECTION(...)                                                                        \
    if (snitch::impl::section_entry_checker SNITCH_MACRO_CONCAT(section_id_, __COUNTER__){         \
            {__VA_ARGS__}, snitch::impl::get_current_test()})

#define SNITCH_CAPTURE(...)                                                                        \
    auto SNITCH_MACRO_CONCAT(capture_id_, __COUNTER__) =                                           \
        snitch::impl::add_captures(snitch::impl::get_current_test(), #__VA_ARGS__, __VA_ARGS__)

#define SNITCH_INFO(...)                                                                           \
    auto SNITCH_MACRO_CONCAT(capture_id_, __COUNTER__) =                                           \
        snitch::impl::add_info(snitch::impl::get_current_test(), __VA_ARGS__)

// Public test macros: checks.
// ------------------------------

#define SNITCH_REQUIRE(...)                                                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_DECOMPOSABLE(__VA_ARGS__)) {                                          \
            if (SNITCH_EXPR_TRUE("REQUIRE", __VA_ARGS__)) {                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "REQUIRE(" #__VA_ARGS__ ")");       \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CHECK(...)                                                                          \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_DECOMPOSABLE(__VA_ARGS__)) {                                          \
            if (SNITCH_EXPR_TRUE("CHECK", __VA_ARGS__)) {                                          \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "CHECK(" #__VA_ARGS__ ")");         \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_REQUIRE_FALSE(...)                                                                  \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_DECOMPOSABLE(__VA_ARGS__)) {                                          \
            if (SNITCH_EXPR_FALSE("REQUIRE_FALSE", __VA_ARGS__)) {                                 \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "REQUIRE_FALSE(" #__VA_ARGS__ ")"); \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CHECK_FALSE(...)                                                                    \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_DECOMPOSABLE(__VA_ARGS__)) {                                          \
            if (SNITCH_EXPR_FALSE("CHECK_FALSE", __VA_ARGS__)) {                                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, "CHECK_FALSE(" #__VA_ARGS__ ")");   \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_FAIL(MESSAGE)                                                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_CURRENT_TEST.reg.report_failure(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
        SNITCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNITCH_FAIL_CHECK(MESSAGE)                                                                 \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_CURRENT_TEST.reg.report_failure(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
    } while (0)

#define SNITCH_SKIP(MESSAGE)                                                                       \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.reg.report_skipped(                                                    \
            SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, (MESSAGE));                                 \
        SNITCH_TESTING_ABORT;                                                                      \
    } while (0)

#define SNITCH_REQUIRE_THAT(EXPR, ...)                                                             \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                       \
        auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                                  \
        if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                       \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                SNITCH_TEMP_MATCHER.describe_match(                                                \
                    SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));                   \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
    } while (0)

#define SNITCH_CHECK_THAT(EXPR, ...)                                                               \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                       \
        auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                                  \
        if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                       \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                SNITCH_TEMP_MATCHER.describe_match(                                                \
                    SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));                   \
        }                                                                                          \
    } while (0)

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define TEST_CASE(NAME, ...)                       SNITCH_TEST_CASE(NAME, __VA_ARGS__)
#    define TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) SNITCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)
#    define TEMPLATE_TEST_CASE(NAME, TAGS, ...)        SNITCH_TEMPLATE_TEST_CASE(NAME, TAGS, __VA_ARGS__)

#    define TEST_CASE_METHOD(FIXTURE, NAME, ...)                       SNITCH_TEST_CASE_METHOD(FIXTURE, NAME, __VA_ARGS__)
#    define TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES) SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES)
#    define TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, ...)        SNITCH_TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, __VA_ARGS__)

#    define SECTION(NAME, ...) SNITCH_SECTION(NAME, __VA_ARGS__)
#    define CAPTURE(...) SNITCH_CAPTURE(__VA_ARGS__)
#    define INFO(...)    SNITCH_INFO(__VA_ARGS__)

#    define REQUIRE(...)               SNITCH_REQUIRE(__VA_ARGS__)
#    define CHECK(...)                 SNITCH_CHECK(__VA_ARGS__)
#    define REQUIRE_FALSE(...)         SNITCH_REQUIRE_FALSE(__VA_ARGS__)
#    define CHECK_FALSE(...)           SNITCH_CHECK_FALSE(__VA_ARGS__)
#    define FAIL(MESSAGE)              SNITCH_FAIL(MESSAGE)
#    define FAIL_CHECK(MESSAGE)        SNITCH_FAIL_CHECK(MESSAGE)
#    define SKIP(MESSAGE)              SNITCH_SKIP(MESSAGE)
#    define REQUIRE_THAT(EXP, ...)     SNITCH_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CHECK_THAT(EXP, ...)       SNITCH_CHECK_THAT(EXP, __VA_ARGS__)
#endif
// clang-format on

#if SNITCH_WITH_EXCEPTIONS

#    define SNITCH_REQUIRE_THROWS_AS(EXPRESSION, ...)                                              \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                ++SNITCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #__VA_ARGS__ " expected but no exception thrown");                             \
                SNITCH_TESTING_ABORT;                                                              \
            } catch (const __VA_ARGS__&) {                                                         \
                /* success */                                                                      \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #__VA_ARGS__ " expected but other std::exception thrown; message: ",       \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #__VA_ARGS__ " expected but other unknown exception thrown");              \
                }                                                                                  \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } while (0)

#    define SNITCH_CHECK_THROWS_AS(EXPRESSION, ...)                                                \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                ++SNITCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #__VA_ARGS__ " expected but no exception thrown");                             \
            } catch (const __VA_ARGS__&) {                                                         \
                /* success */                                                                      \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #__VA_ARGS__ " expected but other std::exception thrown; message: ",       \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #__VA_ARGS__ " expected but other unknown exception thrown");              \
                }                                                                                  \
            }                                                                                      \
        } while (0)

#    define SNITCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)                              \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                ++SNITCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #EXCEPTION " expected but no exception thrown");                               \
                SNITCH_TESTING_ABORT;                                                              \
            } catch (const EXCEPTION& e) {                                                         \
                auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                          \
                if (!SNITCH_TEMP_MATCHER.match(e)) {                                               \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        SNITCH_TEMP_MATCHER.describe_match(                                        \
                            e, snitch::matchers::match_status::failed));                           \
                    SNITCH_TESTING_ABORT;                                                          \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } while (0)

#    define SNITCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)                                \
        do {                                                                                       \
            auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                          \
            try {                                                                                  \
                ++SNITCH_CURRENT_TEST.asserts;                                                     \
                EXPRESSION;                                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    #EXCEPTION " expected but no exception thrown");                               \
            } catch (const EXCEPTION& e) {                                                         \
                auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                          \
                if (!SNITCH_TEMP_MATCHER.match(e)) {                                               \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        "could not match caught " #EXCEPTION " with expected content: ",           \
                        SNITCH_TEMP_MATCHER.describe_match(                                        \
                            e, snitch::matchers::match_status::failed));                           \
                }                                                                                  \
            } catch (...) {                                                                        \
                try {                                                                              \
                    throw;                                                                         \
                } catch (const std::exception& e) {                                                \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other std::exception thrown; message: ",         \
                        e.what());                                                                 \
                } catch (...) {                                                                    \
                    SNITCH_CURRENT_TEST.reg.report_failure(                                        \
                        SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                 \
                        #EXCEPTION " expected but other unknown exception thrown");                \
                }                                                                                  \
            }                                                                                      \
        } while (0)

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define REQUIRE_THROWS_AS(EXPRESSION, ...)                 SNITCH_REQUIRE_THROWS_AS(EXPRESSION, __VA_ARGS__)
#    define CHECK_THROWS_AS(EXPRESSION, ...)                   SNITCH_CHECK_THROWS_AS(EXPRESSION, __VA_ARGS__)
#    define REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...) SNITCH_REQUIRE_THROWS_MATCHES(EXPRESSION, EXCEPTION, __VA_ARGS__)
#    define CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, ...)   SNITCH_CHECK_THROWS_MATCHES(EXPRESSION, EXCEPTION, __VA_ARGS__)
#endif
// clang-format on

#endif

#endif
