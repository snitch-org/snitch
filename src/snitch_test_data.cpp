#include "snitch/snitch_test_data.hpp"

#include "snitch/snitch_registry.hpp"

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
    if (std::uncaught_exceptions() == 0) {
        notify_exception_handled();
    }
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

namespace snitch {
#if SNITCH_WITH_EXCEPTIONS
void notify_exception_handled() noexcept {
    auto& state = impl::get_current_test();
    if (!state.held_info.has_value()) {
        return;
    }

    // Close all sections that were left open by the exception.
    auto&       current_held_section = state.held_info.value().sections.current_section;
    const auto& current_section      = state.info.sections.current_section;
    while (current_held_section.size() > current_section.size()) {
        registry::report_section_ended(current_held_section.back());
        current_held_section.pop_back();
    }

    state.held_info.reset();
}
#endif
} // namespace snitch
