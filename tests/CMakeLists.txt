# Enable gtest_discover_tests
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(tests
    parser/test_block.cpp
    parser/test_expr.cpp
    parser/test_function_call_statement.cpp
    parser/test_function_declaration.cpp
    parser/test_function_definition.cpp
    parser/test_function_parameter.cpp
    parser/test_path_segment.cpp
    parser/test_path.cpp
    parser/test_return_statement.cpp
    parser/test_string.cpp
    parser/test_integer.cpp
)
target_link_libraries(tests PRIVATE project_defaults life-lang GTest::gtest GTest::gtest_main fmt::fmt Boost::spirit)

gtest_discover_tests(tests)