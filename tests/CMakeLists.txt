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

set(RUNTIME_TEST_FILES
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests_type_name.cpp
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests_small_vector.cpp
  ${PROJECT_SOURCE_DIR}/tests/runtime_tests_small_string.cpp)

add_executable(snatch_runtime_tests ${RUNTIME_TEST_FILES})
target_link_libraries(snatch_runtime_tests PRIVATE snatch::snatch)
add_platform_definitions(snatch_runtime_tests)

add_custom_target(snatch_runtime_tests_run
  COMMAND snatch_runtime_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${RUNTIME_TEST_FILES}
)
set_target_properties(snatch_runtime_tests_run PROPERTIES EXCLUDE_FROM_ALL True)
