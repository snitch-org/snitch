cmake_minimum_required(VERSION 3.8)

project(snatch LANGUAGES CXX VERSION 1.0.0)

set(SNATCH_MAX_TEST_CASES         5000 CACHE STRING "Maximum number of test cases in a test application.")
set(SNATCH_MAX_NESTED_SECTIONS    8    CACHE STRING "Maximum depth of nested sections in a test case.")
set(SNATCH_MAX_EXPR_LENGTH        1024 CACHE STRING "Maximum length of a printed expression when reporting failure.")
set(SNATCH_MAX_MESSAGE_LENGTH     1024 CACHE STRING "Maximum length of error or status messages.")
set(SNATCH_MAX_TEST_NAME_LENGTH   1024 CACHE STRING "Maximum length of a test case name.")
set(SNATCH_MAX_CAPTURES           8    CACHE STRING "Maximum number of captured expressions in a test case.")
set(SNATCH_MAX_CAPTURE_LENGTH     256  CACHE STRING "Maximum length of a captured expression.")
set(SNATCH_MAX_UNIQUE_TAGS        1024 CACHE STRING "Maximum number of unique tags in a test application.")
set(SNATCH_MAX_COMMAND_LINE_ARGS  1024 CACHE STRING "Maximum number of command line arguments to a test application.")
set(SNATCH_DEFINE_MAIN            ON   CACHE BOOL   "Define main() in snatch -- disable to provide your own main() function.")
set(SNATCH_WITH_EXCEPTIONS        ON   CACHE BOOL   "Use exceptions in snatch implementation -- will be forced OFF if exceptions are not available.")
set(SNATCH_WITH_TIMINGS           ON   CACHE BOOL   "Measure the time taken by each test case -- disable to speed up tests.")
set(SNATCH_WITH_SHORTHAND_MACROS  ON   CACHE BOOL   "Use short names for test macros -- disable if this causes conflicts.")
set(SNATCH_DEFAULT_WITH_COLOR     ON   CACHE BOOL   "Enable terminal colors by default -- can also be controlled by command line interface.")
set(SNATCH_CREATE_HEADER_ONLY     ON   CACHE BOOL   "Create a single-header header-only version of snatch.")
set(SNATCH_CREATE_LIBRARY         ON   CACHE BOOL   "Build a compiled library version of snatch.")

# Figure out git hash, if any
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  RESULT_VARIABLE GIT_COMMAND_SUCCESS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (GIT_COMMAND_SUCCESS EQUAL 0)
    set(SNATCH_FULL_VERSION "${PROJECT_VERSION}.${GIT_COMMIT_HASH}")
endif()

# Create configure file to store CMake build parameter
configure_file("${PROJECT_SOURCE_DIR}/include/snatch/snatch_config.hpp.config"
               "${PROJECT_BINARY_DIR}/snatch/snatch_config.hpp")

