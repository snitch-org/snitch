cmake_minimum_required(VERSION 3.8)

project(snitch LANGUAGES CXX VERSION 1.0.0)

set(SNITCH_MAX_TEST_CASES         5000 CACHE STRING "Maximum number of test cases in a test application.")
set(SNITCH_MAX_NESTED_SECTIONS    8    CACHE STRING "Maximum depth of nested sections in a test case.")
set(SNITCH_MAX_EXPR_LENGTH        1024 CACHE STRING "Maximum length of a printed expression when reporting failure.")
set(SNITCH_MAX_MESSAGE_LENGTH     1024 CACHE STRING "Maximum length of error or status messages.")
set(SNITCH_MAX_TEST_NAME_LENGTH   1024 CACHE STRING "Maximum length of a test case name.")
set(SNITCH_MAX_TAG_LENGTH         256  CACHE STRING "Maximum length of a test tag.")
set(SNITCH_MAX_CAPTURES           8    CACHE STRING "Maximum number of captured expressions in a test case.")
set(SNITCH_MAX_CAPTURE_LENGTH     256  CACHE STRING "Maximum length of a captured expression.")
set(SNITCH_MAX_UNIQUE_TAGS        1024 CACHE STRING "Maximum number of unique tags in a test application.")
set(SNITCH_MAX_COMMAND_LINE_ARGS  1024 CACHE STRING "Maximum number of command line arguments to a test application.")
set(SNITCH_DEFINE_MAIN            ON   CACHE BOOL   "Define main() in snitch -- disable to provide your own main() function.")
set(SNITCH_WITH_EXCEPTIONS        ON   CACHE BOOL   "Use exceptions in snitch implementation -- will be forced OFF if exceptions are not available.")
set(SNITCH_WITH_TIMINGS           ON   CACHE BOOL   "Measure the time taken by each test case -- disable to speed up tests.")
set(SNITCH_WITH_SHORTHAND_MACROS  ON   CACHE BOOL   "Use short names for test macros -- disable if this causes conflicts.")
set(SNITCH_DEFAULT_WITH_COLOR     ON   CACHE BOOL   "Enable terminal colors by default -- can also be controlled by command line interface.")
set(SNITCH_CREATE_HEADER_ONLY     ON   CACHE BOOL   "Create a single-header header-only version of snitch.")
set(SNITCH_CREATE_LIBRARY         ON   CACHE BOOL   "Build a compiled library version of snitch.")

# Figure out git hash, if any
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  RESULT_VARIABLE GIT_COMMAND_SUCCESS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (GIT_COMMAND_SUCCESS EQUAL 0)
    set(SNITCH_FULL_VERSION "${PROJECT_VERSION}.${GIT_COMMIT_HASH}")
endif()

# Create configure file to store CMake build parameter
configure_file("${PROJECT_SOURCE_DIR}/include/snitch/snitch_config.hpp.config"
               "${PROJECT_BINARY_DIR}/snitch/snitch_config.hpp")

if (SNITCH_CREATE_LIBRARY)
  # Build as a standard library (static or dynamic) with header.
  add_library(snitch
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_teamcity.hpp
    ${PROJECT_BINARY_DIR}/snitch/snitch_config.hpp
    ${PROJECT_SOURCE_DIR}/src/snitch.cpp)

  add_library(snitch::snitch ALIAS snitch)
  set_target_properties(snitch PROPERTIES EXPORT_NAME snitch::snitch)

  target_compile_features(snitch PUBLIC cxx_std_20)
  target_include_directories(snitch PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

  target_compile_definitions(snitch PUBLIC
    SNITCH_MAX_TEST_CASES=${SNITCH_MAX_TEST_CASES}
    SNITCH_MAX_NESTED_SECTIONS=${SNITCH_MAX_NESTED_SECTIONS}
    SNITCH_MAX_EXPR_LENGTH=${SNITCH_MAX_EXPR_LENGTH}
    SNITCH_MAX_MESSAGE_LENGTH=${SNITCH_MAX_MESSAGE_LENGTH}
    SNITCH_MAX_TEST_NAME_LENGTH=${SNITCH_MAX_TEST_NAME_LENGTH}
    SNITCH_MAX_TAG_LENGTH=${SNITCH_MAX_TAG_LENGTH}
    SNITCH_MAX_UNIQUE_TAGS=${SNITCH_MAX_UNIQUE_TAGS}
    SNITCH_MAX_COMMAND_LINE_ARGS=${SNITCH_MAX_COMMAND_LINE_ARGS}
    SNITCH_DEFINE_MAIN=$<BOOL:${SNITCH_DEFINE_MAIN}>
    SNITCH_WITH_EXCEPTIONS=$<BOOL:${SNITCH_WITH_EXCEPTIONS}>
    SNITCH_WITH_TIMINGS=$<BOOL:${SNITCH_WITH_TIMINGS}>
    SNITCH_WITH_SHORTHAND_MACROS=$<BOOL:${SNITCH_WITH_SHORTHAND_MACROS}>
    SNITCH_DEFAULT_WITH_COLOR=$<BOOL:${SNITCH_DEFAULT_WITH_COLOR}>)

  install(FILES
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_teamcity.hpp
    ${PROJECT_BINARY_DIR}/snitch/snitch_config.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snitch)
endif()

if(SNITCH_CREATE_HEADER_ONLY)
  find_package(Python3)

  # Build as a header-only library.
  add_custom_command(
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/make_snitch_all.py" "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}"
    VERBATIM
    OUTPUT ${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp
    DEPENDS
      ${PROJECT_SOURCE_DIR}/include/snitch/snitch.hpp
      ${PROJECT_BINARY_DIR}/snitch/snitch_config.hpp
      ${PROJECT_SOURCE_DIR}/src/snitch.cpp)

  add_custom_target(snitch-header-only-impl ALL DEPENDS ${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp)

  add_library(snitch-header-only INTERFACE)
  add_library(snitch::snitch-header-only ALIAS snitch-header-only)
  add_dependencies(snitch-header-only snitch-header-only-impl)
  set_target_properties(snitch-header-only PROPERTIES EXPORT_NAME snitch::snitch-header-only)

  target_sources(snitch-header-only INTERFACE
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/snitch/snitch_all.hpp>)
  target_include_directories(snitch-header-only INTERFACE
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)
  target_compile_features(snitch-header-only INTERFACE cxx_std_20)

  install(FILES
    ${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snitch)
endif()

# Setup CMake config file
if (SNITCH_CREATE_LIBRARY AND SNITCH_CREATE_HEADER_ONLY)
  install(TARGETS snitch snitch-header-only EXPORT snitch-targets)
elseif (SNITCH_CREATE_LIBRARY)
  install(TARGETS snitch EXPORT snitch-targets)
elseif (SNITCH_CREATE_HEADER_ONLY)
  install(TARGETS snitch-header-only EXPORT snitch-targets)
endif()

install(EXPORT snitch-targets
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/snitch COMPONENT Development)

export(EXPORT snitch-targets)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/snitch-config.cmake.in"
  "${PROJECT_BINARY_DIR}/snitch-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  NO_SET_AND_CHECK_MACRO)

install(FILES
  "${PROJECT_BINARY_DIR}/snitch-config.cmake"
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/snitch COMPONENT Development)

# Setup tests
if (SNITCH_DO_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
