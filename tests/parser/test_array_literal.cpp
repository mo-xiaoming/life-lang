#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// Empty array
constexpr auto k_empty_array_should_succeed = true;
constexpr auto k_empty_array_input = "[]";
inline auto const k_empty_array_expected = "(array_lit ())";

// Single element
constexpr auto k_single_element_should_succeed = true;
constexpr auto k_single_element_input = "[42]";
inline auto const k_single_element_expected = R"((array_lit ((integer "42"))))";

// Multiple integers
constexpr auto k_multiple_integers_should_succeed = true;
constexpr auto k_multiple_integers_input = "[1, 2, 3, 4, 5]";
inline auto const k_multiple_integers_expected =
    R"((array_lit ((integer "1") (integer "2") (integer "3") (integer "4") (integer "5"))))";

// Mixed types (parser accepts, semantic analysis checks)
constexpr auto k_mixed_types_should_succeed = true;
constexpr auto k_mixed_types_input = R"([1, "hello", true])";
inline std::string const k_mixed_types_expected =
    std::format(R"((array_lit ((integer "1") (string "\"hello\"") {})))", test_sexp::bool_literal(true));

// Nested arrays
constexpr auto k_nested_arrays_should_succeed = true;
constexpr auto k_nested_arrays_input = "[[1, 2], [3, 4]]";
inline auto const k_nested_arrays_expected =
    R"((array_lit ((array_lit ((integer "1") (integer "2"))) (array_lit ((integer "3") (integer "4"))))))";

// Array with expressions
constexpr auto k_array_with_expressions_should_succeed = true;
constexpr auto k_array_with_expressions_input = "[1 + 2, x * 3, foo()]";
inline std::string const k_array_with_expressions_expected = std::format(
    R"((array_lit ((binary + (integer "1") (integer "2")) (binary * {} (integer "3")) (call {} ()))))",
    test_sexp::var_name("x"),
    test_sexp::var_name("foo")
);

// Array with variables
constexpr auto k_array_with_variables_should_succeed = true;
constexpr auto k_array_with_variables_input = "[x, y, z]";
inline std::string const k_array_with_variables_expected = std::format(
    R"((array_lit ({} {} {})))",
    test_sexp::var_name("x"),
    test_sexp::var_name("y"),
    test_sexp::var_name("z")
);

// Trailing comma
constexpr auto k_trailing_comma_should_succeed = true;
constexpr auto k_trailing_comma_input = "[1, 2, 3,]";
inline auto const k_trailing_comma_expected = R"((array_lit ((integer "1") (integer "2") (integer "3"))))";

// Multiline array
constexpr auto k_multiline_should_succeed = true;
constexpr auto k_multiline_input = R"([
  1,
  2,
  3
])";
inline auto const k_multiline_expected = R"((array_lit ((integer "1") (integer "2") (integer "3"))))";

// No spaces
constexpr auto k_no_spaces_should_succeed = true;
constexpr auto k_no_spaces_input = "[1,2,3]";
inline auto const k_no_spaces_expected = R"((array_lit ((integer "1") (integer "2") (integer "3"))))";

// String array
constexpr auto k_string_array_should_succeed = true;
constexpr auto k_string_array_input = R"(["hello", "world"])";
inline auto const k_string_array_expected = R"((array_lit ((string "\"hello\"") (string "\"world\""))))";

// Array with struct literals
constexpr auto k_array_with_structs_should_succeed = true;
constexpr auto k_array_with_structs_input = "[Point { x: 1, y: 2 }, Point { x: 3, y: 4 }]";
inline auto const k_array_with_structs_expected =
    R"((array_lit ((struct_lit "Point" ((field_init "x" (integer "1")) (field_init "y" (integer "2")))) (struct_lit "Point" ((field_init "x" (integer "3")) (field_init "y" (integer "4")))))))";

