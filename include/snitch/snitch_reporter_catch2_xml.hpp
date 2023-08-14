#ifndef SNITCH_CATCH2_XML_HPP
#define SNITCH_CATCH2_XML_HPP

#include "snitch/snitch_config.hpp"

#if defined(SNITCH_WITH_CATCH2_XML_REPORTER) || defined(SNITCH_WITH_ALL_REPORTERS)

#    include "snitch/snitch_test_data.hpp"

#    include <cstddef>
#    include <initializer_list>
#    include <string_view>

namespace snitch::catch2_xml {
struct reporter {
    std::size_t indent_level = 0;

    explicit reporter(registry& r) noexcept;

    bool configure(registry&, std::string_view, std::string_view) noexcept;

    void report(const registry& r, const snitch::event::data& event) noexcept;
};
} // namespace snitch::catch2_xml

#endif
#endif
