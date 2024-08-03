# Enable gtest_discover_tests
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(tests
    test_spirit.cpp
)
target_link_libraries(tests PRIVATE project_defaults GTest::gtest GTest::gtest_main)

gtest_discover_tests(tests)