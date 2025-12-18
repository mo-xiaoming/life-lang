// Integration test for array types in function signatures

#include <doctest/doctest.h>

#include "parser.hpp"

TEST_CASE("Array types in function signatures") {
  auto const* const input = R"(
fn process_array(arr: [I32; 10]): I32 {
  return 42;
}

fn matrix_func(mat: [[I32; 4]; 3]): Bool {
  return true;
}
)";

  life_lang::parser::Parser parser{input, "test.life"};
  auto const result = parser.parse_module();

  REQUIRE(result.has_value());
  auto const& module = *result;

  // Should have 2 function definitions
  REQUIRE(module.items.size() == 2);

  // Check both are function definitions (Item.item is the Statement variant containing shared_ptr<Func_Def>)
  REQUIRE(std::holds_alternative<std::shared_ptr<life_lang::ast::Func_Def>>(module.items[0].item));
  REQUIRE(std::holds_alternative<std::shared_ptr<life_lang::ast::Func_Def>>(module.items[1].item));
}