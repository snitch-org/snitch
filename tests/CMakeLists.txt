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
        target_compile_options(${TARGET} PRIVATE "/F 8388608")
    endif()
endfunction()

function(configure_snitch_for_tests TARGET)
    target_compile_definitions(${TARGET} PUBLIC
        SNITCH_MAX_TEST_CASES=200
        SNITCH_MAX_EXPR_LENGTH=128
        SNITCH_MAX_MESSAGE_LENGTH=128
        SNITCH_MAX_TEST_NAME_LENGTH=128
        SNITCH_MAX_CAPTURE_LENGTH=128)
endfunction()

include(FetchContent)

set(DOCTEST_WITH_MAIN_IN_STATIC_LIB ON)
set(DOCTEST_NO_INSTALL ON)

FetchContent_Declare(doctest
                     GIT_REPOSITORY https://github.com/doctest/doctest.git
                     GIT_TAG        v2.4.9)
FetchContent_MakeAvailable(doctest)

set(RUNTIME_TEST_FILES
    ${PROJECT_SOURCE_DIR}/tests/testing_event.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/type_name.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_vector.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_string.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/string_utility.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_function.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/matchers.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/check.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/skip.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/capture.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/section.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/cli.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/registry.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/macros.cpp
    ${PROJECT_SOURCE_DIR}/tests/runtime_tests/regressions.cpp)

# Test snitch with doctest
add_executable(snitch_runtime_tests ${PROJECT_SOURCE_DIR}/src/snitch.cpp ${RUNTIME_TEST_FILES})
target_include_directories(snitch_runtime_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snitch_runtime_tests PRIVATE
    doctest::doctest
    doctest::doctest_with_main)
add_platform_definitions(snitch_runtime_tests)
configure_snitch_for_tests(snitch_runtime_tests)
target_compile_features(snitch_runtime_tests PUBLIC cxx_std_20)
target_compile_definitions(snitch_runtime_tests PUBLIC
    SNITCH_DEFINE_MAIN=0
    SNITCH_WITH_SHORTHAND_MACROS=0)

add_custom_target(snitch_runtime_tests_run
    COMMAND snitch_runtime_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(snitch_runtime_tests_run PROPERTIES EXCLUDE_FROM_ALL True)

# Test snitch with itself.
# Note: DO NOT target_link_libraries(snitch::snitch)!
# The pre-built library won't be built with configure_snitch_for_tests().
# Use add_dependencies() to ensure the proper build order, and include the
# sources directly in this project so they can be built with the wanted
# configuration.
add_executable(snitch_runtime_tests_self
    ${RUNTIME_TEST_FILES}
    ${SNITCH_SOURCES})
target_include_directories(snitch_runtime_tests_self PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/tests)
add_dependencies(snitch_runtime_tests_self snitch::snitch)
add_platform_definitions(snitch_runtime_tests_self)
configure_snitch_for_tests(snitch_runtime_tests_self)
target_compile_features(snitch_runtime_tests_self PUBLIC cxx_std_20)
target_compile_definitions(snitch_runtime_tests_self PUBLIC
    SNITCH_TEST_WITH_SNITCH)

add_custom_target(snitch_runtime_tests_self_run
    COMMAND snitch_runtime_tests_self
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(snitch_runtime_tests_self_run PROPERTIES EXCLUDE_FROM_ALL True)

# Test header-only snitch with itself.
# Note: Fine to use target_link_libraries(snitch::snitch-header-only);
# nothing is pre-built in this target, the configuration can be changed.
add_executable(snitch_runtime_tests_self_header_only
    ${PROJECT_SOURCE_DIR}/tests/testing.cpp
    ${RUNTIME_TEST_FILES})
target_include_directories(snitch_runtime_tests_self_header_only PRIVATE
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snitch_runtime_tests_self_header_only PRIVATE
    snitch::snitch-header-only)
add_platform_definitions(snitch_runtime_tests_self_header_only)
configure_snitch_for_tests(snitch_runtime_tests_self_header_only)
target_compile_features(snitch_runtime_tests_self_header_only PUBLIC cxx_std_20)
target_compile_definitions(snitch_runtime_tests_self_header_only PUBLIC
    SNITCH_TEST_WITH_SNITCH
    SNITCH_TEST_HEADER_ONLY)

add_custom_target(snitch_runtime_tests_self_header_only_run
    COMMAND snitch_runtime_tests_self_header_only
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(snitch_runtime_tests_self_header_only_run PROPERTIES EXCLUDE_FROM_ALL True)
