cmake_minimum_required(VERSION 3.8)

project(snatch LANGUAGES CXX VERSION 1.0)

set(SNATCH_MAX_TEST_CASES         5000 CACHE STRING "Maximum number of test cases in a test application.")
set(SNATCH_MAX_EXPR_LENGTH        1024 CACHE STRING "Maximum length of a printed expression when reporting failure.")
set(SNATCH_MAX_MESSAGE_LENGTH     1024 CACHE STRING "Maximum length of error or status messages.")
set(SNATCH_MAX_TEST_NAME_LENGTH   1024 CACHE STRING "Maximum length of a test case name.")
set(SNATCH_MAX_UNIQUE_TAGS        1024 CACHE STRING "Maximum number of unique tags in a test application.")
set(SNATCH_MAX_COMMAND_LINE_ARGS  1024 CACHE STRING "Maximum number of command line arguments to a test application.")
set(SNATCH_DEFINE_MAIN            ON   CACHE BOOL   "Define main() in snatch -- disable to provide your own main() function.")
set(SNATCH_WITH_EXCEPTIONS        ON   CACHE BOOL   "Use exceptions in snatch implementation -- will be force OFF if exceptions are not available.")
set(SNATCH_WITH_SHORTHAND_MACROS  ON   CACHE BOOL   "Use short names for test macros -- disable if this causes conflicts.")
set(SNATCH_DEFAULT_WITH_COLOR     ON   CACHE BOOL   "Enable terminal colors by default -- can also be controlled on command line.")

add_library(snatch ${PROJECT_SOURCE_DIR}/src/snatch.cpp)

add_library(snatch::snatch ALIAS snatch)

target_compile_features(snatch PUBLIC cxx_std_20)
target_include_directories(snatch PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

target_compile_definitions(snatch PUBLIC
  SNATCH_MAX_TEST_CASES=${SNATCH_MAX_TEST_CASES}
  SNATCH_MAX_EXPR_LENGTH=${SNATCH_MAX_EXPR_LENGTH}
  SNATCH_MAX_MESSAGE_LENGTH=${SNATCH_MAX_MESSAGE_LENGTH}
  SNATCH_MAX_TEST_NAME_LENGTH=${SNATCH_MAX_TEST_NAME_LENGTH}
  SNATCH_MAX_UNIQUE_TAGS=${SNATCH_MAX_UNIQUE_TAGS}
  SNATCH_MAX_COMMAND_LINE_ARGS=${SNATCH_MAX_COMMAND_LINE_ARGS}
  SNATCH_DEFINE_MAIN=$<BOOL:${SNATCH_DEFINE_MAIN}>
  SNATCH_WITH_EXCEPTIONS=$<BOOL:${SNATCH_WITH_EXCEPTIONS}>
  SNATCH_WITH_SHORTHAND_MACROS=$<BOOL:${SNATCH_WITH_SHORTHAND_MACROS}>
  SNATCH_DEFAULT_WITH_COLOR=$<BOOL:${SNATCH_DEFAULT_WITH_COLOR}>)

install(FILES ${PROJECT_SOURCE_DIR}/include/snatch/snatch.hpp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snatch)

install(TARGETS snatch EXPORT snatch-targets)

install(EXPORT snatch-targets
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/snatch COMPONENT Development)

export(EXPORT snatch-targets)

# Setup CMake config file
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
