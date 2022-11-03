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

FetchContent_Declare(doctest
                     GIT_REPOSITORY https://github.com/doctest/doctest.git
                     GIT_TAG        v2.4.9)
FetchContent_MakeAvailable(doctest)

set(RUNTIME_TEST_FILES
  ${PROJECT_SOURCE_DIR}/src/snatch.cpp
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests/type_name.cpp
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_vector.cpp
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests/small_string.cpp)

add_executable(snatch_runtime_tests ${RUNTIME_TEST_FILES})
target_include_directories(snatch_runtime_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(snatch_runtime_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(snatch_runtime_tests PRIVATE doctest::doctest)
target_link_libraries(snatch_runtime_tests PRIVATE doctest::doctest_with_main)
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
