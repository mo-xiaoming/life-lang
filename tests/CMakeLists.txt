# Enable gtest_discover_tests
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(tests test_spirit.cpp)
target_link_libraries(tests PRIVATE project_defaults life-lang GTest::gtest GTest::gtest_main fmt::fmt Boost::spirit Boost::hana)

gtest_discover_tests(tests)