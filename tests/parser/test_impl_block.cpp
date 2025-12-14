#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Impl_Block;

PARSE_TEST(Impl_Block, impl_block)

namespace {
// Empty impl block
constexpr auto k_empty_impl_should_succeed = true;
constexpr auto k_empty_impl_input = "impl Point { }";
inline auto const k_empty_impl_expected = test_sexp::impl_block(test_sexp::type_name("Point"), {});

// Basic impl block with single method
constexpr auto k_basic_single_method_should_succeed = true;
constexpr auto k_basic_single_method_input = "impl Point { fn distance(self: Point): F64 { return 0.0; } }";
inline auto const k_basic_single_method_expected = test_sexp::impl_block(
    test_sexp::type_name("Point"),
    {test_sexp::func_def(
        test_sexp::func_decl(
            "distance",
            {},
            {test_sexp::function_parameter("self", test_sexp::type_name("Point"))},
            test_sexp::type_name("F64")
        ),
        test_sexp::block({test_sexp::return_statement("(float \"0.0\")")})
    )}
);

// Impl block with optional self type (no explicit type annotation)
constexpr auto k_optional_self_type_should_succeed = true;
constexpr auto k_optional_self_type_input = "impl Point { fn distance(self): F64 { return 0.0; } }";
inline auto const k_optional_self_type_expected = test_sexp::impl_block(
    test_sexp::type_name("Point"),
    {test_sexp::func_def(
        test_sexp::func_decl("distance", {}, {"(param false \"self\" nil)"}, test_sexp::type_name("F64")),
        test_sexp::block({test_sexp::return_statement("(float \"0.0\")")})
    )}
);

// Generic impl block
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "impl<T> Array<T> { fn len(self: Array<T>): I32 { return 0; } }";
inline auto const k_generic_single_param_expected = test_sexp::impl_block(
    R"((path ((type_segment "Array" ((path ((type_segment "T"))))))))",
    {test_sexp::func_def(
        test_sexp::func_decl(
            "len",
            {},
            {R"((param false "self" (path ((type_segment "Array" ((path ((type_segment "T")))))))))"},
            test_sexp::type_name("I32")
        ),
        test_sexp::block({test_sexp::return_statement(test_sexp::integer("0"))})
    )},
    {"(type_param (path ((type_segment \"T\"))))"}  // type_params
);

// Invalid cases
constexpr auto k_invalid_no_braces_should_succeed = false;
constexpr auto k_invalid_no_braces_input = "impl Point";
inline auto const k_invalid_no_braces_expected = test_sexp::impl_block("(path ())", {});

constexpr auto k_invalid_empty_should_succeed = false;
constexpr auto k_invalid_empty_input = "";
inline auto const k_invalid_empty_expected = test_sexp::impl_block("(path ())", {});
}  // namespace

TEST_CASE("Parse Impl_Block") {
  std::vector<Impl_Block_Params> const params_list = {
      {.name = "empty impl",
       .input = k_empty_impl_input,
       .expected = k_empty_impl_expected,
       .should_succeed = k_empty_impl_should_succeed},
      {.name = "basic single method",
       .input = k_basic_single_method_input,
       .expected = k_basic_single_method_expected,
       .should_succeed = k_basic_single_method_should_succeed},
      {.name = "optional self type",
       .input = k_optional_self_type_input,
       .expected = k_optional_self_type_expected,
       .should_succeed = k_optional_self_type_should_succeed},
      {.name = "generic single param",
       .input = k_generic_single_param_input,
       .expected = k_generic_single_param_expected,
       .should_succeed = k_generic_single_param_should_succeed},
      {.name = "invalid - no braces",
       .input = k_invalid_no_braces_input,
       .expected = k_invalid_no_braces_expected,
       .should_succeed = k_invalid_no_braces_should_succeed},
      {.name = "invalid - empty",
       .input = k_invalid_empty_input,
       .expected = k_invalid_empty_expected,
       .should_succeed = k_invalid_empty_should_succeed},
  };
  for (auto const& params : params_list) {
    SUBCASE(std::string(params.name).c_str()) {
      check_parse(params);
    }
  }
}
