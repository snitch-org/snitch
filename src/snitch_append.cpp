#include "snitch/snitch_append.hpp"

#include <cstdint> // for std::uintptr_t
#include <cstring> // for std::memmove

namespace snitch::impl {
using snitch::small_string_span;
using namespace std::literals;

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
    }

    if (!append_fast(ss, "0x"sv)) {
        return false;
    }

    const std::uintptr_t int_ptr = reinterpret_cast<std::uintptr_t>(ptr);

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

    return append_constexpr<16>(ss, int_ptr);
}

bool append_fast(small_string_span ss, large_uint_t i) noexcept {
    return append_constexpr(ss, i);
}

bool append_fast(small_string_span ss, large_int_t i) noexcept {
    return append_constexpr(ss, i);
}

bool append_fast(small_string_span ss, float f) noexcept {
    return append_constexpr(ss, f);
}

bool append_fast(small_string_span ss, double d) noexcept {
    return append_constexpr(ss, d);
}
} // namespace snitch::impl
