find_package(Catch2 CONFIG REQUIRED)

# Enable catch_discover_tests
include(Catch)

add_executable(tests)

target_sources(tests
    PRIVATE
        parser/test_block.cpp
        parser/test_diagnostics.cpp
        parser/test_expr.cpp
        parser/test_function_call_statement.cpp
        parser/test_function_declaration.cpp
        parser/test_function_definition.cpp
        parser/test_function_parameter.cpp
        parser/test_integer.cpp
        parser/test_internal_parse_errors.cpp
        parser/test_keywords.cpp
        parser/test_module.cpp
        parser/test_path_segment.cpp
        parser/test_path.cpp
        parser/test_return_statement.cpp
        parser/test_string.cpp
)

target_link_libraries(tests
    PRIVATE
        life-lang
        Catch2::Catch2WithMain
        fmt::fmt
)

catch_discover_tests(tests)