#define SNITCH_IMPLEMENTATION

#if defined(SNITCH_TEST_WITH_SNITCH)
#    if defined(SNITCH_TEST_HEADER_ONLY) && SNITCH_WITH_STDOUT && SNITCH_WITH_STD_FILE_IO
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
// Define our own implementation for the tests, using `std::cout` just as an alternative example.
// If you want to test without standard output, replace the implementation here with your own.
#    include <iostream>
void custom_console_print(std::string_view message) noexcept {
    std::cout << message << std::flush;
}

int init_console [[maybe_unused]] = [] {
    snitch::cli::console_print = &custom_console_print;
    return 0;
}();
#endif

#if !SNITCH_WITH_STD_FILE_IO
// Library is configured without standard file I/O.
// Define our own implementation for the tests, using `std::ofstream` just as an alternative
// example. If you want to test without standard file I/O, replace the implementation here with your
// own.
#    include <fstream>
void custom_file_open(snitch::file_object_storage& storage, std::string_view path) {
    storage.emplace<std::ofstream>(std::string(path));
    if (!storage.get<std::ofstream>().is_open()) {
        snitch::assertion_failed("output file could not be opened for writing");
    }
}

void custom_file_write(
    const snitch::file_object_storage& storage, std::string_view message) noexcept {
    storage.get_mutable<std::ofstream>() << message << std::flush;
}

void custom_file_close(snitch::file_object_storage& storage) noexcept {
    storage.reset();
}

int init_file [[maybe_unused]] = [] {
    snitch::file::open  = &custom_file_open;
    snitch::file::write = &custom_file_write;
    snitch::file::close = &custom_file_close;
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
