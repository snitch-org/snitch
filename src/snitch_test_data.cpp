#include "snitch/snitch_test_data.hpp"

namespace snitch::impl {
namespace {
thread_local test_state* thread_current_test = nullptr;
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
} // namespace snitch::impl
