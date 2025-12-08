#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Statement;

// PARSE_TEST generates check_parse() function and Statement_Params type
PARSE_TEST(Statement, statement)

namespace {
// === Valid Let Statements ===

// Simple let binding
constexpr auto k_simple_let_should_succeed = true;
constexpr auto k_simple_let_input = "let x = 42;";
inline auto const k_simple_let_expected =
    test_json::let_statement(test_json::simple_pattern("x"), test_json::integer(42));

// Let with type annotation
constexpr auto k_let_with_type_should_succeed = true;
constexpr auto k_let_with_type_input = "let x: I32 = 42;";
inline auto const k_let_with_type_expected = test_json::let_statement(
    test_json::simple_pattern("x"),
    test_json::integer(42),
    false,
    test_json::type_name("I32")
);

// Let mut binding
constexpr auto k_let_mut_should_succeed = true;
constexpr auto k_let_mut_input = "let mut count = 0;";
inline auto const k_let_mut_expected =
    test_json::let_statement(test_json::simple_pattern("count"), test_json::integer(0), true);

// Let mut with type annotation
constexpr auto k_let_mut_with_type_should_succeed = true;
constexpr auto k_let_mut_with_type_input = "let mut counter: I32 = 100;";
inline auto const k_let_mut_with_type_expected = test_json::let_statement(
    test_json::simple_pattern("counter"),
    test_json::integer(100),
    true,
    test_json::type_name("I32")
);

// Let with variable initializer
constexpr auto k_let_with_variable_should_succeed = true;
constexpr auto k_let_with_variable_input = "let y = x;";
inline auto const k_let_with_variable_expected = fmt::format(
    R"({{
  "Let_Statement": {{
    "is_mut": false,
    "pattern": {{
      "Simple_Pattern": {{
        "name": "y"
      }}
    }},
    "type": null,
    "value": {}
  }}
}})",
    test_json::var_name("x")
);

// Let with expression initializer
constexpr auto k_let_with_expression_should_succeed = true;
constexpr auto k_let_with_expression_input = "let result = x + 10;";
inline auto const k_let_with_expression_expected = test_json::let_statement(
    test_json::simple_pattern("result"),
    test_json::binary_expr("+", test_json::var_name("x"), test_json::integer(10))
);

// Let with function call initializer
constexpr auto k_let_with_function_call_should_succeed = true;
constexpr auto k_let_with_function_call_input = "let value = calculate();";
inline auto const k_let_with_function_call_expected = test_json::let_statement(
    test_json::simple_pattern("value"),
    test_json::function_call(test_json::var_name("calculate"), {})
);

// Let with tuple pattern
constexpr auto k_let_with_tuple_pattern_should_succeed = true;
constexpr auto k_let_with_tuple_pattern_input = "let (x, y) = point;";
inline auto const k_let_with_tuple_pattern_expected = test_json::let_statement(
    test_json::tuple_pattern({test_json::simple_pattern("x"), test_json::simple_pattern("y")}),
    test_json::var_name("point")
);

// Let with qualified type
constexpr auto k_let_with_qualified_type_should_succeed = true;
constexpr auto k_let_with_qualified_type_input = "let name: Std.String = create_name();";
inline auto const k_let_with_qualified_type_expected = test_json::let_statement(
    test_json::simple_pattern("name"),
    test_json::function_call(test_json::var_name("create_name"), {}),
    false,
    test_json::type_name_path({"Std", "String"})
);

// Let with string initializer
constexpr auto k_let_with_string_should_succeed = true;
constexpr auto k_let_with_string_input = R"(let msg = "hello";)";
inline auto const k_let_with_string_expected =
    test_json::let_statement(test_json::simple_pattern("msg"), test_json::string(R"(\"hello\")"));

// === Invalid Let Statements ===

// Missing initializer
constexpr auto k_let_missing_init_should_succeed = false;
constexpr auto k_let_missing_init_input = "let x;";

// Missing pattern
constexpr auto k_let_missing_pattern_should_succeed = false;
constexpr auto k_let_missing_pattern_input = "let = 42;";

// Missing semicolon
constexpr auto k_let_missing_semicolon_should_succeed = false;
constexpr auto k_let_missing_semicolon_input = "let x = 42";

// Literal pattern - parser accepts but semantic analysis should reject (can't assign to literal)
constexpr auto k_let_invalid_pattern_should_succeed = true;
constexpr auto k_let_invalid_pattern_input = "let 123 = x;";

// Missing equals sign
constexpr auto k_let_missing_equals_should_succeed = false;
constexpr auto k_let_missing_equals_input = "let x 42;";

}  // namespace

TEST_CASE("Parse Let_Statement", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Statement_Params>({
          {"simple let", k_simple_let_input, k_simple_let_expected, k_simple_let_should_succeed},
          {"let with type", k_let_with_type_input, k_let_with_type_expected, k_let_with_type_should_succeed},
          {"let mut", k_let_mut_input, k_let_mut_expected, k_let_mut_should_succeed},
          {"let mut with type",
           k_let_mut_with_type_input,
           k_let_mut_with_type_expected,
           k_let_mut_with_type_should_succeed},
          {"let with variable",
           k_let_with_variable_input,
           k_let_with_variable_expected,
           k_let_with_variable_should_succeed},
          {"let with expression",
           k_let_with_expression_input,
           k_let_with_expression_expected,
           k_let_with_expression_should_succeed},
          {"let with function call",
           k_let_with_function_call_input,
           k_let_with_function_call_expected,
           k_let_with_function_call_should_succeed},
          {"let with tuple pattern",
           k_let_with_tuple_pattern_input,
           k_let_with_tuple_pattern_expected,
           k_let_with_tuple_pattern_should_succeed},
          {"let with qualified type",
           k_let_with_qualified_type_input,
           k_let_with_qualified_type_expected,
           k_let_with_qualified_type_should_succeed},
          {"let with string", k_let_with_string_input, k_let_with_string_expected, k_let_with_string_should_succeed},
          {"invalid: missing init", k_let_missing_init_input, "", k_let_missing_init_should_succeed},
          {"invalid: missing pattern", k_let_missing_pattern_input, "", k_let_missing_pattern_should_succeed},
          {"invalid: missing semicolon", k_let_missing_semicolon_input, "", k_let_missing_semicolon_should_succeed},
          {"invalid: literal pattern (semantic)",
           k_let_invalid_pattern_input,
           "",
           k_let_invalid_pattern_should_succeed},
          {"invalid: missing equals", k_let_missing_equals_input, "", k_let_missing_equals_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
