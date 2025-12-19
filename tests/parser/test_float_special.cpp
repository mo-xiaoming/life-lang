#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {

// === NaN literals ===

constexpr auto k_nan_lowercase_should_succeed = true;
constexpr auto k_nan_lowercase_input = "nan";
constexpr auto k_nan_lowercase_expected = R"((float "nan"))";

constexpr auto k_nan_uppercase_should_succeed = true;
constexpr auto k_nan_uppercase_input = "NaN";
constexpr auto k_nan_uppercase_expected = R"((float "nan"))";

constexpr auto k_nan_allcaps_should_succeed = true;
constexpr auto k_nan_allcaps_input = "NAN";
constexpr auto k_nan_allcaps_expected = R"((float "nan"))";

constexpr auto k_nan_mixedcase_should_succeed = true;
constexpr auto k_nan_mixedcase_input = "Nan";
constexpr auto k_nan_mixedcase_expected = R"((float "nan"))";

constexpr auto k_nan_f32_suffix_should_succeed = true;
constexpr auto k_nan_f32_suffix_input = "nanF32";
constexpr auto k_nan_f32_suffix_expected = R"((float "nan" "F32"))";

constexpr auto k_nan_f64_suffix_should_succeed = true;
constexpr auto k_nan_f64_suffix_input = "nanF64";
constexpr auto k_nan_f64_suffix_expected = R"((float "nan" "F64"))";

// === Inf literals ===

constexpr auto k_inf_lowercase_should_succeed = true;
constexpr auto k_inf_lowercase_input = "inf";
constexpr auto k_inf_lowercase_expected = R"((float "inf"))";

constexpr auto k_inf_uppercase_should_succeed = true;
constexpr auto k_inf_uppercase_input = "Inf";
constexpr auto k_inf_uppercase_expected = R"((float "inf"))";

constexpr auto k_inf_allcaps_should_succeed = true;
constexpr auto k_inf_allcaps_input = "INF";
constexpr auto k_inf_allcaps_expected = R"((float "inf"))";

constexpr auto k_inf_f32_suffix_should_succeed = true;
constexpr auto k_inf_f32_suffix_input = "infF32";
constexpr auto k_inf_f32_suffix_expected = R"((float "inf" "F32"))";

constexpr auto k_inf_f64_suffix_should_succeed = true;
constexpr auto k_inf_f64_suffix_input = "infF64";
constexpr auto k_inf_f64_suffix_expected = R"((float "inf" "F64"))";

// === Special literals in expressions ===

constexpr auto k_negative_inf_should_succeed = true;
constexpr auto k_negative_inf_input = "-inf";
inline auto const k_negative_inf_expected = test_sexp::unary_expr("-", "(float \"inf\")");

constexpr auto k_negative_nan_should_succeed = true;
constexpr auto k_negative_nan_input = "-nan";
inline auto const k_negative_nan_expected = test_sexp::unary_expr("-", "(float \"nan\")");

constexpr auto k_inf_in_addition_should_succeed = true;
constexpr auto k_inf_in_addition_input = "x + inf";
inline auto const k_inf_in_addition_expected = test_sexp::binary_expr("+", test_sexp::var_name("x"), "(float \"inf\")");

constexpr auto k_nan_in_comparison_should_succeed = true;
constexpr auto k_nan_in_comparison_input = "value == nan";
inline auto const k_nan_in_comparison_expected =
    test_sexp::binary_expr("==", test_sexp::var_name("value"), "(float \"nan\")");

constexpr auto k_inf_in_function_call_should_succeed = true;
constexpr auto k_inf_in_function_call_input = "process(inf)";
inline auto const k_inf_in_function_call_expected =
    test_sexp::function_call(test_sexp::var_name("process"), {"(float \"inf\")"});

// === Not confused with identifiers ===

constexpr auto k_nancy_identifier_should_succeed = true;
constexpr auto k_nancy_identifier_input = "nancy";
inline auto const k_nancy_identifier_expected = test_sexp::var_name("nancy");

