#ifndef SNITCH_MACROS_MISC_HPP
#define SNITCH_MACROS_MISC_HPP

#include "snitch/snitch_capture.hpp"
#include "snitch/snitch_config.hpp"
#include "snitch/snitch_macros_utility.hpp"
#include "snitch/snitch_section.hpp"
#include "snitch/snitch_test_data.hpp"

#if SNITCH_ENABLE
#    define SNITCH_SECTION(...)                                                                    \
        if (snitch::impl::section_entry_checker SNITCH_MACRO_CONCAT(section_id_, __COUNTER__){     \
                {__VA_ARGS__}, SNITCH_CURRENT_LOCATION, snitch::impl::get_current_test()})

#    define SNITCH_CAPTURE(...)                                                                    \
        auto SNITCH_MACRO_CONCAT(capture_id_, __COUNTER__) = snitch::impl::add_captures(           \
            snitch::impl::get_current_test(), #__VA_ARGS__, __VA_ARGS__)

#    define SNITCH_INFO(...)                                                                       \
        auto SNITCH_MACRO_CONCAT(capture_id_, __COUNTER__) =                                       \
            snitch::impl::add_info(snitch::impl::get_current_test(), __VA_ARGS__)
#else // SNITCH_ENABLE
// clang-format off
#    define SNITCH_SECTION(NAME, ...) if constexpr (false)
#    define SNITCH_CAPTURE(...)       SNITCH_DISCARD_ARGS(__VA_ARGS__)
#    define SNITCH_INFO(...)          SNITCH_DISCARD_ARGS(__VA_ARGS__)
// clang-format on
#endif // SNITCH_ENABLE

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define SECTION(NAME, ...) SNITCH_SECTION(NAME, __VA_ARGS__)
#    define CAPTURE(...)       SNITCH_CAPTURE(__VA_ARGS__)
#    define INFO(...)          SNITCH_INFO(__VA_ARGS__)
#endif
// clang-format on

#endif
