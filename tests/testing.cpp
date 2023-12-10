#define SNITCH_IMPLEMENTATION

#if defined(SNITCH_TEST_WITH_SNITCH)
#    undef SNITCH_DEFINE_MAIN
#    define SNITCH_DEFINE_MAIN 1
#endif

#include "testing.hpp"

#if defined(SNITCH_TEST_WITH_SNITCH) && !defined(SNITCH_TEST_HEADER_ONLY)
#    undef SNITCH_EXPORT
#    define SNITCH_EXPORT
#    include "snitch_main.cpp"
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
