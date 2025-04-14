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
struct basic_vtable {
    type_id_t                                 id            = snitch::type_id<void>();
    function_ptr<void(void*) noexcept>        delete_object = [](void*) noexcept {};
    function_ptr<void(void*, void*) noexcept> move_object   = [](void*, void*) noexcept {};
};

SNITCH_EXPORT extern const basic_vtable empty_vtable;

template<typename T>
const basic_vtable* get_vtable() noexcept {
    static const basic_vtable table{
        .id            = snitch::type_id<T>(),
        .delete_object = [](void* storage) noexcept { reinterpret_cast<T*>(storage)->~T(); },
        .move_object =
            [](void* storage, void* from) noexcept {
                new (storage) T(std::move(*reinterpret_cast<T*>(from)));
            }};
    return &table;
}
} // namespace impl

template<std::size_t MaxSize>
class inplace_any {
    std::array<char, MaxSize> storage = {};
    const impl::basic_vtable* vtable  = &impl::empty_vtable;

    template<typename T>
    void check() const {
        if (vtable != impl::get_vtable<T>()) {
            if (vtable == &impl::empty_vtable) {
                assertion_failed("inplace_any is empty");
            } else {
                assertion_failed("inplace_any holds an object of a different type");
            }
        }
    }

public:
    constexpr inplace_any() = default;

    inplace_any(const inplace_any&) = delete;

    constexpr inplace_any(inplace_any&& other) noexcept : vtable(other.vtable) {
        vtable->move_object(storage.data(), other.storage.data());
        other.reset();
    }

    inplace_any& operator=(const inplace_any&) = delete;

    constexpr inplace_any& operator=(inplace_any&& other) noexcept {
        vtable->delete_object(storage.data());
        vtable = other.vtable;
        vtable->move_object(storage.data(), other.storage.data());
        other.reset();
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
        return vtable != &impl::empty_vtable;
    }

    type_id_t type() const noexcept {
        return vtable->id;
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args) {
        static_assert(
            sizeof(T) <= MaxSize,
            "This type is too large to fit in this inplace_any, increase storage size");

        vtable->delete_object(storage.data());
        new (storage.data()) T(std::forward<Args>(args)...);
        vtable = impl::get_vtable<T>();
        return *reinterpret_cast<T*>(storage.data());
    }

    // Requires: not empty and stored type == T.
    template<typename T>
    const T& get() const {
        check<T>();
        return *reinterpret_cast<const T*>(storage.data());
    }

    // Requires: not empty and stored type == T.
    template<typename T>
    T& get() {
        check<T>();
        return *reinterpret_cast<T*>(storage.data());
    }

    // Requires: not empty and stored type == T.
    template<typename T>
    T& get_mutable() const {
        check<T>();
        return const_cast<T&>(*reinterpret_cast<const T*>(storage.data()));
    }

    void reset() noexcept {
        vtable->delete_object(storage.data());
        vtable = &impl::empty_vtable;
    }
};
} // namespace snitch

#endif
