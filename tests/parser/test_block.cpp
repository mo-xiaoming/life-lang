#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Block;

PARSE_TEST(Block, block)

namespace {
// Empty block
constexpr auto k_empty_block_should_succeed = true;
constexpr auto k_empty_block_input = "{}";
inline auto const k_empty_block_expected = test_sexp::block({});

// Single statement blocks
constexpr auto k_single_return_should_succeed = true;
constexpr auto k_single_return_input = "{return hello;}";
inline auto const k_single_return_expected =
    test_sexp::block({test_sexp::return_statement(test_sexp::var_name("hello"))});

constexpr auto k_single_func_call_should_succeed = true;
constexpr auto k_single_func_call_input = "{foo();}";
inline auto const k_single_func_call_expected =
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("foo"), {}))});

// Multiple statements
constexpr auto k_two_statements_should_succeed = true;
constexpr auto k_two_statements_input = "{hello.a(); return world;}";
inline auto const k_two_statements_expected = test_sexp::block(
    {test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name_path({"hello", "a"}), {})),
     test_sexp::return_statement(test_sexp::var_name("world"))}
);

constexpr auto k_multiple_statements_should_succeed = true;
constexpr auto k_multiple_statements_input = "{foo(); bar(); return 0;}";
inline auto const k_multiple_statements_expected = test_sexp::block(
    {test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("foo"), {})),
     test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("bar"), {})),
     test_sexp::return_statement(test_sexp::integer(0))}
);

// Nested blocks
constexpr auto k_nested_block_should_succeed = true;
constexpr auto k_nested_block_input = "{hello(b); {return world;}}";
inline auto const k_nested_block_expected = test_sexp::block(
    {test_sexp::function_call_statement(
         test_sexp::function_call(test_sexp::var_name("hello"), {test_sexp::var_name("b")})
     ),
     test_sexp::block({test_sexp::return_statement(test_sexp::var_name("world"))})}
);

// Whitespace handling
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "{  foo(  )  ;  }";
inline auto const k_with_spaces_expected =
    test_sexp::block({test_sexp::function_call_statement(test_sexp::function_call(test_sexp::var_name("foo"), {}))});

// Trailing content
constexpr auto k_with_trailing_code_should_succeed = false;
constexpr auto k_with_trailing_code_input = "{return x;} y";
inline auto const k_with_trailing_code_expected =
    test_sexp::block({test_sexp::return_statement(test_sexp::var_name("x"))});

// Invalid cases
constexpr auto k_invalid_no_closing_brace_should_succeed = false;
constexpr auto k_invalid_no_closing_brace_input = "{return x;";
constexpr auto k_invalid_no_opening_brace_should_succeed = false;
constexpr auto k_invalid_no_opening_brace_input = "return x;}";
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_expected = test_sexp::block({});
}  // namespace

TEST_CASE("Parse Block") {
  std::vector<Block_Params> const params_list = {
      // Empty block
      {.name = "empty block",
       .input = k_empty_block_input,
       .expected = k_empty_block_expected,
       .should_succeed = k_empty_block_should_succeed},

      // Single statement blocks
      {.name = "single return",
       .input = k_single_return_input,
       .expected = k_single_return_expected,
       .should_succeed = k_single_return_should_succeed},
      {.name = "single function call",
       .input = k_single_func_call_input,
       .expected = k_single_func_call_expected,
       .should_succeed = k_single_func_call_should_succeed},

      // Multiple statements
      {.name = "two statements",
       .input = k_two_statements_input,
       .expected = k_two_statements_expected,
       .should_succeed = k_two_statements_should_succeed},
      {.name = "multiple statements",
       .input = k_multiple_statements_input,
       .expected = k_multiple_statements_expected,
       .should_succeed = k_multiple_statements_should_succeed},

      // Nested blocks
      {.name = "nested block",
       .input = k_nested_block_input,
       .expected = k_nested_block_expected,
       .should_succeed = k_nested_block_should_succeed},

      // Whitespace handling
      {.name = "with spaces",
       .input = k_with_spaces_input,
       .expected = k_with_spaces_expected,
       .should_succeed = k_with_spaces_should_succeed},

      // Trailing content
      {.name = "with trailing code",
       .input = k_with_trailing_code_input,
       .expected = k_with_trailing_code_expected,
       .should_succeed = k_with_trailing_code_should_succeed},

      // Invalid cases
      {.name = "invalid - no closing brace",
       .input = k_invalid_no_closing_brace_input,
       .expected = k_invalid_expected,
       .should_succeed = k_invalid_no_closing_brace_should_succeed},
      {.name = "invalid - no opening brace",
       .input = k_invalid_no_opening_brace_input,
       .expected = k_invalid_expected,
       .should_succeed = k_invalid_no_opening_brace_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_expected,
       .should_succeed = k_invalid_empty_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}