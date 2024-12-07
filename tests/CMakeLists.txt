cmake_minimum_required(VERSION 3.20)

function(add_platform_definitions TARGET)
    target_compile_features(${TARGET} INTERFACE cxx_std_20)
    if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
        target_compile_definitions(${TARGET} PRIVATE SNITCH_PLATFORM_WASM)
        target_compile_definitions(${TARGET} PRIVATE SNITCH_COMPILER_EMSCRIPTEN)
    elseif (APPLE)
        target_compile_definitions(${TARGET} PRIVATE SNITCH_PLATFORM_OSX)
    elseif (UNIX)
        target_compile_definitions(${TARGET} PRIVATE SNITCH_PLATFORM_LINUX)
    elseif (WIN32)
        target_compile_definitions(${TARGET} PRIVATE SNITCH_PLATFORM_WINDOWS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_definitions(${TARGET} PRIVATE SNITCH_COMPILER_MSVC)
        endif()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TARGET} PRIVATE -Wall)
        target_compile_options(${TARGET} PRIVATE -Wextra)
        target_compile_options(${TARGET} PRIVATE -Werror)
        target_compile_options(${TARGET} PRIVATE -pedantic)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        target_compile_options(${TARGET} PRIVATE -Wall)
        target_compile_options(${TARGET} PRIVATE -Wextra)
        target_compile_options(${TARGET} PRIVATE -Werror)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${TARGET} PRIVATE /W4)
        target_compile_options(${TARGET} PRIVATE /WX)
        target_compile_options(${TARGET} PRIVATE /EHs)
        # Increase default stack size to match default for Linux
        target_link_options(${TARGET} PRIVATE /STACK:8388608)
    endif()

    if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
        # Increase default stack size to match default for Linux
        target_link_options(${TARGET} PRIVATE "SHELL:-s STACK_SIZE=8388608")
    endif()
endfunction()

set(TEST_UTILITY_FILES
    ${PROJECT_SOURCE_DIR}/tests/testing.cpp
    ${PROJECT_SOURCE_DIR}/tests/testing_event.cpp
    ${PROJECT_SOURCE_DIR}/tests/testing_reporters.cpp)

set(RUNTIME_TEST_FILES
    ${TEST_UTILITY_FILES}
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/any.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/capture.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/check.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/cli.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/function_ref.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/macros.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/matchers.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/registry.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/regressions.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/section.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/skip.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_string.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_vector.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/string_utility.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/type_id.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/type_name.cpp)

set(APPROVAL_TEST_FILES
    ${TEST_UTILITY_FILES}
    ${PROJECT_SOURCE_DIR}/tests/approval_tests/reporter_catch2_xml.cpp
    ${PROJECT_SOURCE_DIR}/tests/approval_tests/reporter_console.cpp
    ${PROJECT_SOURCE_DIR}/tests/approval_tests/reporter_teamcity.cpp)

if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    # For Emcripten, we need the working directory to be where the binaries are created,
    # because Emscripten will generate *.data files there that we need to load.
    # We don't have access to the filesystem otherwise.
    set(APPROVAL_TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(RUNTIME_TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else()
    set(APPROVAL_TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/approval_tests)
    set(RUNTIME_TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/runtime_tests)
endif()

function(copy_shared_library TARGET)
    if (BUILD_SHARED_LIBS AND WIN32)
        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:snitch-testlib> $<TARGET_FILE_DIR:${TARGET}>)
    endif()
endfunction()

if (SNITCH_USE_SYSTEM_DOCTEST)
    find_package(doctest REQUIRED)
else()
    include(FetchContent)

    set(DOCTEST_NO_INSTALL ON)

    FetchContent_Declare(doctest
                         GIT_REPOSITORY https://github.com/doctest/doctest.git
                         GIT_TAG        v2.4.9)
    FetchContent_MakeAvailable(doctest)
endif()

# Test snitch with doctest.
add_executable(snitch_runtime_tests
    ${RUNTIME_TEST_FILES})
target_include_directories(snitch_runtime_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snitch_runtime_tests PRIVATE
    snitch-testlib
    doctest::doctest)
add_platform_definitions(snitch_runtime_tests)
target_compile_features(snitch_runtime_tests PUBLIC cxx_std_20)
target_compile_definitions(snitch_runtime_tests PUBLIC
    SNITCH_WITH_SHORTHAND_MACROS=0)
copy_shared_library(snitch_runtime_tests)

add_custom_target(snitch_runtime_tests_run
    COMMAND snitch_runtime_tests
    WORKING_DIRECTORY ${RUNTIME_TEST_WORKING_DIRECTORY}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(snitch_runtime_tests_run PROPERTIES EXCLUDE_FROM_ALL True)

# Test snitch with doctest (approval tests).
add_executable(snitch_approval_tests
    ${APPROVAL_TEST_FILES})
target_include_directories(snitch_approval_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snitch_approval_tests PRIVATE
    snitch-testlib
    doctest::doctest)
add_platform_definitions(snitch_approval_tests)
target_compile_features(snitch_approval_tests PUBLIC cxx_std_20)
target_compile_definitions(snitch_approval_tests PUBLIC
    SNITCH_WITH_SHORTHAND_MACROS=0)
copy_shared_library(snitch_approval_tests)

if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    # Add approval test data to the preload-file list
    target_link_options(snitch_approval_tests PUBLIC
        "SHELL:--preload-file ${PROJECT_SOURCE_DIR}/tests/approval_tests/data@data")
endif()

add_custom_target(snitch_approval_tests_run
    COMMAND snitch_approval_tests
    WORKING_DIRECTORY ${APPROVAL_TEST_WORKING_DIRECTORY}
    SOURCES ${APPROVAL_TEST_FILES})
set_target_properties(snitch_approval_tests_run PROPERTIES EXCLUDE_FROM_ALL True)

# Test snitch with itself.
add_executable(snitch_runtime_tests_self
    ${RUNTIME_TEST_FILES})
target_include_directories(snitch_runtime_tests_self PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snitch_runtime_tests_self PRIVATE snitch-testlib)
add_platform_definitions(snitch_runtime_tests_self)
target_compile_features(snitch_runtime_tests_self PUBLIC cxx_std_20)
target_compile_definitions(snitch_runtime_tests_self PUBLIC
    SNITCH_TEST_WITH_SNITCH)
copy_shared_library(snitch_runtime_tests_self)

add_custom_target(snitch_runtime_tests_self_run
    COMMAND snitch_runtime_tests_self
    WORKING_DIRECTORY ${RUNTIME_TEST_WORKING_DIRECTORY}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(snitch_runtime_tests_self_run PROPERTIES EXCLUDE_FROM_ALL True)
