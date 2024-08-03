cmake_minimum_required(VERSION 3.28)

project(life-lang LANGUAGES CXX)

# Default C++ project settings
include(cmake/ProjectDefaults.cmake)
include(cmake/ProjectTools.cmake)

# Find packages
find_package(Boost CONFIG REQUIRED COMPONENTS algorithm spirit)
find_package(fmt CONFIG REQUIRED)

# lib and binaries
add_subdirectory(src)

# Enable testing
enable_testing()

# Enable gtest_discover_tests
include(CTest)
set(CTEST_NO_TESTS_ACTION "ERROR")
set(CTEST_OUTPUT_ON_FAILURE TRUE)
add_subdirectory(tests)
