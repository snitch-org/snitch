#include "snitch/snitch_console.hpp"

#if SNITCH_WITH_STDOUT
#    include <cstdio> // for std::fwrite and stdout
#else
#    include <exception> // for std::terminate
#endif

namespace snitch::impl {
#if SNITCH_WITH_STDOUT
void stdout_print(std::string_view message) noexcept {
    std::fwrite(message.data(), sizeof(char), message.length(), stdout);
}
#else
// No default console; it is expected that the user will use their own
// implementation wherever a console is needed.
void stdout_print(std::string_view) noexcept {
    std::terminate();
}
#endif
} // namespace snitch::impl
