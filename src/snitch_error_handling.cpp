#include "snitch/snitch_error_handling.hpp"

#include "snitch/snitch_console.hpp"

#include <exception> // for std::terminate

namespace snitch {
[[noreturn]] void terminate_with(std::string_view msg) noexcept {
    impl::stdout_print("terminate called with message: ");
    impl::stdout_print(msg);
    impl::stdout_print("\n");

    std::terminate();
}

[[noreturn]] void assertion_failed(std::string_view msg) {
    assertion_failed_handler(msg);

    // The assertion handler should either spin, throw, or terminate, but never return.
    // We cannot enforce [[noreturn]] through the small_function wrapper. So just in case
    // it accidentally returns, we terminate.
    std::terminate();
}

small_function<void(std::string_view)> assertion_failed_handler = &terminate_with;
} // namespace snitch
