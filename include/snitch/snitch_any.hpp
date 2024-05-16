#ifndef SNITCH_ANY_HPP
#define SNITCH_ANY_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_error_handling.hpp"
#include "snitch/snitch_function.hpp"
#include "snitch/snitch_type_id.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace snitch {
namespace impl {
template<typename T>
void delete_object(char* storage) noexcept {
    reinterpret_cast<T*>(storage)->~T();
}
} // namespace impl

template<std::size_t MaxSize>
class inplace_any {
    std::array<char, MaxSize>          storage = {};
    function_ref<void(char*) noexcept> deleter = [](char*) noexcept {};
    type_id_t                          id      = type_id<void>();

    void release() noexcept {
        deleter = [](char*) noexcept {};
        id      = type_id<void>();
    }

public:
    constexpr inplace_any() = default;

    inplace_any(const inplace_any&) = delete;

    constexpr inplace_any(inplace_any&& other) noexcept :
        storage(other.storage), deleter(other.deleter), id(other.id) {
        other.release();
    }

    inplace_any& operator=(const inplace_any&) = delete;

    constexpr inplace_any& operator=(inplace_any&& other) noexcept {
        reset();
        storage = other.storage;
        deleter = other.deleter;
        id      = other.id;
        other.release();
        return *this;
    }

    template<typename T, typename... Args>
    explicit inplace_any(std::in_place_type_t<T>, Args&&... args) {
        emplace<T>(std::forward<Args>(args)...);
    }

    ~inplace_any() {
        reset();
    }

    bool has_value() const noexcept {
        return id != type_id<void>();
    }

    type_id_t type() const noexcept {
        return id;
    }

    template<typename T, typename... Args>
    void emplace(Args&&... args) {
        static_assert(
            sizeof(T) <= MaxSize,
            "This type is too large to fit in this inplace_any, increase storage size");

        reset();
        new (storage.data()) T(std::forward<Args>(args)...);
        deleter = &impl::delete_object<T>;
        id      = type_id<T>();
    }

    // Requires: not empty and stored type == T.
    template<typename T>
    const T& get() const {
        if (!has_value()) {
            assertion_failed("inplace_any is empty");
        }
        if (type() != type_id<T>()) {
            assertion_failed("inplace_any holds an object of a different type");
        }

        return *reinterpret_cast<const T*>(storage.data());
    }

    // Requires: not empty and stored type == T.
    template<typename T>
    T& get() {
        return const_cast<T&>(const_cast<const inplace_any*>(this)->get<T>());
    }

    void reset() noexcept {
        deleter(storage.data());
        release();
    }
};
} // namespace snitch

#endif
