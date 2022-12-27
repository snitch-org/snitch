find_path(SNITCH_INCLUDE_DIR snitch.hpp
  HINTS ${SNITCH_DIR}
  PATH_SUFFIXES snitch include/snitch
)

find_library(SNITCH_LIBRARY
    NAMES snitch snitch.lib
    HINTS ${SNITCH_DIR}
    PATH_SUFFIXES lib
)

set(SNITCH_INCLUDE_DIRS ${SNITCH_INCLUDE_DIR})
set(SNITCH_LIBRARIES ${SNITCH_LIBRARY})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(snitch REQUIRED_VARS SNITCH_INCLUDE_DIRS SNITCH_LIBRARIES)

mark_as_advanced(SNITCH_INCLUDE_DIR SNITCH_LIBRARY)

if (SNITCH_FOUND)
    if(NOT TARGET snitch::snitch)
        add_library(snitch::snitch UNKNOWN IMPORTED)
        set_target_properties(snitch::snitch PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SNITCH_INCLUDE_DIRS}")
        set_target_properties(snitch::snitch PROPERTIES INTERFACE_LINK_LIBRARIES "${SNITCH_LIBRARIES}")
        set_target_properties(snitch::snitch PROPERTIES IMPORTED_LOCATION "${SNITCH_LIBRARY}")
    endif()
endif()
