#ifndef SNITCH_VECTOR_HPP
#define SNITCH_VECTOR_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_error_handling.hpp"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <utility>

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
        return data_size != nullptr ? *data_size : 0;
    }
    constexpr bool empty() const noexcept {
        return data_size == nullptr || *data_size == 0;
    }

    // Requires: !empty().
    constexpr const ElemType& back() const {
        if (empty()) {
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

#endif
