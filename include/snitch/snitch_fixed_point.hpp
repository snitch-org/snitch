#ifndef SNITCH_FIXED_POINT_HPP
#define SNITCH_FIXED_POINT_HPP

#include "snitch/snitch_config.hpp"

#include <cstddef>
#include <cstdint>
#if SNITCH_CONSTEXPR_FLOAT_USE_BITCAST
#    include <bit> // for compile-time float to string
#endif
#include <array>
#include <limits>

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

#endif
