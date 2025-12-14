find_package(Catch2 CONFIG REQUIRED)

# Enable catch_discover_tests
include(Catch)

add_executable(tests)

target_sources(tests
    PRIVATE
        # Semantic analysis tests
        semantic/test_symbol_table.cpp
    semantic/test_declaration_collection.cpp
        
        # Unit tests - test semantic boundaries only (11 exposed rules)
        parser/test_assignment.cpp
        parser/test_binary_expr.cpp
        parser/test_block.cpp
        parser/test_break_statement.cpp
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
        parser/test_integer.cpp
        parser/test_internal_parse_errors.cpp
        parser/test_keywords.cpp
        parser/test_let_statement.cpp
        parser/test_match_expr.cpp
        parser/test_method_chaining.cpp
        parser/test_range_expr.cpp
        parser/test_string.cpp
        parser/test_struct_def.cpp
        parser/test_trait_bounds.cpp
        parser/test_trait_bounds_in_declarations.cpp
        parser/test_trait_def.cpp
        parser/test_trait_impl.cpp
        parser/test_tuple_pattern.cpp
        parser/test_type_alias.cpp
        parser/test_unary_expr.cpp
        parser/test_unit_type.cpp
        parser/test_where_clause.cpp
        parser/test_while_expr.cpp
        
        # Integration tests - test complete parse_module workflows
        integration/test_parse_module_integration.cpp
        integration/test_readme_example.cpp
)

target_link_libraries(tests
    PRIVATE
        life-lang
        Catch2::Catch2WithMain
        fmt::fmt
)

# Reuse precompiled headers from life-lang library
catch_discover_tests(tests)