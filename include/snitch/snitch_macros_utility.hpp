#ifndef SNITCH_MACROS_UTILITY_HPP
#define SNITCH_MACROS_UTILITY_HPP

#include "snitch/snitch_config.hpp"

#define SNITCH_CONCAT_IMPL(x, y) x##y
#define SNITCH_MACRO_CONCAT(x, y) SNITCH_CONCAT_IMPL(x, y)

#define SNITCH_CURRENT_LOCATION                                                                    \
    snitch::source_location { std::string_view{__FILE__}, static_cast<std::size_t>(__LINE__) }

#endif
