cmake_minimum_required(VERSION 3.28)

project(life-lang LANGUAGES CXX)

# Default C++ project settings
include(cmake/ProjectDefaults.cmake)

# Clang-Tidy
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

# Find packages
find_package(Boost CONFIG REQUIRED COMPONENTS algorithm spirit)
find_package(fmt CONFIG REQUIRED)

# Add executable
add_executable(lifec src/main.cpp)
target_link_libraries(lifec PRIVATE project_defaults Boost::algorithm Boost::spirit fmt::fmt)
