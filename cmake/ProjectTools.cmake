# Clang-Tidy
find_program(CLANG_TIDY "clang-tidy")
if(CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
endif()

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    message(STATUS "Building with coverage information")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Custom target to generate coverage report
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage
        COMMAND lcov --capture --directory . --output-file coverage/coverage.info
        COMMAND lcov --remove coverage/coverage.info '/usr/*' --output-file coverage/coverage.info
        COMMAND lcov --list coverage/coverage.info
        COMMAND genhtml coverage/coverage.info --output-directory coverage
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report"
    )
endif()
