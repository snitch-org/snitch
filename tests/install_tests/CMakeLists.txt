cmake_minimum_required(VERSION 3.8)
project(test_package LANGUAGES CXX)

find_package(snitch REQUIRED CONFIG)

add_executable(standalone standalone.cpp)

if (HEADER_ONLY)
    target_compile_definitions(standalone PRIVATE HEADER_ONLY)
    target_link_libraries(standalone PRIVATE snitch::snitch-header-only)
else()
    target_link_libraries(standalone PRIVATE snitch::snitch)
endif()

target_compile_features(standalone PRIVATE cxx_std_20)
