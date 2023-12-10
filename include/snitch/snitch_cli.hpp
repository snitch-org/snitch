#ifndef SNITCH_CLI_HPP
#define SNITCH_CLI_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_function.hpp"
#include "snitch/snitch_string.hpp"
#include "snitch/snitch_string_utility.hpp"
#include "snitch/snitch_vector.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

// Testing framework configuration.
// --------------------------------

namespace snitch {
// Maximum number of command line arguments.
constexpr std::size_t max_command_line_args = SNITCH_MAX_COMMAND_LINE_ARGS;
} // namespace snitch

namespace snitch::cli {
struct argument {
    std::string_view                name       = {};
    std::optional<std::string_view> value_name = {};
    std::optional<std::string_view> value      = {};
};

struct input {
    std::string_view                              executable = {};
    small_vector<argument, max_command_line_args> arguments  = {};
};

SNITCH_EXPORT extern function_ref<void(std::string_view) noexcept> console_print;

template<typename... Args>
void print(Args&&... args) noexcept {
    small_string<max_message_length> message;
    append_or_truncate(message, std::forward<Args>(args)...);
    console_print(message);
}

struct print_help_settings {
    bool with_color = true;
};

SNITCH_EXPORT void print_help(
    std::string_view           program_name,
    const print_help_settings& settings = print_help_settings{}) noexcept;

SNITCH_EXPORT std::optional<input> parse_arguments(int argc, const char* const argv[]) noexcept;

SNITCH_EXPORT std::optional<cli::argument>
              get_option(const cli::input& args, std::string_view name) noexcept;

SNITCH_EXPORT std::optional<cli::argument>
              get_positional_argument(const cli::input& args, std::string_view name) noexcept;

SNITCH_EXPORT void for_each_positional_argument(
    const cli::input&                                    args,
    std::string_view                                     name,
    const function_ref<void(std::string_view) noexcept>& callback) noexcept;
} // namespace snitch::cli

#endif
