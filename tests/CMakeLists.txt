function(add_platform_definitions TARGET)
  target_compile_features(${TARGET} INTERFACE cxx_std_20)
  if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
      target_compile_definitions(${TARGET} PRIVATE SNATCH_PLATFORM_WASM)
      target_compile_definitions(${TARGET} PRIVATE SNATCH_COMPILER_EMSCRIPTEN)
  elseif (APPLE)
      target_compile_definitions(${TARGET} PRIVATE SNATCH_PLATFORM_OSX)
  elseif (UNIX)
      target_compile_definitions(${TARGET} PRIVATE SNATCH_PLATFORM_LINUX)
  elseif (WIN32)
      target_compile_definitions(${TARGET} PRIVATE SNATCH_PLATFORM_WINDOWS)
      if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
          target_compile_definitions(${TARGET} PRIVATE SNATCH_COMPILER_MSVC)
      endif()
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(${TARGET} PRIVATE -Wall)
      target_compile_options(${TARGET} PRIVATE -Wextra)
      target_compile_options(${TARGET} PRIVATE -Werror)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
      target_compile_options(${TARGET} PRIVATE -Wall)
      target_compile_options(${TARGET} PRIVATE -Wextra)
      target_compile_options(${TARGET} PRIVATE -Werror)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${TARGET} PRIVATE /W4)
      target_compile_options(${TARGET} PRIVATE /WX)
      target_compile_options(${TARGET} PRIVATE /EHs)
    endif()
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
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests/capture.cpp
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests/section.cpp)

# Test snatch with doctest
add_executable(snatch_runtime_tests ${PROJECT_SOURCE_DIR}/src/snatch.cpp ${RUNTIME_TEST_FILES})
target_include_directories(snatch_runtime_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snatch_runtime_tests PRIVATE
  doctest::doctest
  doctest::doctest_with_main)
add_platform_definitions(snatch_runtime_tests)
target_compile_features(snatch_runtime_tests PUBLIC cxx_std_20)
target_compile_definitions(snatch_runtime_tests PUBLIC
  SNATCH_DEFINE_MAIN=0
  SNATCH_WITH_SHORTHAND_MACROS=0)

add_custom_target(snatch_runtime_tests_run
  COMMAND snatch_runtime_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${RUNTIME_TEST_FILES}
)
set_target_properties(snatch_runtime_tests_run PROPERTIES EXCLUDE_FROM_ALL True)

# Test snatch with itself
add_executable(snatch_runtime_tests_self ${PROJECT_SOURCE_DIR}/src/snatch.cpp ${RUNTIME_TEST_FILES})
target_include_directories(snatch_runtime_tests_self PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/tests)
add_platform_definitions(snatch_runtime_tests_self)
target_compile_features(snatch_runtime_tests_self PUBLIC cxx_std_20)
target_compile_definitions(snatch_runtime_tests_self PUBLIC SNATCH_TEST_WITH_SNATCH)

add_custom_target(snatch_runtime_tests_self_run
  COMMAND snatch_runtime_tests_self
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${RUNTIME_TEST_FILES}
)
set_target_properties(snatch_runtime_tests_self_run PROPERTIES EXCLUDE_FROM_ALL True)

# Test header-only snatch with itself
add_executable(snatch_runtime_tests_self_header_only ${PROJECT_SOURCE_DIR}/tests/testing.cpp ${RUNTIME_TEST_FILES})
target_include_directories(snatch_runtime_tests_self_header_only PRIVATE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/tests)
add_platform_definitions(snatch_runtime_tests_self_header_only)
target_compile_features(snatch_runtime_tests_self_header_only PUBLIC cxx_std_20)
target_compile_definitions(snatch_runtime_tests_self_header_only PUBLIC
  SNATCH_TEST_WITH_SNATCH
  SNATCH_TEST_HEADER_ONLY)

add_custom_target(snatch_runtime_tests_self_header_only_run
  COMMAND snatch_runtime_tests_self_header_only
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${RUNTIME_TEST_FILES}
)
set_target_properties(snatch_runtime_tests_self_header_only_run PROPERTIES EXCLUDE_FROM_ALL True)
