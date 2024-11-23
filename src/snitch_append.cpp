#include "snitch/snitch_append.hpp"

#include "snitch/snitch_concepts.hpp"
#include "snitch/snitch_string.hpp"

#include <algorithm> // for std::copy
#include <cstdint> // for std::uintptr_t
#if SNITCH_APPEND_TO_CHARS
#    include <charconv> // for std::to_chars
#    include <system_error> // for std::errc
#endif

namespace snitch::impl {
namespace {
using snitch::small_string_span;
using namespace std::literals;

#if SNITCH_APPEND_TO_CHARS
template<floating_point T>
bool append_to(small_string_span ss, T value) noexcept {
    constexpr auto fmt       = std::chars_format::scientific;
    constexpr auto precision = same_as<float, std::remove_cvref_t<T>> ? 6 : 15;
    auto [end, err] = std::to_chars(ss.end(), ss.begin() + ss.capacity(), value, fmt, precision);
    if (err != std::errc{}) {
        // Not enough space, try into a temporary string that *should* be big enough,
        // and copy whatever we can. 32 characters is enough for all integers and floating
        // point values encoded on 64 bit or less.
        small_string<32> fallback;
        auto [end2, err2] = std::to_chars(
            fallback.end(), fallback.begin() + fallback.capacity(), value, fmt, precision);
        if (err2 != std::errc{}) {
            return false;
        }
        fallback.grow(end2 - fallback.begin());
        return append(ss, fallback);
    }

    ss.grow(end - ss.end());
    return true;
}

template<large_int_t Base = 10, integral T>
bool append_to(small_string_span ss, T value) noexcept {
    auto [end, err] = std::to_chars(ss.end(), ss.begin() + ss.capacity(), value, Base);
    if (err != std::errc{}) {
        // Not enough space, try into a temporary string that *should* be big enough,
        // and copy whatever we can. 32 characters is enough for all integers and floating
        // point values encoded on 64 bit or less.
        small_string<32> fallback;
        auto [end2, err2] =
            std::to_chars(fallback.end(), fallback.begin() + fallback.capacity(), value, Base);
        if (err2 != std::errc{}) {
            return false;
        }
        fallback.grow(end2 - fallback.begin());
        return append(ss, fallback);
    }
    ss.grow(end - ss.end());
    return true;
}
#else
template<floating_point T>
bool append_to(small_string_span ss, T value) noexcept {
    return append_constexpr(ss, value);
}

template<large_int_t Base = 10, integral T>
bool append_to(small_string_span ss, T value) noexcept {
    return append_constexpr<Base>(ss, value);
}
#endif
} // namespace

bool append_fast(small_string_span ss, std::string_view str) noexcept {
    if (str.empty()) {
        return true;
    }

    const bool        could_fit  = str.size() <= ss.available();
    const std::size_t copy_count = std::min(str.size(), ss.available());

    const std::size_t offset = ss.size();
    ss.grow(copy_count);
    std::copy(str.begin(), str.begin() + copy_count, ss.begin() + offset);

    return could_fit;
}

bool append_fast(small_string_span ss, const void* ptr) noexcept {
    if (ptr == nullptr) {
        return append(ss, nullptr);
    }

    if (!append_fast(ss, "0x"sv)) {
        return false;
    }

    const auto int_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    // Pad with zeros.
    constexpr std::size_t max_digits = 2 * sizeof(void*);
    std::size_t           padding    = max_digits - num_digits<16>(int_ptr);
    while (padding > 0) {
        constexpr std::string_view zeroes = "0000000000000000";
        const std::size_t          batch  = std::min(zeroes.size(), padding);
        if (!append_fast(ss, zeroes.substr(0, batch))) {
            return false;
        }

        padding -= batch;
    }
    return append_to<16>(ss, int_ptr);
}

bool append_fast(small_string_span ss, large_uint_t i) noexcept {
    return append_to(ss, i);
}

bool append_fast(small_string_span ss, large_int_t i) noexcept {
    return append_to(ss, i);
}

bool append_fast(small_string_span ss, float f) noexcept {
    return append_to(ss, f);
}

bool append_fast(small_string_span ss, double d) noexcept {
    return append_to(ss, d);
}
} // namespace snitch::impl
