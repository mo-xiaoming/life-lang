#include <doctest/doctest.h>
#include "parser.hpp"
#include "sexp.hpp"

TEST_CASE("Tuple types and literals in function signatures") {
  auto const* const input = R"(
    fn create_point(x: I32, y: I32): (I32, I32) {
      return (x, y);
    }
    
    fn swap(pair: (I32, I32)): (I32, I32) {
      let (a, b) = pair;
      return (b, a);
    }
    
    fn nested_tuples(): ((I32, I32), (String, Bool)) {
      return ((1, 2), ("hello", true));
    }
    
    fn complex_signature(data: (Vec<I32>, Map<String, I32>)): (Bool, [I32; 4]) {
      return (true, [1, 2, 3, 4]);
    }
  )";

  life_lang::parser::Parser parser(input);
  auto const module = parser.parse_module();

  REQUIRE(module.has_value());
  CHECK(module->items.size() == 4);

  // Verify all functions parsed successfully (item.item is a Statement variant)
  for (auto const& item : module->items) {
    CHECK(std::holds_alternative<std::shared_ptr<life_lang::ast::Func_Def>>(item.item));
  }

  // Print S-expression for verification
  auto const sexp = life_lang::ast::to_sexp_string(*module, 2);
  INFO("Module S-expression:\n", sexp);

  // Verify tuple types appear in signatures
  CHECK(sexp.find("tuple_type") != std::string::npos);

  // Verify tuple literals appear in bodies
  CHECK(sexp.find("tuple_lit") != std::string::npos);
}
