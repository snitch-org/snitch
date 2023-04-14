#ifndef SNITCH_ERROR_HANDLING_HPP
#define SNITCH_ERROR_HANDLING_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_function.hpp"

#include <string_view>

namespace snitch {
[[noreturn]] void terminate_with(std::string_view msg) noexcept;

extern small_function<void(std::string_view)> assertion_failed_handler;

[[noreturn]] void assertion_failed(std::string_view msg);
} // namespace snitch

#endif
