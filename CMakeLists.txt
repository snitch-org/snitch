cmake_minimum_required(VERSION 3.8)

project(snatch LANGUAGES CXX VERSION 1.0)

set(SNATCH_MAX_TEST_CASES         5000 CACHE STRING "Maximum number of test cases in a test application.")
set(SNATCH_MAX_EXPR_LENGTH        1024 CACHE STRING "Maximum length of a printed expression when reporting failure.")
set(SNATCH_MAX_MATCHER_MSG_LENGTH 1024 CACHE STRING "Maximum length of matcher error message when reporting failure.")
set(SNATCH_MAX_TEST_NAME_LENGTH   1024 CACHE STRING "Maximum length of a test case name.")
set(SNATCH_MAX_UNIQUE_TAGS        1024 CACHE STRING "Maximum number of unique tags in a test application.")
set(SNATCH_DEFINE_MAIN            ON   CACHE BOOL   "Define main() in snatch -- disable to provide your own main() function.")
set(SNATCH_WITH_EXCEPTIONS        ON   CACHE BOOL   "Use exceptions in snatch implementation -- will be force OFF if exceptions are not available.")

add_library(snatch ${PROJECT_SOURCE_DIR}/src/snatch.cpp)

add_library(snatch::snatch ALIAS snatch)

target_compile_features(snatch PUBLIC cxx_std_20)
target_include_directories(snatch PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

target_compile_definitions(snatch PUBLIC
  SNATCH_MAX_TEST_CASES=${SNATCH_MAX_TEST_CASES}
  SNATCH_MAX_EXPR_LENGTH=${SNATCH_MAX_EXPR_LENGTH}
  SNATCH_MAX_MATCHER_MSG_LENGTH=${SNATCH_MAX_MATCHER_MSG_LENGTH}
  SNATCH_MAX_TEST_NAME_LENGTH=${SNATCH_MAX_TEST_NAME_LENGTH}
  SNATCH_MAX_UNIQUE_TAGS=${SNATCH_MAX_UNIQUE_TAGS}
  SNATCH_DEFINE_MAIN=$<BOOL:${SNATCH_DEFINE_MAIN}>
  SNATCH_WITH_EXCEPTIONS=$<BOOL:${SNATCH_WITH_EXCEPTIONS}>)

install(FILES ${PROJECT_SOURCE_DIR}/include/snatch/snatch.hpp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snatch)
