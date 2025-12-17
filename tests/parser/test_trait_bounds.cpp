//! Test trait bounds parsing
//! Tests: fn foo<T: Display>, struct Bar<T: Clone>, etc.

#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Func_Def;

PARSE_TEST(Func_Def, func_def)

namespace {
constexpr auto k_single_bound_should_succeed = true;
constexpr auto k_single_bound_input = "fn foo<T: Display>(x: T): I32 { return 0; }";
inline auto const k_single_bound_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {R"((type_param (path ((type_segment "T"))) ((trait_bound (path ((type_segment "Display")))))))"},
        {R"((param false "x" (path ((type_segment "T")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

constexpr auto k_no_bound_should_succeed = true;
constexpr auto k_no_bound_input = "fn foo<T>(x: T): I32 { return 0; }";
inline auto const k_no_bound_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {test_sexp::type_param(test_sexp::type_name("T"))},
        {R"((param false "x" (path ((type_segment "T")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Mixed bounded and unbounded parameters
constexpr auto k_mixed_bounds_should_succeed = true;
constexpr auto k_mixed_bounds_input = "fn foo<T: Display, U>(x: T, y: U): I32 { return 0; }";
inline auto const k_mixed_bounds_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {R"((type_param (path ((type_segment "T"))) ((trait_bound (path ((type_segment "Display")))))))",
         test_sexp::type_param(test_sexp::type_name("U"))},
        {R"((param false "x" (path ((type_segment "T")))))", R"((param false "y" (path ((type_segment "U")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Qualified trait name (e.g., Std.Display)
constexpr auto k_qualified_trait_should_succeed = true;
constexpr auto k_qualified_trait_input = "fn foo<T: Std.Display>(x: T): I32 { return 0; }";
inline auto const k_qualified_trait_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {"(type_param (path ((type_segment \"T\"))) ((trait_bound (path ((type_segment \"Std\") (type_segment "
         "\"Display\"))))))"},
        {R"((param false "x" (path ((type_segment "T")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Multiple parameters, all with bounds
constexpr auto k_all_bounded_should_succeed = true;
constexpr auto k_all_bounded_input = "fn foo<T: Display, U: Clone>(x: T, y: U): I32 { return 0; }";
inline auto const k_all_bounded_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {R"((type_param (path ((type_segment "T"))) ((trait_bound (path ((type_segment "Display")))))))",
         R"((type_param (path ((type_segment "U"))) ((trait_bound (path ((type_segment "Clone")))))))"},
        {R"((param false "x" (path ((type_segment "T")))))", R"((param false "y" (path ((type_segment "U")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Error case: Missing trait name after colon
constexpr auto k_missing_trait_should_succeed = false;
constexpr auto k_missing_trait_input = "fn foo<T:>(x: T): I32 { return 0; }";
inline auto const k_missing_trait_expected = "";

// Multiple bounds with +
constexpr auto k_multiple_bounds_should_succeed = true;
constexpr auto k_multiple_bounds_input = "fn foo<T: Display + Clone>(x: T): I32 { return 0; }";
inline auto const k_multiple_bounds_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {"(type_param (path ((type_segment \"T\"))) ((trait_bound (path ((type_segment \"Display\")))) (trait_bound "
         "(path ((type_segment \"Clone\"))))))"},
        {R"((param false "x" (path ((type_segment "T")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Three bounds
constexpr auto k_three_bounds_should_succeed = true;
constexpr auto k_three_bounds_input = "fn foo<T: Eq + Ord + Hash>(x: T): I32 { return 0; }";
inline auto const k_three_bounds_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {"(type_param (path ((type_segment \"T\"))) ((trait_bound (path ((type_segment \"Eq\")))) (trait_bound (path "
         "((type_segment \"Ord\")))) (trait_bound (path ((type_segment \"Hash\"))))))"},
        {R"((param false "x" (path ((type_segment "T")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Multiple params with multiple bounds each
constexpr auto k_multiple_params_multiple_bounds_should_succeed = true;
constexpr auto k_multiple_params_multiple_bounds_input =
    "fn foo<T: Display + Clone, U: Eq + Ord>(x: T, y: U): I32 { return 0; }";
inline auto const k_multiple_params_multiple_bounds_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {"(type_param (path ((type_segment \"T\"))) ((trait_bound (path ((type_segment \"Display\")))) (trait_bound "
         "(path ((type_segment \"Clone\"))))))",
         "(type_param (path ((type_segment \"U\"))) ((trait_bound (path ((type_segment \"Eq\")))) (trait_bound (path "
         "((type_segment \"Ord\"))))))"},
        {R"((param false "x" (path ((type_segment "T")))))", R"((param false "y" (path ((type_segment "U")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

// Qualified trait names with multiple bounds
constexpr auto k_qualified_multiple_bounds_should_succeed = true;
constexpr auto k_qualified_multiple_bounds_input = "fn foo<T: Std.Display + Std.Clone>(x: T): I32 { return 0; }";
inline auto const k_qualified_multiple_bounds_expected = test_sexp::func_def(
    test_sexp::func_decl(
        "foo",
        {"(type_param (path ((type_segment \"T\"))) ((trait_bound (path ((type_segment \"Std\") (type_segment "
         "\"Display\")))) (trait_bound (path ((type_segment \"Std\") (type_segment \"Clone\"))))))"},
        {R"((param false "x" (path ((type_segment "T")))))"},
        test_sexp::type_name("I32")
    ),
    test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
);

}  // namespace

TEST_CASE("Parse Func_Def with trait bounds") {
  std::vector<Func_Def_Params> const params_list = {
      // Single inline bounds
      {.name = "single trait bound",
       .input = k_single_bound_input,
       .expected = k_single_bound_expected,
       .should_succeed = k_single_bound_should_succeed},
      {.name = "no trait bound (backward compat)",
       .input = k_no_bound_input,
       .expected = k_no_bound_expected,
       .should_succeed = k_no_bound_should_succeed},
      {.name = "mixed bounded/unbounded params",
       .input = k_mixed_bounds_input,
       .expected = k_mixed_bounds_expected,
       .should_succeed = k_mixed_bounds_should_succeed},
      {.name = "qualified trait name",
       .input = k_qualified_trait_input,
       .expected = k_qualified_trait_expected,
       .should_succeed = k_qualified_trait_should_succeed},
      {.name = "all params bounded",
       .input = k_all_bounded_input,
       .expected = k_all_bounded_expected,
       .should_succeed = k_all_bounded_should_succeed},

      // Multiple bounds with +
      {.name = "multiple bounds with +",
       .input = k_multiple_bounds_input,
       .expected = k_multiple_bounds_expected,
       .should_succeed = k_multiple_bounds_should_succeed},
      {.name = "three bounds",
       .input = k_three_bounds_input,
       .expected = k_three_bounds_expected,
       .should_succeed = k_three_bounds_should_succeed},
      {.name = "multiple params multiple bounds",
       .input = k_multiple_params_multiple_bounds_input,
       .expected = k_multiple_params_multiple_bounds_expected,
       .should_succeed = k_multiple_params_multiple_bounds_should_succeed},
      {.name = "qualified multiple bounds",
       .input = k_qualified_multiple_bounds_input,
       .expected = k_qualified_multiple_bounds_expected,
       .should_succeed = k_qualified_multiple_bounds_should_succeed},

      // Error cases
      {.name = "missing trait name after colon",
       .input = k_missing_trait_input,
       .expected = k_missing_trait_expected,
       .should_succeed = k_missing_trait_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