constexpr auto k_info_identifier_should_succeed = true;
constexpr auto k_info_identifier_input = "info";
inline auto const k_info_identifier_expected = test_sexp::var_name("info");

}  // namespace

TEST_CASE("Parse Float Special Literals") {
  std::vector<Expr_Params> const params_list = {
      // NaN literals
      {.name = "nan lowercase",
       .input = k_nan_lowercase_input,
       .expected = k_nan_lowercase_expected,
       .should_succeed = k_nan_lowercase_should_succeed},
      {.name = "NaN uppercase",
       .input = k_nan_uppercase_input,
       .expected = k_nan_uppercase_expected,
       .should_succeed = k_nan_uppercase_should_succeed},
      {.name = "NAN all caps",
       .input = k_nan_allcaps_input,
       .expected = k_nan_allcaps_expected,
       .should_succeed = k_nan_allcaps_should_succeed},
      {.name = "Nan mixed case",
       .input = k_nan_mixedcase_input,
       .expected = k_nan_mixedcase_expected,
       .should_succeed = k_nan_mixedcase_should_succeed},
      {.name = "nan with F32 suffix",
       .input = k_nan_f32_suffix_input,
       .expected = k_nan_f32_suffix_expected,
       .should_succeed = k_nan_f32_suffix_should_succeed},
      {.name = "nan with F64 suffix",
       .input = k_nan_f64_suffix_input,
       .expected = k_nan_f64_suffix_expected,
       .should_succeed = k_nan_f64_suffix_should_succeed},
      // Inf literals
      {.name = "inf lowercase",
       .input = k_inf_lowercase_input,
       .expected = k_inf_lowercase_expected,
       .should_succeed = k_inf_lowercase_should_succeed},
      {.name = "Inf uppercase",
       .input = k_inf_uppercase_input,
       .expected = k_inf_uppercase_expected,
       .should_succeed = k_inf_uppercase_should_succeed},
      {.name = "INF all caps",
       .input = k_inf_allcaps_input,
       .expected = k_inf_allcaps_expected,
       .should_succeed = k_inf_allcaps_should_succeed},
      {.name = "inf with F32 suffix",
       .input = k_inf_f32_suffix_input,
       .expected = k_inf_f32_suffix_expected,
       .should_succeed = k_inf_f32_suffix_should_succeed},
      {.name = "inf with F64 suffix",
       .input = k_inf_f64_suffix_input,
       .expected = k_inf_f64_suffix_expected,
       .should_succeed = k_inf_f64_suffix_should_succeed},
      // Special literals in expressions
      {.name = "negative infinity",
       .input = k_negative_inf_input,
       .expected = k_negative_inf_expected,
       .should_succeed = k_negative_inf_should_succeed},
      {.name = "negative NaN",
       .input = k_negative_nan_input,
       .expected = k_negative_nan_expected,
       .should_succeed = k_negative_nan_should_succeed},
      {.name = "inf in addition",
       .input = k_inf_in_addition_input,
       .expected = k_inf_in_addition_expected,
       .should_succeed = k_inf_in_addition_should_succeed},
      {.name = "nan in comparison",
       .input = k_nan_in_comparison_input,
       .expected = k_nan_in_comparison_expected,
       .should_succeed = k_nan_in_comparison_should_succeed},
      {.name = "inf in function call",
       .input = k_inf_in_function_call_input,
       .expected = k_inf_in_function_call_expected,
       .should_succeed = k_inf_in_function_call_should_succeed},
      // Not confused with identifiers
      {.name = "nancy identifier",
       .input = k_nancy_identifier_input,
       .expected = k_nancy_identifier_expected,
       .should_succeed = k_nancy_identifier_should_succeed},
      {.name = "info identifier",
       .input = k_info_identifier_input,
       .expected = k_info_identifier_expected,
       .should_succeed = k_info_identifier_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
