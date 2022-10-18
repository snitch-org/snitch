find_path(SNATCH_INCLUDE_DIR snatch.hpp
  HINTS ${SNATCH_DIR}
  PATH_SUFFIXES snatch include/snatch
)

find_library(SNATCH_LIBRARY
    NAMES snatch snatch.lib
    HINTS ${SNATCH_DIR}
    PATH_SUFFIXES lib
)

set(SNATCH_INCLUDE_DIRS ${SNATCH_INCLUDE_DIR})
set(SNATCH_LIBRARIES ${SNATCH_LIBRARY})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(snatch REQUIRED_VARS SNATCH_INCLUDE_DIRS SNATCH_LIBRARIES)

mark_as_advanced(SNATCH_INCLUDE_DIR SNATCH_LIBRARY)

if (SNATCH_FOUND)
    if(NOT TARGET snatch::snatch)
        add_library(snatch::snatch UNKNOWN IMPORTED)
        set_target_properties(snatch::snatch PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SNATCH_INCLUDE_DIRS}")
        set_target_properties(snatch::snatch PROPERTIES INTERFACE_LINK_LIBRARIES "${SNATCH_LIBRARIES}")
        set_target_properties(snatch::snatch PROPERTIES IMPORTED_LOCATION "${SNATCH_LIBRARY}")
    endif()
endif()
