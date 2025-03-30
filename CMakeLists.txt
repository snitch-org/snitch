cmake_minimum_required(VERSION 3.14)

project(snitch LANGUAGES CXX VERSION 1.3.2)

# Maximum lengths.
set(SNITCH_MAX_TEST_CASES           5000 CACHE STRING "Maximum number of test cases in a test application.")
set(SNITCH_MAX_NESTED_SECTIONS      8    CACHE STRING "Maximum depth of nested sections in a test case.")
set(SNITCH_MAX_EXPR_LENGTH          1024 CACHE STRING "Maximum length of a printed expression when reporting failure.")
set(SNITCH_MAX_MESSAGE_LENGTH       1024 CACHE STRING "Maximum length of error or status messages.")
set(SNITCH_MAX_TEST_NAME_LENGTH     1024 CACHE STRING "Maximum length of a test case name.")
set(SNITCH_MAX_TAG_LENGTH           256  CACHE STRING "Maximum length of a test tag.")
set(SNITCH_MAX_CAPTURES             8    CACHE STRING "Maximum number of captured expressions in a test case.")
set(SNITCH_MAX_CAPTURE_LENGTH       256  CACHE STRING "Maximum length of a captured expression.")
set(SNITCH_MAX_UNIQUE_TAGS          1024 CACHE STRING "Maximum number of unique tags in a test application.")
set(SNITCH_MAX_COMMAND_LINE_ARGS    1024 CACHE STRING "Maximum number of command line arguments to a test application.")
set(SNITCH_MAX_REGISTERED_REPORTERS 8    CACHE STRING "Maximum number of registered reporter that can be selected from the command line.")
set(SNITCH_MAX_PATH_LENGTH          1024 CACHE STRING "Maximum length of a file path when writing output to file.")
set(SNITCH_MAX_REPORTER_SIZE_BYTES  128  CACHE STRING "Maximum size (in bytes) of a reporter object.")

# Feature toggles.
set(SNITCH_ENABLE                          ON  CACHE BOOL "Enable/disable snitch at build time.")
set(SNITCH_DEFINE_MAIN                     ON  CACHE BOOL "Define main() in snitch -- disable to provide your own main() function.")
set(SNITCH_WITH_EXCEPTIONS                 ON  CACHE BOOL "Use exceptions in snitch implementation -- will be forced OFF if exceptions are not available.")
set(SNITCH_WITH_MULTITHREADING             ON  CACHE BOOL "Make the testing framework thread-safe -- disable if multithreading is not needed.")
set(SNITCH_WITH_TIMINGS                    ON  CACHE BOOL "Measure the time taken by each test case -- disable to speed up tests.")
set(SNITCH_WITH_SHORTHAND_MACROS           ON  CACHE BOOL "Use short names for test macros -- disable if this causes conflicts.")
set(SNITCH_CONSTEXPR_FLOAT_USE_BITCAST     ON  CACHE BOOL "Use std::bit_cast if available to implement exact constexpr float-to-string conversion.")
set(SNITCH_APPEND_TO_CHARS                 ON  CACHE BOOL "Use std::to_chars for string conversions -- disable for greater compatability with a slight performance cost.")
set(SNITCH_DEFAULT_WITH_COLOR              ON  CACHE BOOL "Enable terminal colors by default -- can also be controlled by command line interface.")
set(SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS OFF CACHE BOOL "Enable expression decomposition even for successful assertions -- more expensive.")
set(SNITCH_WITH_ALL_REPORTERS              ON  CACHE BOOL "Allow all built-in reporters to be selected from the command line -- disable for faster compilation.")
set(SNITCH_WITH_TEAMCITY_REPORTER          OFF CACHE BOOL "Allow the TeamCity reporter to be selected from the command line -- enable if needed.")
set(SNITCH_WITH_CATCH2_XML_REPORTER        OFF CACHE BOOL "Allow the Catch2 XML reporter to be selected from the command line -- enable if needed.")

# Building and packaging options; not part of the library API.
set(SNITCH_HEADER_ONLY        OFF CACHE BOOL "Create a single-header header-only version of snitch.")
set(SNITCH_UNITY_BUILD        ON  CACHE BOOL "Build sources as single file instead of separate files (faster full build).")
set(SNITCH_DO_TEST            OFF CACHE BOOL "Build tests.")
set(SNITCH_USE_SYSTEM_DOCTEST OFF CACHE BOOL "Assume doctest is already installed, do not download it (used in tests only).")
set(SNITCH_INSTALL_DOCS       ON  CACHE BOOL "Install docs.")

