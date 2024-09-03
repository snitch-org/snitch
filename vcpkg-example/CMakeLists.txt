cmake_minimum_required(VERSION 3.14)

project(vcpkg-example)

add_executable(vcpkg-example main.cpp)

find_package(snitch CONFIG REQUIRED)
target_link_libraries(vcpkg-example PRIVATE snitch::snitch)
