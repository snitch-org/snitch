cmake_minimum_required(VERSION 3.14)

project(main)

add_executable(main main.cpp)

find_package(snitch CONFIG REQUIRED)
target_link_libraries(main PRIVATE snitch::snitch)