# Figure out git hash, if any
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    RESULT_VARIABLE GIT_COMMAND_SUCCESS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

if (GIT_COMMAND_SUCCESS EQUAL 0)
    set(SNITCH_FULL_VERSION "${PROJECT_VERSION}.${GIT_COMMIT_HASH}")
else()
    set(SNITCH_FULL_VERSION "${PROJECT_VERSION}")
endif()

if (NOT SNITCH_HEADER_ONLY AND DEFINED BUILD_SHARED_LIBS)
    set(SNITCH_SHARED_LIBRARY ${BUILD_SHARED_LIBS})
endif()

# Create configure file to store CMake build parameter
configure_file("${PROJECT_SOURCE_DIR}/include/snitch/snitch_config.hpp.config"
               "${PROJECT_BINARY_DIR}/snitch/snitch_config.hpp")

set(SNITCH_INCLUDES
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_any.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_append.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_capture.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_cli.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_concepts.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_console.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_error_handling.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_expression.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_file.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_fixed_point.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_function.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_check.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_check_base.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_consteval.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_constexpr.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_exceptions.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_misc.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_reporter.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_test_case.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_utility.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_macros_warnings.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_main.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_matcher.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_registry.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_reporter_catch2_xml.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_reporter_console.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_reporter_teamcity.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_section.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_string.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_string_utility.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_test_data.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_time.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_type_id.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_type_name.hpp
    ${PROJECT_SOURCE_DIR}/include/snitch/snitch_vector.hpp
    ${PROJECT_BINARY_DIR}/snitch/snitch_config.hpp)

set(SNITCH_SOURCES_INDIVIDUAL
    ${PROJECT_SOURCE_DIR}/src/snitch_append.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_capture.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_cli.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_console.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_error_handling.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_file.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_main.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_matcher.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_reporter_catch2_xml.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_reporter_console.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_reporter_teamcity.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_section.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_string_utility.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_test_data.cpp
    ${PROJECT_SOURCE_DIR}/src/snitch_time.cpp)

if (SNITCH_ENABLE)
    if (SNITCH_UNITY_BUILD)
        set(SNITCH_SOURCES ${PROJECT_SOURCE_DIR}/src/snitch.cpp)
    else()
        set(SNITCH_SOURCES ${SNITCH_SOURCES_INDIVIDUAL})
    endif()
else()
    set(SNITCH_SOURCES ${PROJECT_SOURCE_DIR}/src/snitch_main.cpp)
endif()

function(configure_snitch_exports TARGET)
    if (BUILD_SHARED_LIBS)
        target_compile_definitions(${TARGET} PRIVATE SNITCH_EXPORTS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR MINGW)
            # Nothing to do; default is already to hide symbols unless exported.
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
            CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
            CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            # Set default visibility to "hidden" so only exported symbols are visible.
            target_compile_options(${TARGET} PRIVATE -fvisibility=hidden)
            target_compile_options(${TARGET} PRIVATE -fvisibility-inlines-hidden)
        endif()
    endif()
endfunction()

