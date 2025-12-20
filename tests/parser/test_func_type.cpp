#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Function_Type;
using namespace test_sexp;

PARSE_TEST(Function_Type, function_type)

namespace {

// Simple function type with no params
constexpr auto k_no_params_should_succeed = true;
constexpr auto k_no_params_input = "fn(): ()";
inline auto const k_no_params_expected = test_sexp::func_type({}, test_sexp::type_name("()"));

// Function type with single param
constexpr auto k_single_param_should_succeed = true;
constexpr auto k_single_param_input = "fn(I32): Bool";
inline auto const k_single_param_expected =
    test_sexp::func_type({test_sexp::type_name("I32")}, test_sexp::type_name("Bool"));

// Function type with multiple params
constexpr auto k_multiple_params_should_succeed = true;
constexpr auto k_multiple_params_input = "fn(I32, I32): I32";
inline auto const k_multiple_params_expected =
    test_sexp::func_type({test_sexp::type_name("I32"), test_sexp::type_name("I32")}, test_sexp::type_name("I32"));

// Function type with qualified types
constexpr auto k_qualified_types_should_succeed = true;
constexpr auto k_qualified_types_input = "fn(Std.String): Std.Result";
inline auto const k_qualified_types_expected =
    test_sexp::func_type({test_sexp::type_name_path({"Std", "String"})}, test_sexp::type_name_path({"Std", "Result"}));

// Higher-order function type (function takes function)
constexpr auto k_higher_order_should_succeed = true;
constexpr auto k_higher_order_input = "fn(fn(I32): Bool): Bool";
inline auto const k_higher_order_expected = test_sexp::func_type(
    {test_sexp::func_type({test_sexp::type_name("I32")}, test_sexp::type_name("Bool"))},
    test_sexp::type_name("Bool")
);

// Function type with generic types
constexpr auto k_generic_types_should_succeed = true;
constexpr auto k_generic_types_input = "fn(Array<I32>): Option<I32>";
inline auto const k_generic_types_expected = test_sexp::func_type(
    {R"((path ((type_segment "Array" ((path ((type_segment "I32"))))))))"},
    R"((path ((type_segment "Option" ((path ((type_segment "I32"))))))))"
);

// Function type with spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "fn( I32 , Bool ): String";
inline auto const k_with_spaces_expected =
    test_sexp::func_type({test_sexp::type_name("I32"), test_sexp::type_name("Bool")}, test_sexp::type_name("String"));

// Invalid: missing return type
constexpr auto k_missing_return_type_should_succeed = false;
constexpr auto k_missing_return_type_input = "fn(I32)";

// Invalid: missing colon
constexpr auto k_missing_colon_should_succeed = false;
constexpr auto k_missing_colon_input = "fn(I32) Bool";

// Invalid: missing parentheses
constexpr auto k_missing_parens_should_succeed = false;
constexpr auto k_missing_parens_input = "fn I32: Bool";

}  // namespace

TEST_CASE("Parse Function_Type") {
  std::vector<Function_Type_Params> const params_list = {
      {.name = "no params",
       .input = k_no_params_input,
       .expected = k_no_params_expected,
       .should_succeed = k_no_params_should_succeed},
      {.name = "single param",
       .input = k_single_param_input,
       .expected = k_single_param_expected,
       .should_succeed = k_single_param_should_succeed},
      {.name = "multiple params",
       .input = k_multiple_params_input,
       .expected = k_multiple_params_expected,
       .should_succeed = k_multiple_params_should_succeed},
      {.name = "qualified types",
       .input = k_qualified_types_input,
       .expected = k_qualified_types_expected,
       .should_succeed = k_qualified_types_should_succeed},
      {.name = "higher-order",
       .input = k_higher_order_input,
       .expected = k_higher_order_expected,
       .should_succeed = k_higher_order_should_succeed},
      {.name = "generic types",
       .input = k_generic_types_input,
       .expected = k_generic_types_expected,
       .should_succeed = k_generic_types_should_succeed},
      {.name = "with spaces",
       .input = k_with_spaces_input,
       .expected = k_with_spaces_expected,
       .should_succeed = k_with_spaces_should_succeed},
      {.name = "invalid - missing return type",
       .input = k_missing_return_type_input,
       .expected = "",
       .should_succeed = k_missing_return_type_should_succeed},
      {.name = "invalid - missing colon",
       .input = k_missing_colon_input,
       .expected = "",
       .should_succeed = k_missing_colon_should_succeed},
      {.name = "invalid - missing parens",
       .input = k_missing_parens_input,
       .expected = "",
       .should_succeed = k_missing_parens_should_succeed},
  };
  for (auto const& params: params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
