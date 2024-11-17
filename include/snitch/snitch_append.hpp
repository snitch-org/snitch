#ifndef SNITCH_APPEND_HPP
#define SNITCH_APPEND_HPP

#include "snitch/snitch_concepts.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_fixed_point.hpp"
#include "snitch/snitch_string.hpp"

#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>

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
SNITCH_EXPORT [[nodiscard]] bool append_fast(small_string_span ss, std::string_view str) noexcept;
SNITCH_EXPORT [[nodiscard]] bool append_fast(small_string_span ss, const void* ptr) noexcept;
SNITCH_EXPORT [[nodiscard]] bool append_fast(small_string_span ss, large_uint_t i) noexcept;
SNITCH_EXPORT [[nodiscard]] bool append_fast(small_string_span ss, large_int_t i) noexcept;
SNITCH_EXPORT [[nodiscard]] bool append_fast(small_string_span ss, float f) noexcept;
SNITCH_EXPORT [[nodiscard]] bool append_fast(small_string_span ss, double f) noexcept;

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, std::string_view str) noexcept {
    const bool        could_fit  = str.size() <= ss.available();
    const std::size_t copy_count = could_fit ? str.size() : ss.available();

    const std::size_t offset = ss.size();
    ss.grow(copy_count);
    for (std::size_t i = 0; i < copy_count; ++i) {
        ss[offset + i] = str[i];
    }

    return could_fit;
}

template<large_uint_t Base = 10u, unsigned_integral T>
[[nodiscard]] constexpr std::size_t num_digits(T x) noexcept {
    return x >= Base ? 1u + num_digits<Base>(x / Base) : 1u;
}

template<large_int_t Base = 10, signed_integral T>
[[nodiscard]] constexpr std::size_t num_digits(T x) noexcept {
    return (x >= Base || x <= -Base) ? 1u + num_digits<Base>(x / Base) : x > 0 ? 1u : 2u;
}

constexpr std::array<char, 16> digits = {'0', '1', '2', '3', '4', '5', '6', '7',
                                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

constexpr std::size_t max_uint_length = num_digits(std::numeric_limits<large_uint_t>::max());
constexpr std::size_t max_int_length  = max_uint_length + 1;

template<large_uint_t Base = 10u, unsigned_integral T>
[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, T i) noexcept {
    if (i != 0u) {
        small_string<max_uint_length> tmp;
        tmp.resize(num_digits<Base>(i));
        std::size_t k = 1;
        for (large_uint_t j = i; j != 0u; j /= Base, ++k) {
            tmp[tmp.size() - k] = digits[j % Base];
        }
        return append_constexpr(ss, tmp);
    } else {
        return append_constexpr(ss, "0");
    }
}

template<large_int_t Base = 10, signed_integral T>
[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, T i) noexcept {
    if (i > 0) {
        small_string<max_int_length> tmp;
        tmp.resize(num_digits<Base>(i));
        std::size_t k = 1;
        for (large_int_t j = i; j != 0; j /= Base, ++k) {
            tmp[tmp.size() - k] = digits[j % Base];
        }
        return append_constexpr(ss, tmp);
    } else if (i < 0) {
        small_string<max_int_length> tmp;
        tmp.resize(num_digits<Base>(i));
        std::size_t k = 1;
        for (large_int_t j = i; j != 0; j /= Base, ++k) {
            tmp[tmp.size() - k] = digits[-(j % Base)];
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
    const std::size_t exp_digits = num_digits<10>(static_cast<large_uint_t>(x > 0 ? x : -x));
    return exp_digits < min_exp_digits ? min_exp_digits : exp_digits;
}

[[nodiscard]] constexpr std::size_t num_digits(const signed_fixed_data& x) noexcept {
    // Don't forget to modify the stored exponent by the number of stored digits, since we always
    // print floating point numbers as 1.23456 but store them as 123456.
    // Why +3:
    // +1 for fractional separator '.'
    // +1 for exponent separator 'e'
    // +1 for exponent sign
    const std::size_t stored_digits = num_digits<10>(static_cast<large_uint_t>(x.digits));
    return stored_digits + (x.sign ? 1u : 0u) +
           num_exp_digits(static_cast<fixed_exp_t>(x.exponent + stored_digits - 1)) + 3u;
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

    std::size_t base_digits = num_digits<10>(static_cast<large_uint_t>(fd.digits));

    bool only_zero = true;
    while (base_digits > p) {
        if (base_digits > p + 1u) {
            if (fd.digits % 10u > 0u) {
                only_zero = false;
            }
            fd.digits = fd.digits / 10u;
            base_digits -= 1u;
        } else {
            fd.digits   = round_half_to_even(fd.digits, only_zero);
            base_digits = num_digits<10>(static_cast<large_uint_t>(fd.digits));
        }

        fd.exponent += 1;
    }

    return fd;
}

[[nodiscard]] constexpr bool append_constexpr(small_string_span ss, signed_fixed_data fd) noexcept {
    // Statically allocate enough space for the biggest float,
    small_string<max_float_length> tmp;

    // Resize to fit the digits (without exponent part).
    // +1 for fractional separator '.'
    // +1 for sign
    const std::size_t stored_digits = num_digits<10>(static_cast<large_uint_t>(fd.digits));
    tmp.resize(stored_digits + 1u + (fd.sign ? 1u : 0u));

    // The exponent has a fixed size, so we can start by writing the main digits.
    // We write the digits with always a single digit before the decimal separator,
    // and the rest as fractional part. This will require adjusting the value of
    // the exponent later.
    std::size_t k = 1u;
    for (fixed_digits_t j = fd.digits; j != 0u; j /= 10u, ++k) {
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
    const fixed_exp_t exponent = static_cast<fixed_exp_t>(fd.exponent + stored_digits - 1);

    // Allocate space for it, +1 for 'e', and +1 for exponent sign.
    tmp.grow(num_exp_digits(exponent) + 2u);

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

template<typename T>
    requires(raw_string<T> || pointer<T> || function_pointer<T> || member_function_pointer<T>)
[[nodiscard]] constexpr bool append(small_string_span ss, T&& value) noexcept {
    if constexpr (raw_string<T>) {
        return append(ss, std::string_view(value, std::extent_v<impl::decay_object<T>> - 1u));
    } else {
        if (value == nullptr) {
            return append(ss, nullptr);
        }

        if constexpr (function_pointer<T> || member_function_pointer<T>) {
            constexpr std::string_view function_ptr_str = "0x????????";
            return append(ss, function_ptr_str);
        } else if constexpr (std::is_same_v<impl::decay_object<T>, const char*>) {
            return append(ss, std::string_view(value));
        } else {
            return append(ss, static_cast<const void*>(value));
        }
    }
}

template<typename T>
concept string_appendable = requires(small_string_span ss, T value) { append(ss, value); };

template<string_appendable T, string_appendable U, string_appendable... Args>
[[nodiscard]] constexpr bool append(small_string_span ss, T&& t, U&& u, Args&&... args) noexcept {
    return append(ss, std::forward<T>(t)) && append(ss, std::forward<U>(u)) &&
           (append(ss, std::forward<Args>(args)) && ...);
}
} // namespace snitch

#endif
