#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Statement;

PARSE_TEST(Statement, statement)

namespace {
// === Valid Let Statements ===

// Simple let binding
constexpr auto k_simple_let_should_succeed = true;
constexpr auto k_simple_let_input = "let x = 42;";
inline auto const k_simple_let_expected =
    test_sexp::let_statement(test_sexp::simple_pattern("x"), test_sexp::integer(42));

// Let with type annotation
constexpr auto k_let_with_type_should_succeed = true;
constexpr auto k_let_with_type_input = "let x: I32 = 42;";
inline auto const k_let_with_type_expected = test_sexp::let_statement(
    test_sexp::simple_pattern("x"),
    test_sexp::integer(42),
    false,
    test_sexp::type_name("I32")
);

// Let mut binding
constexpr auto k_let_mut_should_succeed = true;
constexpr auto k_let_mut_input = "let mut count = 0;";
inline auto const k_let_mut_expected =
    test_sexp::let_statement(test_sexp::simple_pattern("count"), test_sexp::integer(0), true);

// Let mut with type annotation
constexpr auto k_let_mut_with_type_should_succeed = true;
constexpr auto k_let_mut_with_type_input = "let mut counter: I32 = 100;";
inline auto const k_let_mut_with_type_expected = test_sexp::let_statement(
    test_sexp::simple_pattern("counter"),
    test_sexp::integer(100),
    true,
    test_sexp::type_name("I32")
);

// Let with variable initializer
constexpr auto k_let_with_var_should_succeed = true;
constexpr auto k_let_with_var_input = "let y = x;";
inline auto const k_let_with_var_expected =
    test_sexp::let_statement(test_sexp::simple_pattern("y"), test_sexp::var_name("x"));

// Let with expression initializer
constexpr auto k_let_with_expression_should_succeed = true;
constexpr auto k_let_with_expression_input = "let result = x + 10;";
inline auto const k_let_with_expression_expected = test_sexp::let_statement(
    test_sexp::simple_pattern("result"),
    test_sexp::binary_expr("+", test_sexp::var_name("x"), test_sexp::integer(10))
);

// Let with function call initializer
constexpr auto k_let_with_func_call_should_succeed = true;
constexpr auto k_let_with_func_call_input = "let value = calculate();";
inline auto const k_let_with_func_call_expected = test_sexp::let_statement(
    test_sexp::simple_pattern("value"),
    test_sexp::function_call(test_sexp::var_name("calculate"), {})
);

// Let with tuple pattern
constexpr auto k_let_with_tuple_pattern_should_succeed = true;
constexpr auto k_let_with_tuple_pattern_input = "let (x, y) = point;";
inline auto const k_let_with_tuple_pattern_expected = test_sexp::let_statement(
    test_sexp::tuple_pattern({test_sexp::simple_pattern("x"), test_sexp::simple_pattern("y")}),
    test_sexp::var_name("point")
);

// Let with qualified type
constexpr auto k_let_with_qualified_type_should_succeed = true;
constexpr auto k_let_with_qualified_type_input = "let name: Std.String = create_name();";
inline auto const k_let_with_qualified_type_expected = test_sexp::let_statement(
    test_sexp::simple_pattern("name"),
    test_sexp::function_call(test_sexp::var_name("create_name"), {}),
    false,
    test_sexp::type_name_path({"Std", "String"})
);

// Let with string initializer
constexpr auto k_let_with_string_should_succeed = true;
constexpr auto k_let_with_string_input = R"(let msg = "hello";)";
inline auto const k_let_with_string_expected =
    test_sexp::let_statement(test_sexp::simple_pattern("msg"), test_sexp::string(R"("hello")"));

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
inline auto const k_let_invalid_pattern_expected =
    test_sexp::let_statement(test_sexp::literal_pattern(test_sexp::integer(123)), test_sexp::var_name("x"));

// Missing equals sign
constexpr auto k_let_missing_equals_should_succeed = false;
constexpr auto k_let_missing_equals_input = "let x 42;";

}  // namespace

TEST_CASE("Parse Let_Statement") {
  std::vector<Statement_Params> const params_list = {
      {.name = "simple let",
       .input = k_simple_let_input,
       .expected = k_simple_let_expected,
       .should_succeed = k_simple_let_should_succeed},
      {.name = "let with type",
       .input = k_let_with_type_input,
       .expected = k_let_with_type_expected,
       .should_succeed = k_let_with_type_should_succeed},
      {.name = "let mut",
       .input = k_let_mut_input,
       .expected = k_let_mut_expected,
       .should_succeed = k_let_mut_should_succeed},
      {.name = "let mut with type",
       .input = k_let_mut_with_type_input,
       .expected = k_let_mut_with_type_expected,
       .should_succeed = k_let_mut_with_type_should_succeed},
      {.name = "let with variable",
       .input = k_let_with_var_input,
       .expected = k_let_with_var_expected,
       .should_succeed = k_let_with_var_should_succeed},
      {.name = "let with expression",
       .input = k_let_with_expression_input,
       .expected = k_let_with_expression_expected,
       .should_succeed = k_let_with_expression_should_succeed},
      {.name = "let with function call",
       .input = k_let_with_func_call_input,
       .expected = k_let_with_func_call_expected,
       .should_succeed = k_let_with_func_call_should_succeed},
      {.name = "let with tuple pattern",
       .input = k_let_with_tuple_pattern_input,
       .expected = k_let_with_tuple_pattern_expected,
       .should_succeed = k_let_with_tuple_pattern_should_succeed},
      {.name = "let with qualified type",
       .input = k_let_with_qualified_type_input,
       .expected = k_let_with_qualified_type_expected,
       .should_succeed = k_let_with_qualified_type_should_succeed},
      {.name = "let with string",
       .input = k_let_with_string_input,
       .expected = k_let_with_string_expected,
       .should_succeed = k_let_with_string_should_succeed},
      {.name = "invalid: missing init",
       .input = k_let_missing_init_input,
       .expected = "",
       .should_succeed = k_let_missing_init_should_succeed},
      {.name = "invalid: missing pattern",
       .input = k_let_missing_pattern_input,
       .expected = "",
       .should_succeed = k_let_missing_pattern_should_succeed},
      {.name = "invalid: missing semicolon",
       .input = k_let_missing_semicolon_input,
       .expected = "",
       .should_succeed = k_let_missing_semicolon_should_succeed},
      {.name = "invalid: literal pattern (semantic)",
       .input = k_let_invalid_pattern_input,
       .expected = k_let_invalid_pattern_expected,
       .should_succeed = k_let_invalid_pattern_should_succeed},
      {.name = "invalid: missing equals",
       .input = k_let_missing_equals_input,
       .expected = "",
       .should_succeed = k_let_missing_equals_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
