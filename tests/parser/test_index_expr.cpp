#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Simple index
constexpr auto k_simple_index_should_succeed = true;
constexpr auto k_simple_index_input = "arr[0]";
inline std::string const k_simple_index_expected =
    std::format(R"((index {} (integer "0")))", test_sexp::var_name("arr"));

// Variable index
constexpr auto k_variable_index_should_succeed = true;
constexpr auto k_variable_index_input = "arr[i]";
inline std::string const k_variable_index_expected =
    std::format(R"((index {} {}))", test_sexp::var_name("arr"), test_sexp::var_name("i"));

// Expression index
constexpr auto k_expression_index_should_succeed = true;
constexpr auto k_expression_index_input = "arr[i + 1]";
inline std::string const k_expression_index_expected =
    std::format(R"((index {} (binary + {} (integer "1"))))", test_sexp::var_name("arr"), test_sexp::var_name("i"));

// Chained indexing (2D array)
constexpr auto k_chained_index_should_succeed = true;
constexpr auto k_chained_index_input = "matrix[i][j]";
inline std::string const k_chained_index_expected = std::format(
    R"((index (index {} {}) {}))",
    test_sexp::var_name("matrix"),
    test_sexp::var_name("i"),
    test_sexp::var_name("j")
);

// Triple nested indexing (3D array)
constexpr auto k_triple_index_should_succeed = true;
constexpr auto k_triple_index_input = "cube[x][y][z]";
inline std::string const k_triple_index_expected = std::format(
    R"((index (index (index {} {}) {}) {}))",
    test_sexp::var_name("cube"),
    test_sexp::var_name("x"),
    test_sexp::var_name("y"),
    test_sexp::var_name("z")
);

// Index on function call result
constexpr auto k_func_call_index_should_succeed = true;
constexpr auto k_func_call_index_input = "get_array()[0]";
inline std::string const k_func_call_index_expected =
    std::format(R"((index (call {} ()) (integer "0")))", test_sexp::var_name("get_array"));

// Index on field access
constexpr auto k_field_access_index_should_succeed = true;
constexpr auto k_field_access_index_input = "obj.items[0]";
inline std::string const k_field_access_index_expected =
    std::format(R"((index (field_access {} "items") (integer "0")))", test_sexp::var_name("obj"));

// Index on array literal
constexpr auto k_array_literal_index_should_succeed = true;
constexpr auto k_array_literal_index_input = "[1, 2, 3][0]";
inline auto const k_array_literal_index_expected =
    R"((index (array_lit ((integer "1") (integer "2") (integer "3"))) (integer "0")))";

// Index with method call
constexpr auto k_index_with_method_should_succeed = true;
constexpr auto k_index_with_method_input = "arr[0].process()";
inline std::string const k_index_with_method_expected =
    std::format(R"((call {} ((index {} (integer "0")))))", test_sexp::var_name("process"), test_sexp::var_name("arr"));

// Index in assignment target
constexpr auto k_index_in_assignment_should_succeed = true;
constexpr auto k_index_in_assignment_input = "arr[i] = 42";
inline std::string const k_index_in_assignment_expected =
    std::format(R"((assign (index {} {}) (integer "42")))", test_sexp::var_name("arr"), test_sexp::var_name("i"));

// Chained index in assignment
constexpr auto k_chained_index_assignment_should_succeed = true;
constexpr auto k_chained_index_assignment_input = "matrix[i][j] = value";
inline std::string const k_chained_index_assignment_expected = std::format(
    R"((assign (index (index {} {}) {}) {}))",
    test_sexp::var_name("matrix"),
    test_sexp::var_name("i"),
    test_sexp::var_name("j"),
    test_sexp::var_name("value")
);

// Index in binary expression
constexpr auto k_index_in_binary_should_succeed = true;
constexpr auto k_index_in_binary_input = "arr[0] + arr[1]";
inline std::string const k_index_in_binary_expected = std::format(
    R"((binary + (index {} (integer "0")) (index {} (integer "1"))))",
    test_sexp::var_name("arr"),
    test_sexp::var_name("arr")
);

// Index in function call argument
constexpr auto k_index_in_func_arg_should_succeed = true;
constexpr auto k_index_in_func_arg_input = "process(arr[0])";
inline std::string const k_index_in_func_arg_expected =
    std::format(R"((call {} ((index {} (integer "0")))))", test_sexp::var_name("process"), test_sexp::var_name("arr"));

// No spaces
constexpr auto k_no_spaces_should_succeed = true;
constexpr auto k_no_spaces_input = "arr[0]";
inline std::string const k_no_spaces_expected = std::format(R"((index {} (integer "0")))", test_sexp::var_name("arr"));

// With spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "arr[ 0 ]";
inline std::string const k_with_spaces_expected =
    std::format(R"((index {} (integer "0")))", test_sexp::var_name("arr"));

