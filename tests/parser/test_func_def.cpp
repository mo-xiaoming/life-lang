#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Func_Def;
using test_sexp::type_name;
using test_sexp::var_name;

PARSE_TEST(Func_Def, func_def)

namespace {
// Simple function definitions
constexpr auto k_empty_body_should_succeed = true;
constexpr auto k_empty_body_input = "fn hello(): Int {}";
inline auto const k_empty_body_expected =
    test_sexp::func_def(test_sexp::func_decl("hello", {}, {}, type_name("Int")), test_sexp::block({}));

// Functions with parameters
constexpr auto k_with_parameters_should_succeed = true;
constexpr auto k_with_parameters_input = "fn hello(a: Int, b: Double): Int {}";
inline auto const k_with_parameters_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "hello",
        {},
        {test_sexp::function_parameter("a", type_name("Int")), test_sexp::function_parameter("b", type_name("Double"))},
        type_name("Int")
    ),
    test_sexp::block({})
);

// Functions with statements
constexpr auto k_with_return_should_succeed = true;
constexpr auto k_with_return_input = "fn hello(): Int {return world;}";
inline auto const k_with_return_expected = test_sexp::func_def(
    test_sexp::func_decl("hello", {}, {}, type_name("Int")),
    test_sexp::block({test_sexp::return_statement(var_name("world"))})
);

constexpr auto k_with_statements_should_succeed = true;
constexpr auto k_with_statements_input = "fn hello(): Int {foo(); return 0;}";
inline auto const k_with_statements_expected = test_sexp::func_def(
    test_sexp::func_decl("hello", {}, {}, type_name("Int")),
    test_sexp::block(
        {test_sexp::function_call_statement(test_sexp::function_call(var_name("foo"), {})),
         test_sexp::return_statement(test_sexp::integer("0"))}
    )
);

// Nested constructs
constexpr auto k_nested_block_should_succeed = true;
constexpr auto k_nested_block_input = R"(fn hello(a: Int): Int {
    hello();
    {
        return world;
    }
})";
inline auto const k_nested_block_expected = test_sexp::func_def(
    test_sexp::func_decl("hello", {}, {test_sexp::function_parameter("a", type_name("Int"))}, type_name("Int")),
    test_sexp::block(
        {test_sexp::function_call_statement(test_sexp::function_call(var_name("hello"), {})),
         test_sexp::block({test_sexp::return_statement(var_name("world"))})}
    )
);

constexpr auto k_nested_func_should_succeed = true;
constexpr auto k_nested_func_input = R"(fn hello(): Int {
    fn world(): Int {
        return 0;
    }
    return world();
})";
inline auto const k_nested_func_expected = test_sexp::func_def(
    test_sexp::func_decl("hello", {}, {}, type_name("Int")),
    test_sexp::block(
        {test_sexp::func_def(
             test_sexp::func_decl("world", {}, {}, type_name("Int")),
             test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
         ),
         test_sexp::return_statement(test_sexp::function_call(var_name("world"), {}))}
    )
);

// Complex real-world examples
constexpr auto k_hello_world_should_succeed = true;
constexpr auto k_hello_world_input = R"(fn main(args: Std.Array<Std.String>): I32 {
    Std.print("Hello, world!");
    return 0;
})";
inline auto const k_hello_world_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "main",
        {},
        {"(param false \"args\" (path ((type_segment \"Std\") (type_segment \"Array\" ((path ((type_segment \"Std\") "
         "(type_segment \"String\"))))))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block(
        {test_sexp::function_call_statement(
             R"((call (var ((var_segment "Std") (var_segment "print"))) ((string "\"Hello, world!\""))))"
         ),
         test_sexp::return_statement(test_sexp::integer("0"))}
    )
);
;

// Trailing content
constexpr auto k_with_trailing_code_should_succeed = false;
constexpr auto k_with_trailing_code_input = "fn foo(): Int {} bar";
inline auto const k_with_trailing_code_expected =
    test_sexp::func_def(test_sexp::func_decl("foo", {}, {}, type_name("Int")), test_sexp::block({}));

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_should_succeed = false;
constexpr auto k_invalid_no_fn_keyword_input = "hello(): Int {}";
constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_expected =
    test_sexp::func_def(test_sexp::func_decl("", {}, {}, "(path ())"), test_sexp::block({}));

}  // namespace

TEST_CASE("Parse Func_Def") {
  std::vector<Func_Def_Params> const params_list = {
      // Simple function definitions
      {.name = "empty body",
       .input = k_empty_body_input,
       .expected = k_empty_body_expected,
       .should_succeed = k_empty_body_should_succeed},

      // Functions with parameters
      {.name = "with parameters",
       .input = k_with_parameters_input,
       .expected = k_with_parameters_expected,
       .should_succeed = k_with_parameters_should_succeed},

      // Functions with statements
      {.name = "with return",
       .input = k_with_return_input,
       .expected = k_with_return_expected,
       .should_succeed = k_with_return_should_succeed},
      {.name = "with statements",
       .input = k_with_statements_input,
       .expected = k_with_statements_expected,
       .should_succeed = k_with_statements_should_succeed},

      // Nested constructs
      {.name = "nested block",
       .input = k_nested_block_input,
       .expected = k_nested_block_expected,
       .should_succeed = k_nested_block_should_succeed},
      {.name = "nested function",
       .input = k_nested_func_input,
       .expected = k_nested_func_expected,
       .should_succeed = k_nested_func_should_succeed},

      // Complex real-world examples
      {.name = "hello world",
       .input = k_hello_world_input,
       .expected = k_hello_world_expected,
       .should_succeed = k_hello_world_should_succeed},

      // Trailing content
      {.name = "with trailing code",
       .input = k_with_trailing_code_input,
       .expected = k_with_trailing_code_expected,
       .should_succeed = k_with_trailing_code_should_succeed},

      // Invalid cases
      {.name = "invalid - no fn keyword",
       .input = k_invalid_no_fn_keyword_input,
       .expected = k_invalid_expected,
       .should_succeed = k_invalid_no_fn_keyword_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_expected,
       .should_succeed = k_invalid_empty_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