if (SNATCH_CREATE_LIBRARY)
  # Build as a standard library (static or dynamic) with header.
  add_library(snatch
    ${PROJECT_SOURCE_DIR}/include/snatch/snatch.hpp
    ${PROJECT_SOURCE_DIR}/include/snatch/snatch_teamcity.hpp
    ${PROJECT_SOURCE_DIR}/src/snatch.cpp)

  add_library(snatch::snatch ALIAS snatch)
  set_target_properties(snatch PROPERTIES EXPORT_NAME snatch::snatch)

  target_compile_features(snatch PUBLIC cxx_std_20)
  target_include_directories(snatch PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

  target_compile_definitions(snatch PUBLIC
    SNATCH_MAX_TEST_CASES=${SNATCH_MAX_TEST_CASES}
    SNATCH_MAX_NESTED_SECTIONS=${SNATCH_MAX_NESTED_SECTIONS}
    SNATCH_MAX_EXPR_LENGTH=${SNATCH_MAX_EXPR_LENGTH}
    SNATCH_MAX_MESSAGE_LENGTH=${SNATCH_MAX_MESSAGE_LENGTH}
    SNATCH_MAX_TEST_NAME_LENGTH=${SNATCH_MAX_TEST_NAME_LENGTH}
    SNATCH_MAX_UNIQUE_TAGS=${SNATCH_MAX_UNIQUE_TAGS}
    SNATCH_MAX_COMMAND_LINE_ARGS=${SNATCH_MAX_COMMAND_LINE_ARGS}
    SNATCH_DEFINE_MAIN=$<BOOL:${SNATCH_DEFINE_MAIN}>
    SNATCH_WITH_EXCEPTIONS=$<BOOL:${SNATCH_WITH_EXCEPTIONS}>
    SNATCH_WITH_TIMINGS=$<BOOL:${SNATCH_WITH_TIMINGS}>
    SNATCH_WITH_SHORTHAND_MACROS=$<BOOL:${SNATCH_WITH_SHORTHAND_MACROS}>
    SNATCH_DEFAULT_WITH_COLOR=$<BOOL:${SNATCH_DEFAULT_WITH_COLOR}>)

  install(FILES
    ${PROJECT_SOURCE_DIR}/include/snatch/snatch.hpp
    ${PROJECT_SOURCE_DIR}/include/snatch/snatch_teamcity.hpp
    ${PROJECT_BINARY_DIR}/snatch/snatch_config.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snatch)
endif()

if(SNATCH_CREATE_HEADER_ONLY)
  # Build as a header-only library.
  file(READ ${PROJECT_BINARY_DIR}/snatch/snatch_config.hpp SNATCH_FILE_CONFIG)
  file(READ ${PROJECT_SOURCE_DIR}/include/snatch/snatch.hpp SNATCH_FILE_HEADER)
  file(READ ${PROJECT_SOURCE_DIR}/src/snatch.cpp SNATCH_FILE_SOURCE)

  set(SNATCH_ALL_CONTENT "${SNATCH_FILE_CONFIG}\n${SNATCH_FILE_HEADER}\n#if defined(SNATCH_IMPLEMENTATION)\n${SNATCH_FILE_SOURCE}\n#endif")

  # Remove includes to snatch.hpp; it's all one header now
  string(REPLACE "#include \"snatch/snatch.hpp\"\n" "" SNATCH_ALL_CONTENT "${SNATCH_ALL_CONTENT}")
  string(REPLACE "#include \"snatch/snatch_config.hpp\"\n" "" SNATCH_ALL_CONTENT "${SNATCH_ALL_CONTENT}")

  file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/snatch/snatch_all.hpp CONTENT "${SNATCH_ALL_CONTENT}")

  add_library(snatch-header-only INTERFACE)
  add_library(snatch::snatch-header-only ALIAS snatch-header-only)
  set_target_properties(snatch-header-only PROPERTIES EXPORT_NAME snatch::snatch-header-only)

  target_sources(snatch-header-only INTERFACE
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/snatch/snatch_all.hpp>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/snatch/snatch_all.hpp>)
  set_source_files_properties(${PROJECT_BINARY_DIR}/snatch/snatch_all.hpp PROPERTIES GENERATED TRUE)
  target_include_directories(snatch-header-only INTERFACE
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)
  target_compile_features(snatch-header-only INTERFACE cxx_std_20)

  install(FILES
    ${PROJECT_BINARY_DIR}/snatch/snatch_all.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snatch)
endif()

# Setup CMake config file
if (SNATCH_CREATE_LIBRARY AND SNATCH_CREATE_HEADER_ONLY)
  install(TARGETS snatch snatch-header-only EXPORT snatch-targets)
elseif (SNATCH_CREATE_LIBRARY)
  install(TARGETS snatch EXPORT snatch-targets)
elseif (SNATCH_CREATE_HEADER_ONLY)
  install(TARGETS snatch-header-only EXPORT snatch-targets)
endif()

install(EXPORT snatch-targets
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/snatch COMPONENT Development)

export(EXPORT snatch-targets)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/snatch-config.cmake.in"
  "${PROJECT_BINARY_DIR}/snatch-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  NO_SET_AND_CHECK_MACRO)

install(FILES
  "${PROJECT_BINARY_DIR}/snatch-config.cmake"
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/snatch COMPONENT Development)

# Setup tests
if (SNATCH_DO_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
