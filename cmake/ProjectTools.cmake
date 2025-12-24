# Ccache - Compiler cache
option(ENABLE_CCACHE "Enable ccache" OFF)

if(ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(WARNING "ccache not found")
    endif()
endif()

# Clang-Tidy
option(ENABLE_CLANG_TIDY "Enable Clang-Tidy" OFF)

if(ENABLE_CLANG_TIDY)
    message(STATUS "Enabling Clang-Tidy")

    # Try to find clang-tidy-cache first, fall back to clang-tidy
    find_program(CLANG_TIDY_CACHE "clang-tidy-cache")
    find_program(CLANG_TIDY "clang-tidy")

    set(CLANG_TIDY_OPTIONS
        -warnings-as-errors=*,-misc-include-cleaner
        -extra-arg=-Wno-unknown-warning-option
        -extra-arg=-Wno-ignored-optimization-argument
        -extra-arg=-Wno-unknown-pragmas
        -extra-arg=-Wno-unused-command-line-argument
    )

    if(CLANG_TIDY_CACHE)
        message(STATUS "Using clang-tidy-cache: ${CLANG_TIDY_CACHE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_CACHE};${CLANG_TIDY_OPTIONS}")
    elseif(CLANG_TIDY)
        message(STATUS "Using clang-tidy: ${CLANG_TIDY}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY};${CLANG_TIDY_OPTIONS}")
    else()
        message(FATAL_ERROR "Neither clang-tidy-cache nor clang-tidy found")
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