// Complex index expression
constexpr auto k_complex_index_should_succeed = true;
constexpr auto k_complex_index_input = "arr[i * 2 + offset]";
inline std::string const k_complex_index_expected = std::format(
    R"((index {} (binary + (binary * {} (integer "2")) {})))",
    test_sexp::var_name("arr"),
    test_sexp::var_name("i"),
    test_sexp::var_name("offset")
);

// Index followed by field access
constexpr auto k_index_then_field_should_succeed = true;
constexpr auto k_index_then_field_input = "arr[0].value";
inline std::string const k_index_then_field_expected =
    std::format(R"((field_access (index {} (integer "0")) "value"))", test_sexp::var_name("arr"));

// Field access then index
constexpr auto k_field_then_index_should_succeed = true;
constexpr auto k_field_then_index_input = "obj.arr[0]";
inline std::string const k_field_then_index_expected =
    std::format(R"((index (field_access {} "arr") (integer "0")))", test_sexp::var_name("obj"));

// Mix of operations: field, index, call
constexpr auto k_mixed_operations_should_succeed = true;
constexpr auto k_mixed_operations_input = "obj.get_items()[0].name";
inline std::string const k_mixed_operations_expected = std::format(
    R"((field_access (index (call {} ()) (integer "0")) "name"))",
    test_sexp::var_name_path({"obj", "get_items"})
);

}  // namespace

TEST_CASE("Parse Expr (Index Expressions)") {
  std::vector<Expr_Params> const params_list = {
      {.name = "simple index",
       .input = k_simple_index_input,
       .expected = k_simple_index_expected,
       .should_succeed = k_simple_index_should_succeed},
      {.name = "variable index",
       .input = k_variable_index_input,
       .expected = k_variable_index_expected,
       .should_succeed = k_variable_index_should_succeed},
      {.name = "expression index",
       .input = k_expression_index_input,
       .expected = k_expression_index_expected,
       .should_succeed = k_expression_index_should_succeed},
      {.name = "chained indexing",
       .input = k_chained_index_input,
       .expected = k_chained_index_expected,
       .should_succeed = k_chained_index_should_succeed},
      {.name = "triple nested indexing",
       .input = k_triple_index_input,
       .expected = k_triple_index_expected,
       .should_succeed = k_triple_index_should_succeed},
      {.name = "index on function call",
       .input = k_func_call_index_input,
       .expected = k_func_call_index_expected,
       .should_succeed = k_func_call_index_should_succeed},
      {.name = "index on field access",
       .input = k_field_access_index_input,
       .expected = k_field_access_index_expected,
       .should_succeed = k_field_access_index_should_succeed},
      {.name = "index on array literal",
       .input = k_array_literal_index_input,
       .expected = k_array_literal_index_expected,
       .should_succeed = k_array_literal_index_should_succeed},
      {.name = "index with method call",
       .input = k_index_with_method_input,
       .expected = k_index_with_method_expected,
       .should_succeed = k_index_with_method_should_succeed},
      {.name = "index in assignment",
       .input = k_index_in_assignment_input,
       .expected = k_index_in_assignment_expected,
       .should_succeed = k_index_in_assignment_should_succeed},
      {.name = "chained index assignment",
       .input = k_chained_index_assignment_input,
       .expected = k_chained_index_assignment_expected,
       .should_succeed = k_chained_index_assignment_should_succeed},
      {.name = "index in binary expr",
       .input = k_index_in_binary_input,
       .expected = k_index_in_binary_expected,
       .should_succeed = k_index_in_binary_should_succeed},
      {.name = "index in func arg",
       .input = k_index_in_func_arg_input,
       .expected = k_index_in_func_arg_expected,
       .should_succeed = k_index_in_func_arg_should_succeed},
      {.name = "no spaces",
       .input = k_no_spaces_input,
       .expected = k_no_spaces_expected,
       .should_succeed = k_no_spaces_should_succeed},
      {.name = "with spaces",
       .input = k_with_spaces_input,
       .expected = k_with_spaces_expected,
       .should_succeed = k_with_spaces_should_succeed},
      {.name = "complex index",
       .input = k_complex_index_input,
       .expected = k_complex_index_expected,
       .should_succeed = k_complex_index_should_succeed},
      {.name = "index then field",
       .input = k_index_then_field_input,
       .expected = k_index_then_field_expected,
       .should_succeed = k_index_then_field_should_succeed},
      {.name = "field then index",
       .input = k_field_then_index_input,
       .expected = k_field_then_index_expected,
       .should_succeed = k_field_then_index_should_succeed},
      {.name = "mixed operations",
       .input = k_mixed_operations_input,
       .expected = k_mixed_operations_expected,
       .should_succeed = k_mixed_operations_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      INFO(params.name);
      check_parse(params);
    }
  }
}
