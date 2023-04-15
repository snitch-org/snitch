#include "snitch/snitch_console.hpp"

#include <cstdio> // for std::fwrite

namespace snitch::impl {
void stdout_print(std::string_view message) noexcept {
    std::fwrite(message.data(), sizeof(char), message.length(), stdout);
}
} // namespace snitch::impl
