# Clang-Tidy
option(ENABLE_CLANG_TIDY "Enable Clang-Tidy" ON)

if(ENABLE_CLANG_TIDY)
    message(STATUS "Enabling Clang-Tidy")
    find_program(CLANG_TIDY "clang-tidy")

    set(CLANG_TIDY_OPTIONS
        -warnings-as-errors=*,-misc-include-cleaner
        -extra-arg=-Wno-unknown-warning-option
        -extra-arg=-Wno-ignored-optimization-argument
        -extra-arg=-Wno-unknown-pragmas
        -extra-arg=-Wno-unused-command-line-argument
    )

    if(CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY};${CLANG_TIDY_OPTIONS}")
    else()
        message(FATAL_ERROR "clang-tidy not found")
    endif()
endif()

# Add coverage flags
option(ENABLE_COVERAGE "Enable coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_COMPILER_IS_GNUCXX)
        message(STATUS "Building with coverage information")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    else()
        message(FATAL_ERROR "Coverage is only supported with GCC")
    endif()
endif()
