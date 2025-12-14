#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Statement;

// PARSE_TEST generates check_parse() function and Statement_Params type
PARSE_TEST(Statement, statement)

namespace {
// === Valid Break Statements ===

// Simple break without value
constexpr auto k_simple_break_should_succeed = true;
constexpr auto k_simple_break_input = "break;";
inline auto const k_simple_break_expected = test_sexp::break_statement();

// Break with integer value
constexpr auto k_break_with_integer_should_succeed = true;
constexpr auto k_break_with_integer_input = "break 42;";
inline auto const k_break_with_integer_expected = test_sexp::break_statement(test_sexp::integer(42));

// Break with variable
constexpr auto k_break_with_var_should_succeed = true;
constexpr auto k_break_with_var_input = "break result;";
inline auto const k_break_with_var_expected = test_sexp::break_statement(test_sexp::var_name("result"));

// Break with expression
constexpr auto k_break_with_expression_should_succeed = true;
constexpr auto k_break_with_expression_input = "break x + 1;";
inline auto const k_break_with_expression_expected =
    test_sexp::break_statement(test_sexp::binary_expr("+", test_sexp::var_name("x"), test_sexp::integer(1)));

// Break with function call
constexpr auto k_break_with_func_call_should_succeed = true;
constexpr auto k_break_with_func_call_input = "break calculate();";
inline auto const k_break_with_func_call_expected =
    test_sexp::break_statement(test_sexp::function_call(test_sexp::var_name("calculate"), {}));

// Break with string
constexpr auto k_break_with_string_should_succeed = true;
constexpr auto k_break_with_string_input = R"(break "done";)";
inline auto const k_break_with_string_expected = test_sexp::break_statement(test_sexp::string(R"("done")"));

// Break with spaces
constexpr auto k_break_with_spaces_should_succeed = true;
constexpr auto k_break_with_spaces_input = "break   value   ;";
inline auto const k_break_with_spaces_expected = test_sexp::break_statement(test_sexp::var_name("value"));

// === Invalid Break Statements ===

// Missing semicolon
constexpr auto k_missing_semicolon_should_succeed = false;
constexpr auto k_missing_semicolon_input = "break";

// Note: "break;;" actually succeeds - parses "break;" and leaves second ";" unparsed
// This is expected behavior for statement parsing

// Break is a keyword, can't use as variable name
constexpr auto k_break_as_var_should_succeed = false;
constexpr auto k_break_as_var_input = "break = 5;";

}  // namespace

TEST_CASE("Parse Break_Statement") {
  std::vector<Statement_Params> const params_list = {
      // Valid cases
      {.name = "simple break",
       .input = k_simple_break_input,
       .expected = k_simple_break_expected,
       .should_succeed = k_simple_break_should_succeed},
      {.name = "break with integer",
       .input = k_break_with_integer_input,
       .expected = k_break_with_integer_expected,
       .should_succeed = k_break_with_integer_should_succeed},
      {.name = "break with variable",
       .input = k_break_with_var_input,
       .expected = k_break_with_var_expected,
       .should_succeed = k_break_with_var_should_succeed},
      {.name = "break with expression",
       .input = k_break_with_expression_input,
       .expected = k_break_with_expression_expected,
       .should_succeed = k_break_with_expression_should_succeed},
      {.name = "break with function call",
       .input = k_break_with_func_call_input,
       .expected = k_break_with_func_call_expected,
       .should_succeed = k_break_with_func_call_should_succeed},
      {.name = "break with string",
       .input = k_break_with_string_input,
       .expected = k_break_with_string_expected,
       .should_succeed = k_break_with_string_should_succeed},
      {.name = "break with spaces",
       .input = k_break_with_spaces_input,
       .expected = k_break_with_spaces_expected,
       .should_succeed = k_break_with_spaces_should_succeed},

      // Invalid cases
      {.name = "invalid - missing semicolon",
       .input = k_missing_semicolon_input,
       .expected = "",
       .should_succeed = k_missing_semicolon_should_succeed},
      {.name = "invalid - break as variable",
       .input = k_break_as_var_input,
       .expected = "",
       .should_succeed = k_break_as_var_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
