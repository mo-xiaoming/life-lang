#include <doctest/doctest.h>
#include <parser.hpp>
#include <sexp.hpp>

using life_lang::ast::to_sexp_string;
using life_lang::parser::Parser;

TEST_CASE("Float special literals integration") {
  std::string const source = R"(
    fn is_special(value: F64): Bool {
      return value == nan || value == inf || value == -inf;
    }

    fn get_nan(): F32 {
      return nanF32;
    }

    fn get_infinity(): F64 {
      return infF64;
    }
  )";

  Parser parser{source};
  auto const result = parser.parse_module();

  REQUIRE(result);
  CHECK(result->items.size() == 3);

  // Verify S-expression output contains our special literals
  auto const sexp = to_sexp_string(*result, 0);
  CHECK(sexp.find(R"((float "nan"))") != std::string::npos);
  CHECK(sexp.find(R"((float "inf"))") != std::string::npos);
  CHECK(sexp.find(R"((float "nan" "F32"))") != std::string::npos);
  CHECK(sexp.find(R"((float "inf" "F64"))") != std::string::npos);
}
