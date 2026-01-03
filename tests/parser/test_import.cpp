#include "internal_rules.hpp"
#include "utils.hpp"

using namespace life_lang::parser;
using namespace test_sexp;

TEST_CASE("parse_import_statement") {
  struct Test_Case {
    char const* name;
    char const* input;
    std::string expected;
  };

  static Test_Case const k_test_cases[] = {
      {.name = "simple import",
       .input = "import Geometry.{Point};",
       .expected = import_statement({"Geometry"}, {import_item("Point")})},
      {.name = "nested module",
       .input = "import Geometry.Shapes.{Polygon, Triangle};",
       .expected = import_statement({"Geometry", "Shapes"}, {import_item("Polygon"), import_item("Triangle")})},
      {.name = "multiple items",
       .input = "import Math.{add, multiply, divide};",
       .expected = import_statement({"Math"}, {import_item("add"), import_item("multiply"), import_item("divide")})},
      {.name = "deeply nested",
       .input = "import A.B.C.D.{Item};",
       .expected = import_statement({"A", "B", "C", "D"}, {import_item("Item")})},
      {.name = "with as alias",
       .input = "import Geometry.{Point as P};",
       .expected = import_statement({"Geometry"}, {import_item("Point", "P")})},
      {.name = "mixed as and no as",
       .input = "import Geometry.{Point as P, Circle, Line as L};",
       .expected = import_statement(
           {"Geometry"},
           {import_item("Point", "P"), import_item("Circle"), import_item("Line", "L")}
       )},
      {.name = "function with as",
       .input = "import Math.{calculate_distance as dist};",
       .expected = import_statement({"Math"}, {import_item("calculate_distance", "dist")})},
  };

  for (auto const& tc: k_test_cases) {
    SUBCASE(tc.name) {
      life_lang::Diagnostic_Engine diagnostics{"<test>", tc.input};

      life_lang::parser::Parser parser{diagnostics};
      auto const result = parser.parse_import_statement();
      REQUIRE(result.has_value());
      if (result.has_value()) {
        auto const sexp = to_sexp_string(*result, 0);
        CHECK(sexp == tc.expected);
      }
    }
  }
}
