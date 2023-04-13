#ifndef SNITCH_HPP
#define SNITCH_HPP

#include "snitch/snitch_config.hpp"

#include <array> // for small_vector
#include <cstddef> // for std::size_t
#if SNITCH_WITH_EXCEPTIONS
#    include <exception> // for std::exception
#endif
#if SNITCH_CONSTEXPR_FLOAT_USE_BITCAST
#    include <bit> // for compile-time float to string
#endif
#include <initializer_list> // for std::initializer_list
#include <limits> // for compile-time integer to string
#include <optional> // for cli
#include <string_view> // for all strings
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

template<typename T>
concept signed_integral = std::is_signed_v<T>;

template<typename T>
concept unsigned_integral = std::is_unsigned_v<T>;

template<typename T>
concept floating_point = std::is_floating_point_v<T>;

template<typename T, typename U>
concept convertible_to = std::is_convertible_v<T, U>;

template<typename T>
concept enumeration = std::is_enum_v<T>;

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

// Public utilities: small_function.
// ---------------------------------

namespace snitch::impl {
template<typename T>
struct function_traits {
    static_assert(!std::is_same_v<T, T>, "incorrect template parameter for small_function");
};

template<typename T, bool Noexcept>
struct function_traits_base {
    static_assert(!std::is_same_v<T, T>, "incorrect template parameter for small_function");
};

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...) noexcept> {
    using return_type             = Ret;
    using function_ptr            = Ret (*)(Args...) noexcept;
    using function_data_ptr       = Ret (*)(void*, Args...) noexcept;
    using function_const_data_ptr = Ret (*)(const void*, Args...) noexcept;

    static constexpr bool is_noexcept = true;

    template<typename ObjectType, auto MemberFunction>
    static constexpr function_data_ptr to_free_function() noexcept {
        return [](void* ptr, Args... args) noexcept {
            if constexpr (std::is_same_v<return_type, void>) {
                (static_cast<ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            } else {
                return (static_cast<ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            }
        };
    }

    template<typename ObjectType, auto MemberFunction>
    static constexpr function_const_data_ptr to_const_free_function() noexcept {
        return [](const void* ptr, Args... args) noexcept {
            if constexpr (std::is_same_v<return_type, void>) {
                (static_cast<const ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            } else {
                return (static_cast<const ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            }
        };
    }
};

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
    using return_type             = Ret;
    using function_ptr            = Ret (*)(Args...);
    using function_data_ptr       = Ret (*)(void*, Args...);
    using function_const_data_ptr = Ret (*)(const void*, Args...);

    static constexpr bool is_noexcept = false;

    template<typename ObjectType, auto MemberFunction>
    static constexpr function_data_ptr to_free_function() noexcept {
        return [](void* ptr, Args... args) {
            if constexpr (std::is_same_v<return_type, void>) {
                (static_cast<ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            } else {
                return (static_cast<ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            }
        };
    }

    template<typename ObjectType, auto MemberFunction>
    static constexpr function_const_data_ptr to_const_free_function() noexcept {
        return [](const void* ptr, Args... args) {
            if constexpr (std::is_same_v<return_type, void>) {
                (static_cast<const ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            } else {
                return (static_cast<const ObjectType*>(ptr)->*constant<MemberFunction>::value)(
                    std::move(args)...);
            }
        };
    }
};
} // namespace snitch::impl

namespace snitch {
template<typename T>
class small_function {
    using traits = impl::function_traits<T>;

public:
    using return_type             = typename traits::return_type;
    using function_ptr            = typename traits::function_ptr;
    using function_data_ptr       = typename traits::function_data_ptr;
    using function_const_data_ptr = typename traits::function_const_data_ptr;

private:
    struct function_and_data_ptr {
        void*             data = nullptr;
        function_data_ptr ptr;
    };

    struct function_and_const_data_ptr {
        const void*             data = nullptr;
        function_const_data_ptr ptr;
    };

    using data_type =
        std::variant<function_ptr, function_and_data_ptr, function_and_const_data_ptr>;

    data_type data;

public:
    constexpr small_function(function_ptr ptr) noexcept : data{ptr} {}

    template<convertible_to<function_ptr> FunctionType>
    constexpr small_function(FunctionType&& obj) noexcept : data{static_cast<function_ptr>(obj)} {}

    template<typename ObjectType, auto MemberFunction>
    constexpr small_function(ObjectType& obj, constant<MemberFunction>) noexcept :
        data{function_and_data_ptr{
            &obj, traits::template to_free_function<ObjectType, MemberFunction>()}} {}

    template<typename ObjectType, auto MemberFunction>
    constexpr small_function(const ObjectType& obj, constant<MemberFunction>) noexcept :
        data{function_and_const_data_ptr{
            &obj, traits::template to_const_free_function<ObjectType, MemberFunction>()}} {}

    template<typename FunctorType>
    constexpr small_function(FunctorType& obj) noexcept :
        small_function(obj, constant<&FunctorType::operator()>{}) {}

    template<typename FunctorType>
    constexpr small_function(const FunctorType& obj) noexcept :
        small_function(obj, constant<&FunctorType::operator()>{}) {}

    // Prevent inadvertently using temporary stateful lambda; not supported at the moment.
    template<typename FunctorType>
    constexpr small_function(FunctorType&& obj) noexcept = delete;

    // Prevent inadvertently using temporary object; not supported at the moment.
    template<typename FunctorType, auto M>
    constexpr small_function(FunctorType&& obj, constant<M>) noexcept = delete;

    template<typename... CArgs>
    constexpr return_type operator()(CArgs&&... args) const noexcept(traits::is_noexcept) {
        if constexpr (std::is_same_v<return_type, void>) {
            std::visit(
                overload{
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
};
} // namespace snitch

// Public utilities.
// ------------------------------------------------

namespace snitch {
template<typename T>
constexpr std::string_view type_name = impl::get_type_name<T>();

template<typename... Args>
struct type_list {};

[[noreturn]] void terminate_with(std::string_view msg) noexcept;

extern small_function<void(std::string_view)> assertion_failed_handler;

[[noreturn]] void assertion_failed(std::string_view msg);
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

    // Requires: new_size <= capacity().
    constexpr void resize(std::size_t new_size) {
        if (new_size > buffer_size) {
            assertion_failed("small vector is full");
        }

        *data_size = new_size;
    }

    // Requires: size() + elem <= capacity().
    constexpr void grow(std::size_t elem) {
        if (*data_size + elem > buffer_size) {
            assertion_failed("small vector is full");
        }

        *data_size += elem;
    }

    // Requires: size() < capacity().
    constexpr ElemType& push_back(const ElemType& t) {
        if (*data_size == buffer_size) {
            assertion_failed("small vector is full");
        }

        ++*data_size;

        ElemType& elem = buffer_ptr[*data_size - 1];
        elem           = t;

        return elem;
    }

    // Requires: size() < capacity().
    constexpr ElemType& push_back(ElemType&& t) {
        if (*data_size == buffer_size) {
            assertion_failed("small vector is full");
        }

        ++*data_size;
        ElemType& elem = buffer_ptr[*data_size - 1];
        elem           = std::move(t);

        return elem;
    }

    // Requires: !empty().
    constexpr void pop_back() {
        if (*data_size == 0) {
            assertion_failed("pop_back() called on empty vector");
        }

        --*data_size;
    }

    // Requires: !empty().
    constexpr ElemType& back() {
        if (*data_size == 0) {
            assertion_failed("back() called on empty vector");
        }

        return buffer_ptr[*data_size - 1];
    }

    // Requires: !empty().
    constexpr const ElemType& back() const {
        if (*data_size == 0) {
            assertion_failed("back() called on empty vector");
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

    // Requires: i < size().
    constexpr ElemType& operator[](std::size_t i) {
        if (i >= size()) {
            assertion_failed("operator[] called with incorrect index");
        }
        return buffer_ptr[i];
    }

    // Requires: i < size().
    constexpr const ElemType& operator[](std::size_t i) const {
        if (i >= size()) {
            assertion_failed("operator[] called with incorrect index");
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

    // Requires: !empty().
    constexpr const ElemType& back() const {
        if (*data_size == 0) {
            assertion_failed("back() called on empty vector");
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

    // Requires: i < size().
    constexpr const ElemType& operator[](std::size_t i) const {
        if (i >= size()) {
            assertion_failed("operator[] called with incorrect index");
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
    constexpr small_vector(std::initializer_list<ElemType> list) {
        for (const auto& e : list) {
            span().push_back(e);
        }
    }

    constexpr small_vector& operator=(const small_vector& other) noexcept = default;
    constexpr small_vector& operator=(small_vector&& other) noexcept      = default;

    constexpr std::size_t capacity() const noexcept {
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

    // Requires: new_size <= capacity().
    constexpr void resize(std::size_t size) {
        span().resize(size);
    }

    // Requires: size() + elem <= capacity().
    constexpr void grow(std::size_t elem) {
        span().grow(elem);
    }

    // Requires: size() < capacity().
    constexpr ElemType& push_back(const ElemType& t) {
        return this->span().push_back(t);
    }

    // Requires: size() < capacity().
    constexpr ElemType& push_back(ElemType&& t) {
        return this->span().push_back(t);
    }

    // Requires: !empty().
    constexpr void pop_back() {
        return span().pop_back();
    }

    // Requires: !empty().
    constexpr ElemType& back() {
        return span().back();
    }

    // Requires: !empty().
    constexpr const ElemType& back() const {
        return span().back();
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

    // Requires: i < size().
    constexpr ElemType& operator[](std::size_t i) {
        return span()[i];
    }

    // Requires: i < size().
    constexpr const ElemType& operator[](std::size_t i) const {
        return span()[i];
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

    // Requires: str.size() <= MaxLength.
    constexpr small_string(std::string_view str) {
        resize(str.size());
        for (std::size_t i = 0; i < str.size(); ++i) {
            data_buffer[i] = str[i];
        }
    }

    constexpr small_string& operator=(const small_string& other) noexcept = default;
    constexpr small_string& operator=(small_string&& other) noexcept      = default;

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

    // Requires: new_size <= capacity().
    constexpr void resize(std::size_t length) {
        span().resize(length);
    }

    // Requires: size() + elem <= capacity().
    constexpr void grow(std::size_t chars) {
        span().grow(chars);
    }

    // Requires: size() < capacity().
    constexpr char& push_back(char t) {
        return span().push_back(t);
    }

    // Requires: !empty().
    constexpr void pop_back() {
        return span().pop_back();
    }

    // Requires: !empty().
    constexpr char& back() {
        return span().back();
    }

    // Requires: !empty().
    constexpr const char& back() const {
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

    // Requires: i < size().
    constexpr char& operator[](std::size_t i) {
        return span()[i];
    }

    // Requires: i < size().
    constexpr char operator[](std::size_t i) const {
        return const_cast<small_string*>(this)->span()[i];
    }
};
} // namespace snitch

// Internal utilities: fixed point types.
// --------------------------------------

namespace snitch::impl {
using fixed_digits_t = std::uint64_t;
using fixed_exp_t    = std::int32_t;

struct unsigned_fixed_data {
    fixed_digits_t digits   = 0;
    fixed_exp_t    exponent = 0;
};

struct signed_fixed_data {
    fixed_digits_t digits   = 0;
    fixed_exp_t    exponent = 0;
    bool           sign     = false;
};

struct unpacked64 {
    std::uint64_t l;
    std::uint64_t u;
};

constexpr unpacked64 unpack10(std::uint64_t v) noexcept {
    return {v % 10'000'000'000, v / 10'000'000'000};
}

class unsigned_fixed {
    unsigned_fixed_data data = {};

    constexpr void raise_exponent_to(fixed_exp_t new_exponent) noexcept {
        do {
            if (data.digits == 0u) {
                data.exponent = new_exponent;
            } else if (data.exponent < new_exponent - 1) {
                data.digits = data.digits / 10u;
                data.exponent += 1;
            } else {
                data.digits = (data.digits + 5u) / 10u;
                data.exponent += 1;
            }
        } while (data.exponent < new_exponent);
    }

    constexpr void raise_exponent() noexcept {
        data.digits = (data.digits + 5u) / 10u;
        data.exponent += 1;
    }

public:
    constexpr unsigned_fixed(fixed_digits_t digits_in, fixed_exp_t exponent_in) noexcept {
        // Normalise inputs so that we maximize the number of digits stored.
        if (digits_in > 0) {
            constexpr fixed_digits_t cap = std::numeric_limits<fixed_digits_t>::max() / 10u;

            if (digits_in < cap) {
                do {
                    digits_in *= 10u;
                    exponent_in -= 1;
                } while (digits_in < cap);
            }
        } else {
            // Pick the smallest possible exponent for zero;
            // This guarantees that we will preserve precision for whatever number
            // gets added to this.
            exponent_in = std::numeric_limits<fixed_exp_t>::min();
        }

        data.digits   = digits_in;
        data.exponent = exponent_in;
    }

    constexpr fixed_digits_t digits() const noexcept {
        return data.digits;
    }

    constexpr fixed_exp_t exponent() const noexcept {
        return data.exponent;
    }

    friend constexpr unsigned_fixed operator+(unsigned_fixed f1, unsigned_fixed f2) noexcept {
        // Bring both numbers to the same exponent before summing.
        // To prevent overflow: add one to the exponent.
        if (f1.data.exponent > f2.data.exponent) {
            f1.raise_exponent();
            f2.raise_exponent_to(f1.data.exponent + 1);
        } else if (f1.data.exponent < f2.data.exponent) {
            f1.raise_exponent_to(f2.data.exponent + 1);
            f2.raise_exponent();
        } else {
            f1.raise_exponent();
            f2.raise_exponent();
        }

        return unsigned_fixed(f1.data.digits + f2.data.digits, f1.data.exponent);
    }

    constexpr unsigned_fixed& operator+=(const unsigned_fixed f) noexcept {
        return *this = *this + f;
    }

    friend constexpr unsigned_fixed
    operator*(const unsigned_fixed f1, const unsigned_fixed f2) noexcept {
        // To prevent overflow: split each number as f_i = u_i*1e10 + l_i,
        // with l_i and u_i < 1e10, then develop the multiplication of each component:
        //   r = f1*f2 = u1*u2*1e20 + (l1*u2 + l2*u1)*1e10 + l1*l2
        // The resulting integer would overflow, so insted of storing the digits of r, we
        // store the digits of r/1e20:
        //   r/1e20 = u1*u2 + (l1*u2 + l2*u1)/1e10 + l1*l2/1e20 = u + l/1e10 + ll/1e20.
        // For simplicity, we ignore the term ll/1e20 since it is < 0.2 and would at most
        // contribute to changing the last digit of the output integer.

        const auto [l1, u1] = unpack10(f1.data.digits);
        const auto [l2, u2] = unpack10(f2.data.digits);

        // For the (l1*u2 + l2*u1) term, divide by 10 and round each component before summing,
        // since the addition may overflow. Note: although l < 1e10, and l*l can overflow, u < 2e9
        // so l*u cannot overflow.
        const fixed_digits_t l_over_10 = (l1 * u2 + 5u) / 10u + (l2 * u1 + 5u) / 10u;
        // Then shift the digits to the right, with rounding.
        const fixed_digits_t l_over_1e10 = (l_over_10 + 500'000'000) / 1'000'000'000;

        // u1*u2 is straightforward.
        const fixed_digits_t u = u1 * u2;

        // Adding back the lower part cannot overflow, by construction. The exponent
        // is increased by 20 because we computed the digits of (f1*f2)/1e20.
        return unsigned_fixed(u + l_over_1e10, f1.data.exponent + f2.data.exponent + 20);
    }

    constexpr unsigned_fixed& operator*=(const unsigned_fixed f) noexcept {
        return *this = *this * f;
    }
};

template<typename T>
struct float_traits;

template<>
struct float_traits<float> {
    using bits_full_t = std::uint32_t;
    using bits_sig_t  = std::uint32_t;
    using bits_exp_t  = std::uint8_t;

    using int_exp_t = std::int32_t;

    static constexpr bits_full_t bits     = 8u * sizeof(bits_full_t);
    static constexpr bits_full_t sig_bits = 23u;
    static constexpr bits_full_t exp_bits = bits - sig_bits - 1u;

    static constexpr bits_full_t sign_mask = bits_full_t{1u} << (bits - 1u);
    static constexpr bits_full_t sig_mask  = (bits_full_t{1u} << sig_bits) - 1u;
    static constexpr bits_full_t exp_mask  = ((bits_full_t{1u} << (bits - 1u)) - 1u) & ~sig_mask;

    static constexpr int_exp_t exp_origin    = -127;
    static constexpr int_exp_t exp_subnormal = exp_origin + 1;

    static constexpr bits_exp_t exp_bits_special = 0xff;
    static constexpr bits_sig_t sig_bits_nan     = 0x400000;
    static constexpr bits_sig_t sig_bits_inf     = 0x0;

    static constexpr std::size_t precision = 7u;

    static constexpr std::array<unsigned_fixed, sig_bits> sig_elems = {
        {unsigned_fixed(1192092895507812500u, -25), unsigned_fixed(2384185791015625000u, -25),
         unsigned_fixed(4768371582031250000u, -25), unsigned_fixed(9536743164062500000u, -25),
         unsigned_fixed(1907348632812500000u, -24), unsigned_fixed(3814697265625000000u, -24),
         unsigned_fixed(7629394531250000000u, -24), unsigned_fixed(1525878906250000000u, -23),
         unsigned_fixed(3051757812500000000u, -23), unsigned_fixed(6103515625000000000u, -23),
         unsigned_fixed(1220703125000000000u, -22), unsigned_fixed(2441406250000000000u, -22),
         unsigned_fixed(4882812500000000000u, -22), unsigned_fixed(9765625000000000000u, -22),
         unsigned_fixed(1953125000000000000u, -21), unsigned_fixed(3906250000000000000u, -21),
         unsigned_fixed(7812500000000000000u, -21), unsigned_fixed(1562500000000000000u, -20),
         unsigned_fixed(3125000000000000000u, -20), unsigned_fixed(6250000000000000000u, -20),
         unsigned_fixed(1250000000000000000u, -19), unsigned_fixed(2500000000000000000u, -19),
         unsigned_fixed(5000000000000000000u, -19)}};
};

template<>
struct float_traits<double> {
    using bits_full_t = std::uint64_t;
    using bits_sig_t  = std::uint64_t;
    using bits_exp_t  = std::uint16_t;

    using int_exp_t = std::int32_t;

    static constexpr bits_full_t bits     = 8u * sizeof(bits_full_t);
    static constexpr bits_full_t sig_bits = 52u;
    static constexpr bits_full_t exp_bits = bits - sig_bits - 1u;

    static constexpr bits_full_t sign_mask = bits_full_t{1u} << (bits - 1u);
    static constexpr bits_full_t sig_mask  = (bits_full_t{1u} << sig_bits) - 1u;
    static constexpr bits_full_t exp_mask  = ((bits_full_t{1u} << (bits - 1u)) - 1u) & ~sig_mask;

    static constexpr int_exp_t exp_origin    = -1023;
    static constexpr int_exp_t exp_subnormal = exp_origin + 1;

    static constexpr bits_exp_t exp_bits_special = 0x7ff;
    static constexpr bits_sig_t sig_bits_nan     = 0x8000000000000;
    static constexpr bits_sig_t sig_bits_inf     = 0x0;

    static constexpr std::size_t precision = 16u;

    static constexpr std::array<unsigned_fixed, sig_bits> sig_elems = {
        {unsigned_fixed(2220446049250313081u, -34), unsigned_fixed(4440892098500626162u, -34),
         unsigned_fixed(8881784197001252323u, -34), unsigned_fixed(1776356839400250465u, -33),
         unsigned_fixed(3552713678800500929u, -33), unsigned_fixed(7105427357601001859u, -33),
         unsigned_fixed(1421085471520200372u, -32), unsigned_fixed(2842170943040400743u, -32),
         unsigned_fixed(5684341886080801487u, -32), unsigned_fixed(1136868377216160297u, -31),
         unsigned_fixed(2273736754432320595u, -31), unsigned_fixed(4547473508864641190u, -31),
         unsigned_fixed(9094947017729282379u, -31), unsigned_fixed(1818989403545856476u, -30),
         unsigned_fixed(3637978807091712952u, -30), unsigned_fixed(7275957614183425903u, -30),
         unsigned_fixed(1455191522836685181u, -29), unsigned_fixed(2910383045673370361u, -29),
         unsigned_fixed(5820766091346740723u, -29), unsigned_fixed(1164153218269348145u, -28),
         unsigned_fixed(2328306436538696289u, -28), unsigned_fixed(4656612873077392578u, -28),
         unsigned_fixed(9313225746154785156u, -28), unsigned_fixed(1862645149230957031u, -27),
         unsigned_fixed(3725290298461914062u, -27), unsigned_fixed(7450580596923828125u, -27),
         unsigned_fixed(1490116119384765625u, -26), unsigned_fixed(2980232238769531250u, -26),
         unsigned_fixed(5960464477539062500u, -26), unsigned_fixed(1192092895507812500u, -25),
         unsigned_fixed(2384185791015625000u, -25), unsigned_fixed(4768371582031250000u, -25),
         unsigned_fixed(9536743164062500000u, -25), unsigned_fixed(1907348632812500000u, -24),
         unsigned_fixed(3814697265625000000u, -24), unsigned_fixed(7629394531250000000u, -24),
         unsigned_fixed(1525878906250000000u, -23), unsigned_fixed(3051757812500000000u, -23),
         unsigned_fixed(6103515625000000000u, -23), unsigned_fixed(1220703125000000000u, -22),
         unsigned_fixed(2441406250000000000u, -22), unsigned_fixed(4882812500000000000u, -22),
         unsigned_fixed(9765625000000000000u, -22), unsigned_fixed(1953125000000000000u, -21),
         unsigned_fixed(3906250000000000000u, -21), unsigned_fixed(7812500000000000000u, -21),
         unsigned_fixed(1562500000000000000u, -20), unsigned_fixed(3125000000000000000u, -20),
         unsigned_fixed(6250000000000000000u, -20), unsigned_fixed(1250000000000000000u, -19),
         unsigned_fixed(2500000000000000000u, -19), unsigned_fixed(5000000000000000000u, -19)}};
};

template<typename T>
struct float_bits {
    using traits = float_traits<T>;

    typename traits::bits_sig_t significand = 0u;
    typename traits::bits_exp_t exponent    = 0u;
    bool                        sign        = 0;
};

template<typename T>
[[nodiscard]] constexpr float_bits<T> to_bits(T f) noexcept {
    using traits      = float_traits<T>;
    using bits_full_t = typename traits::bits_full_t;
    using bits_sig_t  = typename traits::bits_sig_t;
    using bits_exp_t  = typename traits::bits_exp_t;

#if SNITCH_CONSTEXPR_FLOAT_USE_BITCAST

    const bits_full_t bits = std::bit_cast<bits_full_t>(f);

    return float_bits<T>{
        .significand = static_cast<bits_sig_t>(bits & traits::sig_mask),
        .exponent    = static_cast<bits_exp_t>((bits & traits::exp_mask) >> traits::sig_bits),
        .sign        = (bits & traits::sign_mask) != 0u};

#else

    float_bits<T> b;

    if (f != f) {
        // NaN
        b.sign        = false;
        b.exponent    = traits::exp_bits_special;
        b.significand = traits::sig_bits_nan;
    } else if (f == std::numeric_limits<T>::infinity()) {
        // +Inf
        b.sign        = false;
        b.exponent    = traits::exp_bits_special;
        b.significand = traits::sig_bits_inf;
    } else if (f == -std::numeric_limits<T>::infinity()) {
        // -Inf
        b.sign        = true;
        b.exponent    = traits::exp_bits_special;
        b.significand = traits::sig_bits_inf;
    } else {
        // General case
        if (f < static_cast<T>(0.0)) {
            b.sign = true;
            f      = -f;
        }

        b.exponent = static_cast<bits_exp_t>(-traits::exp_origin);

        if (f >= static_cast<T>(2.0)) {
            do {
                f /= static_cast<T>(2.0);
                b.exponent += 1u;
            } while (f >= static_cast<T>(2.0));
        } else if (f < static_cast<T>(1.0)) {
            do {
                f *= static_cast<T>(2.0);
                b.exponent -= 1u;
            } while (f < static_cast<T>(1.0) && b.exponent > 0u);
        }

        if (b.exponent == 0u) {
            // Sub-normals
            f *= static_cast<T>(static_cast<bits_sig_t>(2u) << (traits::sig_bits - 2u));
        } else {
            // Normals
            f *= static_cast<T>(static_cast<bits_sig_t>(2u) << (traits::sig_bits - 1u));
        }

        b.significand = static_cast<bits_sig_t>(static_cast<bits_full_t>(f) & traits::sig_mask);
    }

    return b;

#endif
}

static constexpr unsigned_fixed binary_table[2][10] = {
    {unsigned_fixed(2000000000000000000u, -18), unsigned_fixed(4000000000000000000u, -18),
     unsigned_fixed(1600000000000000000u, -17), unsigned_fixed(2560000000000000000u, -16),
     unsigned_fixed(6553600000000000000u, -14), unsigned_fixed(4294967296000000000u, -9),
     unsigned_fixed(1844674407370955162u, 1), unsigned_fixed(3402823669209384635u, 20),
     unsigned_fixed(1157920892373161954u, 59), unsigned_fixed(1340780792994259710u, 136)},
    {unsigned_fixed(5000000000000000000u, -19), unsigned_fixed(2500000000000000000u, -19),
     unsigned_fixed(6250000000000000000u, -20), unsigned_fixed(3906250000000000000u, -21),
     unsigned_fixed(1525878906250000000u, -23), unsigned_fixed(2328306436538696289u, -28),
     unsigned_fixed(5421010862427522170u, -38), unsigned_fixed(2938735877055718770u, -57),
     unsigned_fixed(8636168555094444625u, -96), unsigned_fixed(7458340731200206743u, -173)}};

template<typename T>
constexpr void apply_binary_exponent(
    unsigned_fixed&                           fix,
    std::size_t                               mul_div,
    typename float_bits<T>::traits::int_exp_t exponent) noexcept {

    using traits    = float_traits<T>;
    using int_exp_t = typename traits::int_exp_t;

    // NB: We skip the last bit of the exponent. One bit was lost to generate the sign.
    // In other words, for float binary32, although the exponent is encoded on 8 bits, the value
    // can range from -126 to +127, hence the maximum absolute value is 127, which fits on 7 bits.
    // NB2: To preserve as much accuracy as possible, we multiply the powers of two together
    // from smallest to largest (since multiplying small powers can be done without any loss of
    // precision), and finally multiply the combined powers to the input number.
    unsigned_fixed power(1, 0);
    for (std::size_t i = 0; i < traits::exp_bits - 1; ++i) {
        if ((exponent & (static_cast<int_exp_t>(1) << i)) != 0u) {
            power *= binary_table[mul_div][i];
        }
    }

    fix *= power;
}

template<typename T>
[[nodiscard]] constexpr signed_fixed_data to_fixed(const float_bits<T>& bits) noexcept {
    using traits     = float_traits<T>;
    using bits_sig_t = typename traits::bits_sig_t;
    using int_exp_t  = typename traits::int_exp_t;

    // NB: To preserve as much accuracy as possible, we accumulate the significand components from
    // smallest to largest.
    unsigned_fixed fix(0, 0);
    for (bits_sig_t i = 0; i < traits::sig_bits; ++i) {
        if ((bits.significand & (static_cast<bits_sig_t>(1u) << i)) != 0u) {
            fix += traits::sig_elems[static_cast<std::size_t>(i)];
        }
    }

    const bool subnormal = bits.exponent == 0x0;

    if (!subnormal) {
        fix += unsigned_fixed(1, 0);
    }

    int_exp_t exponent = subnormal ? traits::exp_subnormal
                                   : static_cast<int_exp_t>(bits.exponent) + traits::exp_origin;

    if (exponent > 0) {
        apply_binary_exponent<T>(fix, 0u, exponent);
    } else if (exponent < 0) {
        apply_binary_exponent<T>(fix, 1u, -exponent);
    }

    return {.digits = fix.digits(), .exponent = fix.exponent(), .sign = bits.sign};
}
} // namespace snitch::impl

// Public utilities: append.
// -------------------------

namespace snitch {
// These types are used to define the largest printable integer types.
// In C++, integer literals must fit on uintmax_t/intmax_t, so these are good candidates.
// They aren't perfect though. On most 64 bit platforms they are defined as 64 bit integers,
// even though those platforms usually support 128 bit integers.
using large_uint_t = std::uintmax_t;
using large_int_t  = std::intmax_t;

static_assert(
    sizeof(large_uint_t) >= sizeof(impl::fixed_digits_t),
    "large_uint_t is too small to support the float-to-fixed-point conversion implementation");
} // namespace snitch

namespace snitch::impl {
[[nodiscard]] bool append_fast(small_string_span ss, std::string_view str) noexcept;
[[nodiscard]] bool append_fast(small_string_span ss, const void* ptr) noexcept;
[[nodiscard]] bool append_fast(small_string_span ss, large_uint_t i) noexcept;
[[nodiscard]] bool append_fast(small_string_span ss, large_int_t i) noexcept;
[[nodiscard]] bool append_fast(small_string_span ss, float f) noexcept;
[[nodiscard]] bool append_fast(small_string_span ss, double f) noexcept;

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, std::string_view str) noexcept {
    const bool        could_fit  = str.size() <= ss.available();
    const std::size_t copy_count = std::min(str.size(), ss.available());

    const std::size_t offset = ss.size();
    ss.grow(copy_count);
    for (std::size_t i = 0; i < copy_count; ++i) {
        ss[offset + i] = str[i];
    }

    return could_fit;
}

[[nodiscard]] constexpr std::size_t num_digits(large_uint_t x) noexcept {
    return x >= 10u ? 1u + num_digits(x / 10u) : 1u;
}

[[nodiscard]] constexpr std::size_t num_digits(large_int_t x) noexcept {
    return x >= 10 ? 1u + num_digits(x / 10) : x <= -10 ? 1u + num_digits(x / 10) : x > 0 ? 1u : 2u;
}

constexpr std::array<char, 10> digits = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
constexpr std::size_t max_uint_length = num_digits(std::numeric_limits<large_uint_t>::max());
constexpr std::size_t max_int_length  = max_uint_length + 1;

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, large_uint_t i) noexcept {
    if (i != 0u) {
        small_string<max_uint_length> tmp;
        tmp.resize(num_digits(i));
        std::size_t k = 1;
        for (large_uint_t j = i; j != 0u; j /= 10u, ++k) {
            tmp[tmp.size() - k] = digits[j % 10u];
        }
        return append_constexpr(ss, tmp);
    } else {
        return append_constexpr(ss, "0");
    }
}

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, large_int_t i) noexcept {
    if (i > 0) {
        small_string<max_int_length> tmp;
        tmp.resize(num_digits(i));
        std::size_t k = 1;
        for (large_int_t j = i; j != 0; j /= 10, ++k) {
            tmp[tmp.size() - k] = digits[j % 10];
        }
        return append_constexpr(ss, tmp);
    } else if (i < 0) {
        small_string<max_int_length> tmp;
        tmp.resize(num_digits(i));
        std::size_t k = 1;
        for (large_int_t j = i; j != 0; j /= 10, ++k) {
            tmp[tmp.size() - k] = digits[-(j % 10)];
        }
        tmp[0] = '-';
        return append_constexpr(ss, tmp);
    } else {
        return append_constexpr(ss, "0");
    }
}

// Minimum number of digits in the exponent, set to 2 to match std::printf.
constexpr std::size_t min_exp_digits = 2u;

[[nodiscard]] constexpr std::size_t num_exp_digits(fixed_exp_t x) noexcept {
    const std::size_t exp_digits = num_digits(static_cast<large_uint_t>(x > 0 ? x : -x));
    return exp_digits < min_exp_digits ? min_exp_digits : exp_digits;
}

[[nodiscard]] constexpr std::size_t num_digits(const signed_fixed_data& x) noexcept {
    // +1 for fractional separator '.'
    // +1 for exponent separator 'e'
    // +1 for exponent sign
    return num_digits(static_cast<large_uint_t>(x.digits)) + num_exp_digits(x.exponent) +
           (x.sign ? 1u : 0u) + 3u;
}

constexpr std::size_t max_float_length = num_digits(signed_fixed_data{
    .digits   = std::numeric_limits<fixed_digits_t>::max(),
    .exponent = float_traits<double>::exp_origin,
    .sign     = true});

[[nodiscard]] constexpr fixed_digits_t
round_half_to_even(fixed_digits_t i, bool only_zero) noexcept {
    fixed_digits_t r = (i + 5u) / 10u;
    if (only_zero && i % 10u == 5u) {
        // Exact tie detected, correct the rounded value to the nearest even integer.
        r -= 1u - (i / 10u) % 2u;
    }
    return r;
}

[[nodiscard]] constexpr signed_fixed_data
set_precision(signed_fixed_data fd, std::size_t p) noexcept {
    // Truncate the digits of the input to the chosen precision (number of digits on both
    // sides of the decimal point). Precision must be less or equal to 19.
    // We have a choice of the rounding mode here; to stay as close as possible to the
    // std::printf() behavior, we use round-half-to-even (i.e., round to nearest, and break ties
    // to nearest even integer). std::printf() is supposed to follow the current rounding mode,
    // and round-half-to-even is the default rounding mode for IEEE 754 floats. We don't follow
    // the current rounding mode, but we can at least follow the default.

    std::size_t base_digits = num_digits(static_cast<large_uint_t>(fd.digits));

    bool only_zero = true;
    while (base_digits > p) {
        if (base_digits > p + 1u) {
            if (fd.digits % 10u > 0u) {
                only_zero = false;
            }
            fd.digits = fd.digits / 10u;
        } else {
            fd.digits = round_half_to_even(fd.digits, only_zero);
        }

        fd.exponent += 1;
        base_digits -= 1u;
    }

    return fd;
}

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, signed_fixed_data fd) noexcept {
    // Statically allocate enough space for the biggest float,
    // then resize to the length of this particular float.
    small_string<max_float_length> tmp;
    tmp.resize(num_digits(fd));

    const std::size_t exp_digits = num_exp_digits(fd.exponent);

    // The exponent has a unsigned_fixed size, so we can start by writing the main digits.
    // We write the digits with always a single digit before the decimal separator,
    // and the rest as fractional part. This will require adjusting the value of
    // the exponent later.
    std::size_t k            = 3u + exp_digits;
    fixed_exp_t exponent_add = 0;
    for (fixed_digits_t j = fd.digits; j != 0u; j /= 10u, ++k, ++exponent_add) {
        if (j < 10u) {
            tmp[tmp.size() - k] = '.';
            ++k;
        }
        tmp[tmp.size() - k] = digits[j % 10u];
    }

    // Add a negative sign for negative floats.
    if (fd.sign) {
        tmp[0] = '-';
    }

    // Now write the exponent, adjusted for the chosen display (one digit before the decimal
    // separator).
    const fixed_exp_t exponent = fd.exponent + exponent_add - 1;

    k = 1;
    for (fixed_exp_t j = exponent > 0 ? exponent : -exponent; j != 0; j /= 10, ++k) {
        tmp[tmp.size() - k] = digits[j % 10];
    }

    // Pad exponent with zeros if it is shorter than the min number of digits.
    for (; k <= min_exp_digits; ++k) {
        tmp[tmp.size() - k] = '0';
    }

    // Write the sign, and exponent delimitation character.
    tmp[tmp.size() - k] = exponent >= 0 ? '+' : '-';
    ++k;
    tmp[tmp.size() - k] = 'e';
    ++k;

    // Finally write as much of the string as we can to the chosen destination.
    return append_constexpr(ss, tmp);
}

template<floating_point T>
[[nodiscard]] constexpr bool append_constexpr(
    small_string_span ss, T f, std::size_t precision = float_traits<T>::precision) noexcept {
    if constexpr (std::numeric_limits<T>::is_iec559) {
        using traits = float_traits<T>;

        // Float/double precision cannot be greater than 19 digits.
        precision = precision <= 19u ? precision : 19u;

        const float_bits<T> bits = to_bits(f);

        // Handle special cases.
        if (bits.exponent == 0x0) {
            if (bits.significand == 0x0) {
                // Zero.
                constexpr std::string_view zeros = "000000000000000000";
                return append_constexpr(ss, bits.sign ? "-0." : "0.") &&
                       append_constexpr(ss, zeros.substr(0, precision - 1)) &&
                       append_constexpr(ss, "e+00");
            } else {
                // Subnormals.
                return append_constexpr(ss, set_precision(to_fixed(bits), precision));
            }
        } else if (bits.exponent == traits::exp_bits_special) {
            if (bits.significand == traits::sig_bits_inf) {
                // Infinity.
                constexpr std::string_view plus_inf_str  = "inf";
                constexpr std::string_view minus_inf_str = "-inf";
                return bits.sign ? append_constexpr(ss, minus_inf_str)
                                 : append_constexpr(ss, plus_inf_str);
            } else {
                // NaN.
                constexpr std::string_view nan_str = "nan";
                return append_constexpr(ss, nan_str);
            }
        } else {
            // Normal number.
            return append_constexpr(ss, set_precision(to_fixed(bits), precision));
        }
    } else {
        constexpr std::string_view unknown_str = "?";
        return append_constexpr(ss, unknown_str);
    }
}

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, const void* p) noexcept {
    if (p == nullptr) {
        constexpr std::string_view nullptr_str = "nullptr";
        return append_constexpr(ss, nullptr_str);
    } else {
        constexpr std::string_view unknown_ptr_str = "0x????????";
        return append_constexpr(ss, unknown_ptr_str);
    }
}
} // namespace snitch::impl

namespace snitch {
[[nodiscard]] constexpr bool append(small_string_span ss, std::string_view str) noexcept {
    if (std::is_constant_evaluated()) {
        return impl::append_constexpr(ss, str);
    } else {
        return impl::append_fast(ss, str);
    }
}

[[nodiscard]] constexpr bool append(small_string_span ss, const void* ptr) noexcept {
    if (std::is_constant_evaluated()) {
        return impl::append_constexpr(ss, ptr);
    } else {
        return impl::append_fast(ss, ptr);
    }
}

[[nodiscard]] constexpr bool append(small_string_span ss, std::nullptr_t) noexcept {
    constexpr std::string_view nullptr_str = "nullptr";
    return append(ss, nullptr_str);
}

[[nodiscard]] constexpr bool append(small_string_span ss, large_uint_t i) noexcept {
    if (std::is_constant_evaluated()) {
        return impl::append_constexpr(ss, i);
    } else {
        return impl::append_fast(ss, i);
    }
}
[[nodiscard]] constexpr bool append(small_string_span ss, large_int_t i) noexcept {
    if (std::is_constant_evaluated()) {
        return impl::append_constexpr(ss, i);
    } else {
        return impl::append_fast(ss, i);
    }
}

[[nodiscard]] constexpr bool append(small_string_span ss, float f) noexcept {
    if (std::is_constant_evaluated()) {
        return impl::append_constexpr(ss, f);
    } else {
        return impl::append_fast(ss, f);
    }
}

[[nodiscard]] constexpr bool append(small_string_span ss, double f) noexcept {
    if (std::is_constant_evaluated()) {
        return impl::append_constexpr(ss, f);
    } else {
        return impl::append_fast(ss, f);
    }
}

[[nodiscard]] constexpr bool append(small_string_span ss, bool value) noexcept {
    constexpr std::string_view true_str  = "true";
    constexpr std::string_view false_str = "false";
    return append(ss, value ? true_str : false_str);
}

template<typename T>
[[nodiscard]] constexpr bool append(small_string_span ss, T* ptr) noexcept {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        return append(ss, std::string_view(ptr));
    } else if constexpr (std::is_function_v<T>) {
        if (ptr != nullptr) {
            constexpr std::string_view function_ptr_str = "0x????????";
            return append(ss, function_ptr_str);
        } else {
            return append(ss, nullptr);
        }
    } else {
        return append(ss, static_cast<const void*>(ptr));
    }
}

template<std::size_t N>
[[nodiscard]] constexpr bool append(small_string_span ss, const char str[N]) noexcept {
    return append(ss, std::string_view(str));
}

template<signed_integral T>
[[nodiscard]] constexpr bool append(small_string_span ss, T value) noexcept {
    return append(ss, static_cast<large_int_t>(value));
}

template<unsigned_integral T>
[[nodiscard]] constexpr bool append(small_string_span ss, T value) noexcept {
    return append(ss, static_cast<large_uint_t>(value));
}

template<enumeration T>
[[nodiscard]] constexpr bool append(small_string_span ss, T value) noexcept {
    return append(ss, static_cast<std::underlying_type_t<T>>(value));
}

template<convertible_to<std::string_view> T>
[[nodiscard]] constexpr bool append(small_string_span ss, const T& value) noexcept {
    return append(ss, std::string_view(value));
}

template<typename T>
concept string_appendable = requires(small_string_span ss, T value) { append(ss, value); };

template<string_appendable T, string_appendable U, string_appendable... Args>
[[nodiscard]] constexpr bool append(small_string_span ss, T&& t, U&& u, Args&&... args) noexcept {
    return append(ss, std::forward<T>(t)) && append(ss, std::forward<U>(u)) &&
           (append(ss, std::forward<Args>(args)) && ...);
}
} // namespace snitch

// Public utilities: string utilities.
// -----------------------------------

namespace snitch {
constexpr void truncate_end(small_string_span ss) noexcept {
    std::size_t num_dots     = 3;
    std::size_t final_length = ss.size() + num_dots;
    if (final_length > ss.capacity()) {
        final_length = ss.capacity();
    }

    const std::size_t offset = final_length >= num_dots ? final_length - num_dots : 0;
    num_dots                 = final_length - offset;

    ss.resize(final_length);
    for (std::size_t i = 0; i < num_dots; ++i) {
        ss[offset + i] = '.';
    }
}

template<string_appendable... Args>
constexpr bool append_or_truncate(small_string_span ss, Args&&... args) noexcept {
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

void set_current_test(test_state* current) noexcept;

struct section_entry_checker {
    section_id  section = {};
    test_state& state;
    bool        entered = false;

    ~section_entry_checker();

    // Requires: number of sections < max_nested_sections.
    explicit operator bool();
};

#define DEFINE_OPERATOR(OP, NAME, DISP, DISP_INV)                                                  \
    struct operator_##NAME {                                                                       \
        static constexpr std::string_view actual  = DISP;                                          \
        static constexpr std::string_view inverse = DISP_INV;                                      \
                                                                                                   \
        template<typename T, typename U>                                                           \
        constexpr bool operator()(const T& lhs, const U& rhs) const noexcept(noexcept(lhs OP rhs)) \
            requires(requires(const T& lhs, const U& rhs) { lhs OP rhs; })                         \
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
    bool                          success  = true;

    template<string_appendable T>
    [[nodiscard]] constexpr bool append_value(T&& value) noexcept {
        return append(actual, std::forward<T>(value));
    }

    template<typename T>
    [[nodiscard]] constexpr bool append_value(T&&) noexcept {
        constexpr std::string_view unknown_value = "?";
        return append(actual, unknown_value);
    }
};

struct nondecomposable_expression : expression {};

struct invalid_expression {
    // This is an invalid expression; any further operator should produce another invalid
    // expression. We don't want to decompose these operators, but we need to declare them
    // so the expression compiles until calling to_expression(). This enable conditional
    // decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    constexpr invalid_expression operator OP(const V&) noexcept {                                  \
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

    constexpr nondecomposable_expression to_expression() const noexcept {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression. But the code will be instantiated in
        // constexpr expressions, so don't static_assert.
        return nondecomposable_expression{};
    }
};

template<bool Expected, typename T, typename O, typename U>
struct extracted_binary_expression {
    std::string_view expected;
    const T&         lhs;
    const U&         rhs;

    // This is a binary expression; any further operator should produce an invalid
    // expression, since we can't/won't decompose complex expressions. We don't want to decompose
    // these operators, but we need to declare them so the expression compiles until cast to bool.
    // This enable conditional decomposition.
#define EXPR_OPERATOR_INVALID(OP)                                                                  \
    template<typename V>                                                                           \
    constexpr invalid_expression operator OP(const V&) noexcept {                                  \
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

#define EXPR_COMMA ,
    EXPR_OPERATOR_INVALID(EXPR_COMMA)
#undef EXPR_COMMA

#undef EXPR_OPERATOR_INVALID

    // NB: Cannot make this noexcept since user operators may throw.
    constexpr expression to_expression() const noexcept(noexcept(static_cast<bool>(O{}(lhs, rhs))))
        requires(requires(const T& lhs, const U& rhs) { O{}(lhs, rhs); })
    {
        expression expr{expected};

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

            expr.success = false;
        } else {
            expr.success = true;
        }

        return expr;
    }

    constexpr nondecomposable_expression to_expression() const noexcept
        requires(!requires(const T& lhs, const U& rhs) { O{}(lhs, rhs); })
    {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression. But the code will be instantiated in
        // constexpr expressions, so don't static_assert.
        return nondecomposable_expression{};
    }
};

template<bool Expected, typename T>
struct extracted_unary_expression {
    std::string_view expected;
    const T&         lhs;

    // Operators we want to decompose.
#define EXPR_OPERATOR(OP, OP_TYPE)                                                                 \
    template<typename U>                                                                           \
    constexpr extracted_binary_expression<Expected, T, OP_TYPE, U> operator OP(const U& rhs)       \
        const noexcept {                                                                           \
        return {expected, lhs, rhs};                                                               \
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
    constexpr invalid_expression operator OP(const V&) noexcept {                                  \
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

#define EXPR_COMMA ,
    EXPR_OPERATOR_INVALID(EXPR_COMMA)
#undef EXPR_COMMA

#undef EXPR_OPERATOR_INVALID

    constexpr expression to_expression() const noexcept(noexcept(static_cast<bool>(lhs)))
        requires(requires(const T& lhs) { static_cast<bool>(lhs); })
    {
        expression expr{expected};

        if (static_cast<bool>(lhs) != Expected) {
            if (!expr.append_value(lhs)) {
                expr.actual.clear();
            }

            expr.success = false;
        } else {
            expr.success = true;
        }

        return expr;
    }

    constexpr nondecomposable_expression to_expression() const noexcept
        requires(!requires(const T& lhs) { static_cast<bool>(lhs); })
    {
        // This should be unreachable, because we check if an expression is decomposable
        // before calling the decomposed expression. But the code will be instantiated in
        // constexpr expressions, so don't static_assert.
        return nondecomposable_expression{};
    }
};

template<bool Expected>
struct expression_extractor {
    std::string_view expected;

    template<typename T>
    constexpr extracted_unary_expression<Expected, T> operator<=(const T& lhs) const noexcept {
        return {expected, lhs};
    }
};

template<typename T>
constexpr bool is_decomposable = !std::is_same_v<T, nondecomposable_expression>;

struct scoped_capture {
    capture_state& captures;
    std::size_t    count = 0;

    ~scoped_capture() {
        captures.resize(captures.size() - count);
    }
};

std::string_view extract_next_name(std::string_view& names) noexcept;

// Requires: number of captures < max_captures.
small_string<max_capture_length>& add_capture(test_state& state);

// Requires: number of captures < max_captures.
template<string_appendable T>
void add_capture(test_state& state, std::string_view& names, const T& arg) {
    auto& capture = add_capture(state);
    append_or_truncate(capture, extract_next_name(names), " := ", arg);
}

// Requires: number of captures < max_captures.
template<string_appendable... Args>
scoped_capture add_captures(test_state& state, std::string_view names, const Args&... args) {
    (add_capture(state, names, args), ...);
    return {state.captures, sizeof...(args)};
}

// Requires: number of captures < max_captures.
template<string_appendable... Args>
scoped_capture add_info(test_state& state, const Args&... args) {
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
} // namespace event
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

namespace snitch::impl {
void default_reporter(const registry& r, const event::data& event) noexcept;
}

namespace snitch {
class registry {
    small_vector<impl::test_case, max_test_cases> test_list;

public:
    enum class verbosity { quiet, normal, high } verbose = verbosity::normal;
    bool with_color                                      = true;

    using print_function  = small_function<void(std::string_view) noexcept>;
    using report_function = small_function<void(const registry&, const event::data&) noexcept>;

    print_function  print_callback  = &snitch::impl::stdout_print;
    report_function report_callback = &snitch::impl::default_reporter;

    template<typename... Args>
    void print(Args&&... args) const noexcept {
        small_string<max_message_length> message;
        const bool                       could_fit = append(message, std::forward<Args>(args)...);
        this->print_callback(message);
        if (!could_fit) {
            this->print_callback("...");
        }
    }

    // Requires: number of tests + 1 <= max_test_cases, well-formed test ID.
    const char* add(const test_id& id, impl::test_ptr func);

    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename... Args, typename F>
    const char* add_with_types(std::string_view name, std::string_view tags, const F& func) {
        return (
            add({name, tags, impl::get_type_name<Args>()}, impl::to_test_case_ptr<Args>(func)),
            ...);
    }

    // Requires: number of tests + added tests <= max_test_cases, well-formed test ID.
    template<typename T, typename F>
    const char* add_with_type_list(std::string_view name, std::string_view tags, const F& func) {
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

    // Requires: number unique tags <= max_unique_tags.
    void list_all_tags() const;

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

namespace snitch::impl {
template<typename T, typename M>
[[nodiscard]] constexpr auto constexpr_match(T&& value, M&& matcher) noexcept {
    using result_type = decltype(matcher.describe_match(value, matchers::match_status::failed));
    if (!matcher.match(value)) {
        return std::optional<result_type>(
            matcher.describe_match(value, matchers::match_status::failed));
    } else {
        return std::optional<result_type>{};
    }
}
} // namespace snitch::impl

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

#define SNITCH_EXPR_IS_FALSE(TYPE, ...)                                                            \
    auto SNITCH_CURRENT_EXPRESSION =                                                               \
        (snitch::impl::expression_extractor<true>{TYPE "(" #__VA_ARGS__ ")"} <= __VA_ARGS__)       \
            .to_expression();                                                                      \
    !SNITCH_CURRENT_EXPRESSION.success

#define SNITCH_EXPR_IS_TRUE(TYPE, ...)                                                             \
    auto SNITCH_CURRENT_EXPRESSION =                                                               \
        (snitch::impl::expression_extractor<false>{TYPE "(" #__VA_ARGS__ ")"} <= __VA_ARGS__)      \
            .to_expression();                                                                      \
    !SNITCH_CURRENT_EXPRESSION.success

#define SNITCH_IS_DECOMPOSABLE(...)                                                                \
    snitch::impl::is_decomposable<decltype((snitch::impl::expression_extractor<true>{              \
                                                std::declval<std::string_view>()} <= __VA_ARGS__)  \
                                               .to_expression())>

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
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_FALSE("REQUIRE", __VA_ARGS__)) {                                    \
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
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_FALSE("CHECK", __VA_ARGS__)) {                                      \
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
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_TRUE("REQUIRE_FALSE", __VA_ARGS__)) {                               \
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
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if (SNITCH_EXPR_IS_TRUE("CHECK_FALSE", __VA_ARGS__)) {                                 \
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
                "REQUIRE_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                                 \
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
                "CHECK_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                                   \
                SNITCH_TEMP_MATCHER.describe_match(                                                \
                    SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));                   \
        }                                                                                          \
    } while (0)

#define SNITCH_CONSTEVAL_REQUIRE(...)                                                              \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE("CONSTEVAL_REQUIRE", __VA_ARGS__)) {      \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_REQUIRE(" #__VA_ARGS__ ")");                                        \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_CHECK(...)                                                                \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE("CONSTEVAL_CHECK", __VA_ARGS__)) {        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_CHECK(" #__VA_ARGS__ ")");                                          \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_REQUIRE_FALSE(...)                                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE("CONSTEVAL_REQUIRE_FALSE", __VA_ARGS__)) { \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_REQUIRE_FALSE(" #__VA_ARGS__ ")");                                  \
                SNITCH_TESTING_ABORT;                                                              \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_CHECK_FALSE(...)                                                          \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE("CONSTEVAL_CHECK_FALSE", __VA_ARGS__)) {   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEVAL_CHECK_FALSE(" #__VA_ARGS__ ")");                                    \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEVAL_REQUIRE_THAT(EXPR, ...)                                                   \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEVAL_REQUIRE_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                       \
                SNITCH_TEMP_ERROR.value());                                                        \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
    } while (0)

#define SNITCH_CONSTEVAL_CHECK_THAT(EXPR, ...)                                                     \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        ++SNITCH_CURRENT_TEST.asserts;                                                             \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEVAL_CHECK_THAT(" #EXPR ", " #__VA_ARGS__ "), got ",                         \
                SNITCH_TEMP_ERROR.value());                                                        \
        }                                                                                          \
    } while (0)

#define SNITCH_CONSTEXPR_REQUIRE(...)                                                              \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        bool SNITCH_CURRENT_ASSERTION_FAILED = false;                                              \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE(                                          \
                              "CONSTEXPR_REQUIRE[compile-time]", __VA_ARGS__)) {                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
            if (SNITCH_EXPR_IS_FALSE("CONSTEXPR_REQUIRE[run-time]", __VA_ARGS__)) {                \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE[compile-time](" #__VA_ARGS__ ")");                          \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE[run-time](" #__VA_ARGS__ ")");                              \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
        }                                                                                          \
        if (SNITCH_CURRENT_ASSERTION_FAILED) {                                                     \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_CHECK(...)                                                                \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_FALSE(                                          \
                              "CONSTEXPR_CHECK[compile-time]", __VA_ARGS__)) {                     \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
            if (SNITCH_EXPR_IS_FALSE("CONSTEXPR_CHECK[run-time]", __VA_ARGS__)) {                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (!(__VA_ARGS__)) {                                                        \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK[compile-time](" #__VA_ARGS__ ")");                            \
            }                                                                                      \
            if (!(__VA_ARGS__)) {                                                                  \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK[run-time](" #__VA_ARGS__ ")");                                \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_REQUIRE_FALSE(...)                                                        \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        bool SNITCH_CURRENT_ASSERTION_FAILED = false;                                              \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE(                                           \
                              "CONSTEXPR_REQUIRE_FALSE[compile-time]", __VA_ARGS__)) {             \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
            if (SNITCH_EXPR_IS_TRUE("CONSTEXPR_REQUIRE_FALSE[run-time]", __VA_ARGS__)) {           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_FALSE[compile-time](" #__VA_ARGS__ ")");                    \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
            if (__VA_ARGS__) {                                                                     \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_FALSE[run-time](" #__VA_ARGS__ ")");                        \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
        }                                                                                          \
        if (SNITCH_CURRENT_ASSERTION_FAILED) {                                                     \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_CHECK_FALSE(...)                                                          \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        SNITCH_WARNING_PUSH                                                                        \
        SNITCH_WARNING_DISABLE_PARENTHESES                                                         \
        SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON                                                 \
        if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) {                                       \
            if constexpr (constexpr SNITCH_EXPR_IS_TRUE(                                           \
                              "CONSTEXPR_CHECK_FALSE[compile-time]", __VA_ARGS__)) {               \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
            if (SNITCH_EXPR_IS_TRUE("CONSTEXPR_CHECK_FALSE[run-time]", __VA_ARGS__)) {             \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__}, SNITCH_CURRENT_EXPRESSION);         \
            }                                                                                      \
        } else {                                                                                   \
            if constexpr (__VA_ARGS__) {                                                           \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_FALSE[compile-time](" #__VA_ARGS__ ")");                      \
            }                                                                                      \
            if (__VA_ARGS__) {                                                                     \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_FALSE[run-time](" #__VA_ARGS__ ")");                          \
            }                                                                                      \
        }                                                                                          \
        SNITCH_WARNING_POP                                                                         \
    } while (0)

#define SNITCH_CONSTEXPR_REQUIRE_THAT(EXPR, ...)                                                   \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        bool SNITCH_CURRENT_ASSERTION_FAILED = false;                                              \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEXPR_REQUIRE_THAT[compile-time](" #EXPR ", " #__VA_ARGS__ "), got ",         \
                SNITCH_TEMP_ERROR.value());                                                        \
            SNITCH_CURRENT_ASSERTION_FAILED = true;                                                \
        }                                                                                          \
        {                                                                                          \
            auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                   \
            auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                              \
            if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_REQUIRE_THAT[run-time](" #EXPR ", " #__VA_ARGS__ "), got ",         \
                    SNITCH_TEMP_MATCHER.describe_match(                                            \
                        SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));               \
                SNITCH_CURRENT_ASSERTION_FAILED = true;                                            \
            }                                                                                      \
        }                                                                                          \
        if (SNITCH_CURRENT_ASSERTION_FAILED) {                                                     \
            SNITCH_TESTING_ABORT;                                                                  \
        }                                                                                          \
    } while (0)

#define SNITCH_CONSTEXPR_CHECK_THAT(EXPR, ...)                                                     \
    do {                                                                                           \
        auto& SNITCH_CURRENT_TEST = snitch::impl::get_current_test();                              \
        SNITCH_CURRENT_TEST.asserts += 2u;                                                         \
        if constexpr (constexpr auto SNITCH_TEMP_ERROR =                                           \
                          snitch::impl::constexpr_match(EXPR, __VA_ARGS__);                        \
                      SNITCH_TEMP_ERROR.has_value()) {                                             \
            SNITCH_CURRENT_TEST.reg.report_failure(                                                \
                SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                         \
                "CONSTEXPR_CHECK_THAT[compile-time](" #EXPR ", " #__VA_ARGS__ "), got ",           \
                SNITCH_TEMP_ERROR.value());                                                        \
        }                                                                                          \
        {                                                                                          \
            auto&& SNITCH_TEMP_VALUE   = (EXPR);                                                   \
            auto&& SNITCH_TEMP_MATCHER = __VA_ARGS__;                                              \
            if (!SNITCH_TEMP_MATCHER.match(SNITCH_TEMP_VALUE)) {                                   \
                SNITCH_CURRENT_TEST.reg.report_failure(                                            \
                    SNITCH_CURRENT_TEST, {__FILE__, __LINE__},                                     \
                    "CONSTEXPR_CHECK_THAT[run-time](" #EXPR ", " #__VA_ARGS__ "), got ",           \
                    SNITCH_TEMP_MATCHER.describe_match(                                            \
                        SNITCH_TEMP_VALUE, snitch::matchers::match_status::failed));               \
            }                                                                                      \
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
#    define CAPTURE(...)       SNITCH_CAPTURE(__VA_ARGS__)
#    define INFO(...)          SNITCH_INFO(__VA_ARGS__)

#    define FAIL(MESSAGE)       SNITCH_FAIL(MESSAGE)
#    define FAIL_CHECK(MESSAGE) SNITCH_FAIL_CHECK(MESSAGE)
#    define SKIP(MESSAGE)       SNITCH_SKIP(MESSAGE)

#    define REQUIRE(...)           SNITCH_REQUIRE(__VA_ARGS__)
#    define CHECK(...)             SNITCH_CHECK(__VA_ARGS__)
#    define REQUIRE_FALSE(...)     SNITCH_REQUIRE_FALSE(__VA_ARGS__)
#    define CHECK_FALSE(...)       SNITCH_CHECK_FALSE(__VA_ARGS__)
#    define REQUIRE_THAT(EXP, ...) SNITCH_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CHECK_THAT(EXP, ...)   SNITCH_CHECK_THAT(EXP, __VA_ARGS__)

#    define CONSTEVAL_REQUIRE(...)           SNITCH_CONSTEVAL_REQUIRE(__VA_ARGS__)
#    define CONSTEVAL_CHECK(...)             SNITCH_CONSTEVAL_CHECK(__VA_ARGS__)
#    define CONSTEVAL_REQUIRE_FALSE(...)     SNITCH_CONSTEVAL_REQUIRE_FALSE(__VA_ARGS__)
#    define CONSTEVAL_CHECK_FALSE(...)       SNITCH_CONSTEVAL_CHECK_FALSE(__VA_ARGS__)
#    define CONSTEVAL_REQUIRE_THAT(EXP, ...) SNITCH_CONSTEVAL_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CONSTEVAL_CHECK_THAT(EXP, ...)   SNITCH_CONSTEVAL_CHECK_THAT(EXP, __VA_ARGS__)

#    define CONSTEXPR_REQUIRE(...)           SNITCH_CONSTEXPR_REQUIRE(__VA_ARGS__)
#    define CONSTEXPR_CHECK(...)             SNITCH_CONSTEXPR_CHECK(__VA_ARGS__)
#    define CONSTEXPR_REQUIRE_FALSE(...)     SNITCH_CONSTEXPR_REQUIRE_FALSE(__VA_ARGS__)
#    define CONSTEXPR_CHECK_FALSE(...)       SNITCH_CONSTEXPR_CHECK_FALSE(__VA_ARGS__)
#    define CONSTEXPR_REQUIRE_THAT(EXP, ...) SNITCH_CONSTEXPR_REQUIRE_THAT(EXP, __VA_ARGS__)
#    define CONSTEXPR_CHECK_THAT(EXP, ...)   SNITCH_CONSTEXPR_CHECK_THAT(EXP, __VA_ARGS__)
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