if (NOT SNITCH_HEADER_ONLY)
    # Build as a standard library (static or dynamic) with header.
    set(SNITCH_TARGET_NAME snitch)

    add_library(${SNITCH_TARGET_NAME} ${SNITCH_INCLUDES} ${SNITCH_SOURCES})
    target_compile_features(${SNITCH_TARGET_NAME} PUBLIC cxx_std_20)
    target_include_directories(${SNITCH_TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>)

    configure_snitch_exports(${SNITCH_TARGET_NAME})

    install(
        FILES ${SNITCH_INCLUDES}
        DESTINATION include/snitch)
else()
    # Build as a header-only library.
    set(SNITCH_TARGET_NAME snitch-header-only)

    find_package(Python3)

    add_custom_command(
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/make_snitch_all.py" "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}"
        VERBATIM
        OUTPUT ${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp
        DEPENDS
            ${SNITCH_INCLUDES}
            ${PROJECT_SOURCE_DIR}/src/snitch.cpp
            ${SNITCH_SOURCES_INDIVIDUAL}
            ${PROJECT_SOURCE_DIR}/make_snitch_all.py)

    add_custom_target(snitch-header-only-impl ALL
        DEPENDS ${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp)

    add_library(${SNITCH_TARGET_NAME} INTERFACE)
    add_dependencies(${SNITCH_TARGET_NAME} snitch-header-only-impl)
    target_sources(${SNITCH_TARGET_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp>
        $<INSTALL_INTERFACE:include/snitch/snitch_all.hpp>)
    target_compile_features(${SNITCH_TARGET_NAME} INTERFACE cxx_std_20)
    target_include_directories(${SNITCH_TARGET_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>)

    install(
        FILES ${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp
        DESTINATION include/snitch)
endif()

if (SNITCH_INSTALL_DOCS)
    install(
        FILES ${PROJECT_SOURCE_DIR}/LICENSE
        DESTINATION doc/snitch)
endif()

# Common properties
add_library(snitch::${SNITCH_TARGET_NAME} ALIAS ${SNITCH_TARGET_NAME})
set_target_properties(${SNITCH_TARGET_NAME} PROPERTIES EXPORT_NAME snitch::${SNITCH_TARGET_NAME})

# Setup CMake config file
install(TARGETS ${SNITCH_TARGET_NAME} EXPORT ${SNITCH_TARGET_NAME}-targets)

install(EXPORT ${SNITCH_TARGET_NAME}-targets
    DESTINATION lib/cmake/snitch COMPONENT Development)

export(EXPORT ${SNITCH_TARGET_NAME}-targets)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/snitch-config.cmake.in"
    "${PROJECT_BINARY_DIR}/snitch-config.cmake"
    INSTALL_DESTINATION lib
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    NO_SET_AND_CHECK_MACRO)

install(FILES
    "${PROJECT_BINARY_DIR}/snitch-config.cmake"
    DESTINATION lib/cmake/snitch COMPONENT Development)

# Setup tests
if (SNITCH_DO_TEST AND SNITCH_ENABLE)
    enable_testing()

    # We need to use a different snitch configuration for tests, so we can't reuse
    # the library that was built above. Create a copy.
    function(configure_snitch_for_tests TARGET CHOSEN_INTERFACE)
        # Make sure all lengths are different, so we can catch mismatches.
        target_compile_definitions(${TARGET} ${CHOSEN_INTERFACE}
            SNITCH_MAX_TEST_CASES=200
            SNITCH_MAX_EXPR_LENGTH=128
            SNITCH_MAX_MESSAGE_LENGTH=129
            SNITCH_MAX_TEST_NAME_LENGTH=130
            SNITCH_MAX_CAPTURE_LENGTH=131
            SNITCH_MAX_REPORTER_SIZE_BYTES=16
            SNITCH_DEFINE_MAIN=0)
    endfunction()

    if (NOT SNITCH_HEADER_ONLY)
        add_library(snitch-testlib ${SNITCH_INCLUDES} ${SNITCH_SOURCES})

        target_compile_features(snitch-testlib PUBLIC cxx_std_20)
        target_include_directories(snitch-testlib PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/src
            ${PROJECT_BINARY_DIR})

        configure_snitch_exports(snitch-testlib)
        configure_snitch_for_tests(snitch-testlib PUBLIC)
    else()
        add_library(snitch-testlib INTERFACE)
        add_dependencies(snitch-testlib snitch-header-only-impl)
        target_sources(snitch-testlib INTERFACE
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/snitch/snitch_all.hpp>
            $<INSTALL_INTERFACE:include/snitch/snitch_all.hpp>)

        target_compile_features(snitch-testlib INTERFACE cxx_std_20)
        target_include_directories(snitch-testlib INTERFACE ${PROJECT_BINARY_DIR})

        configure_snitch_for_tests(snitch-testlib INTERFACE)
        target_compile_definitions(snitch-testlib INTERFACE SNITCH_TEST_HEADER_ONLY)
    endif()

    # This dependency is not strictly needed, but it makes developing easier:
    # if the "real" library fails to build, we won't try to compile the version
    # used in the tests.
    add_dependencies(snitch-testlib ${SNITCH_TARGET_NAME})

    add_subdirectory(tests)
endif()
