//! Test trait bounds parsing
//! Tests: fn foo<T: Display>, struct Bar<T: Clone>, etc.

#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Func_Def;

PARSE_TEST(Func_Def, func_def)

namespace {
// Basic case
constexpr auto k_single_bound_should_succeed = true;
constexpr auto k_single_bound_input = "fn foo<T: Display>(x: T): I32 { return 0; }";

// Backward compatibility: No trait bound (ensure existing code still works)
constexpr auto k_no_bound_should_succeed = true;
constexpr auto k_no_bound_input = "fn foo<T>(x: T): I32 { return 0; }";

// Mixed bounded and unbounded parameters
constexpr auto k_mixed_bounds_should_succeed = true;
constexpr auto k_mixed_bounds_input = "fn foo<T: Display, U>(x: T, y: U): I32 { return 0; }";

// Qualified trait name (e.g., Std.Display)
constexpr auto k_qualified_trait_should_succeed = true;
constexpr auto k_qualified_trait_input = "fn foo<T: Std.Display>(x: T): I32 { return 0; }";

// Multiple parameters, all with bounds
constexpr auto k_all_bounded_should_succeed = true;
constexpr auto k_all_bounded_input = "fn foo<T: Display, U: Clone>(x: T, y: U): I32 { return 0; }";

// Error case: Missing trait name after colon
constexpr auto k_missing_trait_should_succeed = false;
constexpr auto k_missing_trait_input = "fn foo<T:>(x: T): I32 { return 0; }";

// Multiple bounds with +
constexpr auto k_multiple_bounds_should_succeed = true;
constexpr auto k_multiple_bounds_input = "fn foo<T: Display + Clone>(x: T): I32 { return 0; }";

// Three bounds
constexpr auto k_three_bounds_should_succeed = true;
constexpr auto k_three_bounds_input = "fn foo<T: Eq + Ord + Hash>(x: T): I32 { return 0; }";

// Multiple params with multiple bounds each
constexpr auto k_multiple_params_multiple_bounds_should_succeed = true;
constexpr auto k_multiple_params_multiple_bounds_input =
    "fn foo<T: Display + Clone, U: Eq + Ord>(x: T, y: U): I32 { return 0; }";

// Qualified trait names with multiple bounds
constexpr auto k_qualified_multiple_bounds_should_succeed = true;
constexpr auto k_qualified_multiple_bounds_input = "fn foo<T: Std.Display + Std.Clone>(x: T): I32 { return 0; }";

}  // namespace

TEST_CASE("Parse Func_Def with trait bounds", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Func_Def_Params>({
          // Single inline bounds
          {"single trait bound", k_single_bound_input, "", k_single_bound_should_succeed},
          {"no trait bound (backward compat)", k_no_bound_input, "", k_no_bound_should_succeed},
          {"mixed bounded/unbounded params", k_mixed_bounds_input, "", k_mixed_bounds_should_succeed},
          {"qualified trait name", k_qualified_trait_input, "", k_qualified_trait_should_succeed},
          {"all params bounded", k_all_bounded_input, "", k_all_bounded_should_succeed},

          // Multiple bounds with +
          {"multiple bounds with +", k_multiple_bounds_input, "", k_multiple_bounds_should_succeed},
          {"three bounds", k_three_bounds_input, "", k_three_bounds_should_succeed},
          {"multiple params multiple bounds",
           k_multiple_params_multiple_bounds_input,
           "",
           k_multiple_params_multiple_bounds_should_succeed},
          {"qualified multiple bounds",
           k_qualified_multiple_bounds_input,
           "",
           k_qualified_multiple_bounds_should_succeed},

          // Error cases
          {"missing trait name after colon", k_missing_trait_input, "", k_missing_trait_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
