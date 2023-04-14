#ifndef SNITCH_STRING_HPP
#define SNITCH_STRING_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_vector.hpp"

#include <string_view>

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

#endif
