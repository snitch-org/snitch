cmake_minimum_required(VERSION 3.8)

project(snatch LANGUAGES CXX VERSION 1.0)

add_library(snatch ${PROJECT_SOURCE_DIR}/src/snatch.cpp)

add_library(snatch::snatch ALIAS snatch)

target_compile_features(snatch PUBLIC cxx_std_20)
target_include_directories(snatch PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

install(FILES ${PROJECT_SOURCE_DIR}/include/snatch.hpp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/snatch)