// Array in function call
constexpr auto k_array_in_func_call_should_succeed = true;
constexpr auto k_array_in_func_call_input = "process([1, 2, 3])";
inline std::string const k_array_in_func_call_expected = std::format(
    R"((call {} ((array_lit ((integer "1") (integer "2") (integer "3"))))))",
    test_sexp::var_name("process")
);

// Array in let statement
constexpr auto k_array_in_let_should_succeed = true;
constexpr auto k_array_in_let_input = "let arr = [1, 2, 3];";
inline std::string const k_array_in_let_expected = test_sexp::let_statement(
    test_sexp::simple_pattern("arr"),
    R"((array_lit ((integer "1") (integer "2") (integer "3"))))"
);

}  // namespace

TEST_CASE("Parse Expr (Array Literals)") {
  std::vector<Expr_Params> const params_list = {
      {.name = "empty array",
       .input = k_empty_array_input,
       .expected = k_empty_array_expected,
       .should_succeed = k_empty_array_should_succeed},
      {.name = "single element",
       .input = k_single_element_input,
       .expected = k_single_element_expected,
       .should_succeed = k_single_element_should_succeed},
      {.name = "multiple integers",
       .input = k_multiple_integers_input,
       .expected = k_multiple_integers_expected,
       .should_succeed = k_multiple_integers_should_succeed},
      {.name = "mixed types",
       .input = k_mixed_types_input,
       .expected = k_mixed_types_expected,
       .should_succeed = k_mixed_types_should_succeed},
      {.name = "nested arrays",
       .input = k_nested_arrays_input,
       .expected = k_nested_arrays_expected,
       .should_succeed = k_nested_arrays_should_succeed},
      {.name = "array with expressions",
       .input = k_array_with_expressions_input,
       .expected = k_array_with_expressions_expected,
       .should_succeed = k_array_with_expressions_should_succeed},
      {.name = "array with variables",
       .input = k_array_with_variables_input,
       .expected = k_array_with_variables_expected,
       .should_succeed = k_array_with_variables_should_succeed},
      {.name = "trailing comma",
       .input = k_trailing_comma_input,
       .expected = k_trailing_comma_expected,
       .should_succeed = k_trailing_comma_should_succeed},
      {.name = "multiline",
       .input = k_multiline_input,
       .expected = k_multiline_expected,
       .should_succeed = k_multiline_should_succeed},
      {.name = "no spaces",
       .input = k_no_spaces_input,
       .expected = k_no_spaces_expected,
       .should_succeed = k_no_spaces_should_succeed},
      {.name = "string array",
       .input = k_string_array_input,
       .expected = k_string_array_expected,
       .should_succeed = k_string_array_should_succeed},
      {.name = "array with structs",
       .input = k_array_with_structs_input,
       .expected = k_array_with_structs_expected,
       .should_succeed = k_array_with_structs_should_succeed},
      {.name = "array in func call",
       .input = k_array_in_func_call_input,
       .expected = k_array_in_func_call_expected,
       .should_succeed = k_array_in_func_call_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      INFO(params.name);
      check_parse(params);
    }
  }
}

TEST_CASE("Parse Let_Statement with Array") {
  using life_lang::ast::Let_Statement;
  using Let_Params = Parse_Test_Params<Let_Statement>;

  auto check_let = [](Let_Params const& params_) {
    INFO("Input: ", std::string(params_.input));
    life_lang::parser::Parser parser(params_.input);
    auto const result = parser.parse_let_statement();
    CHECK(params_.should_succeed == bool(result));
    if (result && params_.expected.has_value()) {
      auto const actual_sexp = life_lang::ast::to_sexp_string(*result, 0);
      auto const expected_sexp = get_expected_sexp(*params_.expected);
      CHECK(normalize_sexp(actual_sexp) == normalize_sexp(expected_sexp));
    }
  };

  Let_Params const params{
      .name = "array in let",
      .input = k_array_in_let_input,
      .expected = k_array_in_let_expected,
      .should_succeed = k_array_in_let_should_succeed
  };
  check_let(params);
}
