cmake_minimum_required(VERSION 3.31)

project(life-lang
  VERSION 0.1.0
  DESCRIPTION "A modern compiler for the life-lang programming language"
  LANGUAGES CXX
)

# GCC 14+ workaround for clang-tidy compatibility
block(SCOPE_FOR VARIABLES)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
      message(NOTICE "GCC 14+ detected, CMAKE_CXX_SCAN_FOR_MODULES is set to OFF")
      # These flags are not recognized by clang-tidy
      set(CMAKE_CXX_SCAN_FOR_MODULES OFF PARENT_SCOPE)
    endif()
  endif()
endblock()

# Default C++ project settings
include(cmake/ProjectDefaults.cmake)
include(cmake/ProjectTools.cmake)

# Find packages
find_package(Boost CONFIG REQUIRED COMPONENTS spirit fusion)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# lib and binaries
add_subdirectory(src)

# Testing configuration
include(CTest)
if(BUILD_TESTING)
  set(CTEST_NO_TESTS_ACTION "ERROR")
  set(CTEST_OUTPUT_ON_FAILURE TRUE)
  add_subdirectory(tests)
endif()
