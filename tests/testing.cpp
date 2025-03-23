#define SNITCH_IMPLEMENTATION

#if defined(SNITCH_TEST_WITH_SNITCH)
#    if defined(SNITCH_TEST_HEADER_ONLY) && SNITCH_WITH_STDOUT
#        undef SNITCH_DEFINE_MAIN
#        define SNITCH_DEFINE_MAIN 1
#    endif
#else
#    define DOCTEST_CONFIG_IMPLEMENT
#    define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif

#include "testing.hpp"

#if !SNITCH_WITH_STDOUT
// Library is configured without `stdout`.
// Define our own implementation for the tests, which uses `stdout` again...
// If you want to test without `stdout`, replace the implementation here with your own.
#    include <cstdio>
void custom_console_print(std::string_view message) noexcept {
    std::fwrite(message.data(), sizeof(char), message.length(), stdout);
}

int init [[maybe_unused]] = [] {
    snitch::cli::console_print = &custom_console_print;
    return 0;
}();
#endif

#if defined(SNITCH_TEST_WITH_SNITCH) && !SNITCH_DEFINE_MAIN
int main(int argc, char* argv[]) {
#    if !SNITCH_WITH_STDOUT
    snitch::tests.print_callback = &custom_console_print;
#    endif
    return snitch::main(argc, argv);
}
#endif

bool contains_color_codes(std::string_view msg) noexcept {
    constexpr std::array codes = {snitch::impl::color::error,      snitch::impl::color::warning,
                                  snitch::impl::color::status,     snitch::impl::color::fail,
                                  snitch::impl::color::skipped,    snitch::impl::color::pass,
                                  snitch::impl::color::highlight1, snitch::impl::color::highlight2,
                                  snitch::impl::color::reset};

    for (const auto c : codes) {
        if (msg.find(c) != std::string_view::npos) {
            return true;
        }
    }

    return false;
}
