#include "snitch/snitch_append.hpp"

#include "snitch/snitch_concepts.hpp"
#include "snitch/snitch_string.hpp"

#include <charconv> // for std::to_chars
#include <cstdint> // for std::uintptr_t
#include <cstring> // for std::memmove
#include <system_error> // for std::errc

namespace snitch::impl {
namespace {
using snitch::small_string_span;

#if SNITCH_APPEND_TO_CHARS
// libstdc++ version 11 or greater
// libc++ version 14 or greater
// MSVC 19.24 or greater (also known as Visual Studio 2019 16.4)
// Apple clang and others have no support as of April-2024
#    if (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE > 11) ||                                    \
        (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION > 14000) ||                                   \
        (defined(_MSC_VER) && _MSC_VER > 1924)
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
#    else
template<std::floating_point T>
bool append_to(small_string_span ss, T value) noexcept {
    return append_constexpr(ss, value);
}
#    endif

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

bool append_to(small_string_span ss, const void* ptr) noexcept {
    return append(ss, "0x") && append_to<16>(ss, reinterpret_cast<std::uintptr_t>(ptr));
}

#else
template <floating_point T>
bool append_to(small_string_span ss, T value) noexcept {
    return append_constexpr(ss, value);
}

template <large_int_t Base = 10, integral T>
bool append_to(small_string_span ss, T value) noexcept {
    return append_constexpr(ss, value);
}

bool append_to(small_string_span ss, const void* value) noexcept {
    return append_constexpr(ss, value);
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
    std::memmove(ss.begin() + offset, str.data(), copy_count);

    return could_fit;
}

bool append_fast(small_string_span ss, const void* ptr) noexcept {
    if (ptr == nullptr) {
        return append(ss, nullptr);
    } else {
        return append(ss, "0x") && append_to(ss, ptr);
    }
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
