#include "snitch/snitch_test_data.hpp"

#if SNITCH_WITH_EXCEPTIONS
#    include <exception>
#endif

namespace snitch::impl {
namespace {
SNITCH_THREAD_LOCAL test_state* thread_current_test = nullptr;
}

test_state& get_current_test() noexcept {
    test_state* current = thread_current_test;
    if (current == nullptr) {
        terminate_with("no test case is currently running on this thread");
    }

    return *current;
}

test_state* try_get_current_test() noexcept {
    return thread_current_test;
}

void set_current_test(test_state* current) noexcept {
    thread_current_test = current;
}

void push_location(test_state& test, const assertion_location& location) noexcept {
    test.info.locations.push_back(location);
}

void pop_location(test_state& test) noexcept {
    test.info.locations.pop_back();
}

scoped_test_check::scoped_test_check(const source_location& location) noexcept :
    test(get_current_test()) {
#if SNITCH_WITH_EXCEPTIONS
    test.held_info.reset();
#endif

    push_location(test, {location.file, location.line, location_type::in_check});
    test.in_check = true;
}

scoped_test_check::~scoped_test_check() noexcept {
    test.in_check = false;

#if SNITCH_WITH_EXCEPTIONS
    if (std::uncaught_exceptions() > 0 && !test.held_info.has_value()) {
        // We are unwinding the stack because an exception has been thrown;
        // keep a copy of the full location state since we will want to preserve the information
        // when reporting the exception.
        test.held_info = test.info;
    }
#endif

    pop_location(test);
}
} // namespace snitch::impl
