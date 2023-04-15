#include "snitch/snitch_append.hpp"

#include <cinttypes> // for format strings
#include <cstdio> // for std::snprintf
#include <cstring> // for std::memmove

namespace snitch::impl {
namespace {
using snitch::small_string_span;

template<typename T>
constexpr const char* get_format_code() noexcept {
    if constexpr (std::is_same_v<T, const void*>) {
#if defined(_MSC_VER)
        return "0x%p";
#else
        return "%p";
#endif
    } else if constexpr (std::is_same_v<T, std::uintmax_t>) {
        return "%" PRIuMAX;
    } else if constexpr (std::is_same_v<T, std::intmax_t>) {
        return "%" PRIdMAX;
    } else if constexpr (std::is_same_v<T, float>) {
        return "%.6e";
    } else if constexpr (std::is_same_v<T, double>) {
        return "%.15e";
    } else {
        static_assert(!std::is_same_v<T, T>, "unsupported type");
    }
}

template<typename T>
bool append_fmt(small_string_span ss, T value) noexcept {
    if (ss.available() <= 1) {
        // snprintf will always print a null-terminating character,
        // so abort early if only space for one or zero character, as
        // this would clobber the original string.
        return false;
    }

    // Calculate required length.
    const int return_code = std::snprintf(nullptr, 0, get_format_code<T>(), value);
    if (return_code < 0) {
        return false;
    }

    // 'return_code' holds the number of characters that are required,
    // excluding the null-terminating character, which always gets appended,
    // so we need to +1.
    const std::size_t length    = static_cast<std::size_t>(return_code) + 1;
    const bool        could_fit = length <= ss.available();

    const std::size_t offset     = ss.size();
    const std::size_t prev_space = ss.available();
    ss.resize(std::min(ss.size() + length, ss.capacity()));
    std::snprintf(ss.begin() + offset, prev_space, get_format_code<T>(), value);

    // Pop the null-terminating character, always printed unfortunately.
    ss.pop_back();

    return could_fit;
}
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
        return append_fmt(ss, ptr);
    }
}

bool append_fast(small_string_span ss, large_uint_t i) noexcept {
    return append_fmt(ss, i);
}

bool append_fast(small_string_span ss, large_int_t i) noexcept {
    return append_fmt(ss, i);
}

bool append_fast(small_string_span ss, float f) noexcept {
    return append_fmt(ss, f);
}

bool append_fast(small_string_span ss, double d) noexcept {
    return append_fmt(ss, d);
}
} // namespace snitch::impl
