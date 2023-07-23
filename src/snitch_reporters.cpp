#include "snitch/snitch_macros_reporter.hpp"

#if defined(SNITCH_WITH_TEAMCITY_REPORTER) || defined(SNITCH_WITH_ALL_REPORTERS)
#    include "snitch/snitch_teamcity.hpp"
#endif
#if defined(SNITCH_WITH_CATCH2_XML_REPORTER) || defined(SNITCH_WITH_ALL_REPORTERS)
#    include "snitch/snitch_catch2_xml.hpp"
#endif
