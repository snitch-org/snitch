#ifndef SNITCH_REPORTER_CATCH2_XML_HPP
#define SNITCH_REPORTER_CATCH2_XML_HPP

#include "snitch/snitch_config.hpp"

#if SNITCH_WITH_CATCH2_XML_REPORTER || SNITCH_WITH_ALL_REPORTERS

#    include "snitch/snitch_test_data.hpp"

#    include <cstddef>
#    include <string_view>

namespace snitch::reporter::catch2_xml {
struct reporter {
    std::size_t indent_level = 0;

    SNITCH_EXPORT explicit reporter(registry& r) noexcept;

    SNITCH_EXPORT bool configure(registry&, std::string_view, std::string_view) noexcept;

    SNITCH_EXPORT void report(const registry& r, const snitch::event::data& event) noexcept;
};
} // namespace snitch::reporter::catch2_xml

#endif
#endif
