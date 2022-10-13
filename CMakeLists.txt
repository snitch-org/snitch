cmake_minimum_required(VERSION 3.8)

project(snatch LANGUAGES CXX VERSION 1.0)

set(SNATCH_MAX_TEST_CASES         5000 CACHE STRING "Maximum number of test cases in a test application.")
set(SNATCH_MAX_EXPR_LENGTH        1024 CACHE STRING "Maximum length of a printed expression when reporting failure.")
set(SNATCH_MAX_MATCHER_MSG_LENGTH 1024 CACHE STRING "Maximum length of matcher error message when reporting failure.")
set(SNATCH_MAX_TEST_NAME_LENGTH   1024 CACHE STRING "Maximum length of a test case name.")
set(SNATCH_DEFINE_MAIN            ON   CACHE BOOL   "Define main() in snatch -- disable to provide your own main() function.")

add_library(snatch ${PROJECT_SOURCE_DIR}/src/snatch.cpp)

add_library(snatch::snatch ALIAS snatch)

target_compile_features(snatch PUBLIC cxx_std_20)
target_include_directories(snatch PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

install(FILES ${PROJECT_SOURCE_DIR}/include/snatch.hpp
target_compile_definitions(snatch PUBLIC
  SNATCH_MAX_TEST_CASES=${SNATCH_MAX_TEST_CASES}
  SNATCH_MAX_EXPR_LENGTH=${SNATCH_MAX_EXPR_LENGTH}
  SNATCH_MAX_MATCHER_MSG_LENGTH=${SNATCH_MAX_MATCHER_MSG_LENGTH}
  SNATCH_MAX_TEST_NAME_LENGTH=${SNATCH_MAX_TEST_NAME_LENGTH})

  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snatch)
