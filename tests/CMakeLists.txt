find_package(doctest CONFIG REQUIRED)

add_executable(tests)

target_sources(tests
    PRIVATE
        # Main test runner
        test_main.cpp
        
        # Unit tests - test semantic boundaries only (11 exposed rules)
        parser/test_array_literal.cpp
        parser/test_array_type.cpp
        parser/test_assignment.cpp
        parser/test_binary_expr.cpp
        parser/test_bitwise_ops.cpp
        parser/test_block.cpp
        parser/test_bool_literal.cpp
        parser/test_break_statement.cpp
        parser/test_cast_expr.cpp
        parser/test_char.cpp
        parser/test_comment.cpp
        parser/test_continue_statement.cpp
        parser/test_diagnostics.cpp
        parser/test_enum_def.cpp
        parser/test_expr.cpp
        parser/test_field_access.cpp
        parser/test_float.cpp
        parser/test_for_expr.cpp
        parser/test_func_def.cpp
        parser/test_func_type.cpp
        parser/test_if_expr.cpp
        parser/test_impl_block.cpp
        parser/test_import.cpp
        parser/test_index_expr.cpp
        parser/test_integer.cpp
        parser/test_internal_parse_errors.cpp
        parser/test_keywords.cpp
        parser/test_let_statement.cpp
        parser/test_match_expr.cpp
        parser/test_method_chaining.cpp
        parser/test_pub_impl_method.cpp
        parser/test_pub_struct_field.cpp
        parser/test_range_expr.cpp
        parser/test_string.cpp
        parser/test_struct_def.cpp
        parser/test_trait_bounds.cpp
        parser/test_trait_bounds_in_declarations.cpp
        parser/test_trait_def.cpp
        parser/test_trait_impl.cpp
        parser/test_tuple_literal.cpp
        parser/test_tuple_pattern.cpp
        parser/test_tuple_type.cpp
        parser/test_type_alias.cpp
        parser/test_unary_expr.cpp
        parser/test_unit_type.cpp
        parser/test_where_clause.cpp
        parser/test_while_expr.cpp
        
        # Integration tests - test complete parse_module workflows
        integration/test_array_type_in_functions.cpp
        integration/test_binary_literals.cpp
        integration/test_bitwise_ops.cpp
        integration/test_bool_literals.cpp
        integration/test_hex_literals.cpp
        integration/test_octal_literals.cpp
        integration/test_parse_module_integration.cpp
        integration/test_readme_example.cpp
        integration/test_tuple_types_and_literals.cpp
)

target_link_libraries(tests
    PRIVATE
        life-lang
        doctest::doctest
)

include(doctest)
doctest_discover_tests(tests)