#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

namespace {

constexpr auto k_pub_methods_input = R"(
impl Point {
  pub fn new(x: I32, y: I32): Point {
    return Point { x: x, y: y };
  }
  
  fn internal_helper(self): I32 {
    return self.x;
  }
  
  pub fn distance(self): F64 {
    return 0.0;
  }
}
)";
inline auto const k_pub_methods_expected = impl_block(
    type_name("Point"),
    {
        func_def(
            func_decl(
                "new",
                {},
                {function_parameter("x", type_name("I32")), function_parameter("y", type_name("I32"))},
                type_name("Point")
            ),
            block({return_statement(
                struct_literal("Point", {field_init("x", var_name("x")), field_init("y", var_name("y"))})
            )}),
            true
        ),
        func_def(
            func_decl("internal_helper", {}, {function_parameter("self", "nil")}, type_name("I32")),
            block({return_statement(field_access(var_name("self"), "x"))}),
            false
        ),
        func_def(
            func_decl("distance", {}, {function_parameter("self", "nil")}, type_name("F64")),
            block({return_statement(float_literal("0.0"))}),
            true
        ),
    }
);

constexpr auto k_all_pub_methods_input = R"(
impl Calculator {
  pub fn add(a: I32, b: I32): I32 {
    return a + b;
  }
  
  pub fn subtract(a: I32, b: I32): I32 {
    return a - b;
  }
}
)";
inline auto const k_all_pub_methods_expected = impl_block(
    type_name("Calculator"),
    {
        func_def(
            func_decl(
                "add",
                {},
                {function_parameter("a", type_name("I32")), function_parameter("b", type_name("I32"))},
                type_name("I32")
            ),
            block({return_statement(binary_expr("+", var_name("a"), var_name("b")))}),
            true
        ),
        func_def(
            func_decl(
                "subtract",
                {},
                {function_parameter("a", type_name("I32")), function_parameter("b", type_name("I32"))},
                type_name("I32")
            ),
            block({return_statement(binary_expr("-", var_name("a"), var_name("b")))}),
            true
        ),
    }
);

constexpr auto k_no_pub_methods_input = R"(
impl Internal {
  fn helper(): I32 {
    return 42;
  }
}
)";
inline auto const k_no_pub_methods_expected = impl_block(
    type_name("Internal"),
    {
        func_def(func_decl("helper", {}, {}, type_name("I32")), block({return_statement(integer("42"))}), false),
    }
);

}  // namespace

TEST_CASE("parse_impl_block - pub methods") {
  struct Test_Case {
    std::string_view name;
    std::string_view input;
    std::string expected;
  };

  std::vector<Test_Case> const test_cases = {
      {.name = "pub methods", .input = k_pub_methods_input, .expected = k_pub_methods_expected},
      {.name = "all pub methods", .input = k_all_pub_methods_input, .expected = k_all_pub_methods_expected},
      {.name = "no pub methods", .input = k_no_pub_methods_input, .expected = k_no_pub_methods_expected},
  };

  for (auto const& tc: test_cases) {
    SUBCASE(std::string(tc.name).c_str()) {
      life_lang::Source_File_Registry registry;
    life_lang::File_Id const file_id = registry.register_file("<test>", std::string{tc.input});
    life_lang::Diagnostic_Engine diagnostics{registry, file_id};

      life_lang::parser::Parser parser{diagnostics};
      auto const result = parser.parse_impl_block();
      REQUIRE(result.has_value());
      if (result.has_value()) {
        CHECK(to_sexp_string(*result, 0) == tc.expected);
      }
    }
  }
}
