cmake_minimum_required(VERSION 3.28)

project(life-lang LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15)
        message(NOTICE "GCC 14 detected, CMAKE_CXX_SCAN_FOR_MODULES is set to OFF")

        # Have to turn this off to stop gcc/ninja adding -fmodules-ts -fmoudle-mapper and -fdeps-format
        # These flags are not recognized by the clang-tidy, it throws "unknow argument"
        # and I don't know how to make clang-tidy ignore it
        set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    endif()
endif()

# Default C++ project settings
include(cmake/ProjectDefaults.cmake)
include(cmake/ProjectTools.cmake)

# Find packages
find_package(Boost CONFIG REQUIRED COMPONENTS spirit fusion)
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
