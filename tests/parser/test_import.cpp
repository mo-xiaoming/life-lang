#include <doctest/doctest.h>

#include "../../src/parser.hpp"

TEST_CASE("parse_import_statement - simple import") {
  life_lang::parser::Parser parser("import Geometry.{Point};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->module_path.size() == 1);
    CHECK(result->module_path[0] == "Geometry");
    CHECK(result->items.size() == 1);
    CHECK(result->items[0].name == "Point");
    CHECK_FALSE(result->items[0].alias.has_value());
  }
}

TEST_CASE("parse_import_statement - nested module") {
  life_lang::parser::Parser parser("import Geometry.Shapes.{Polygon, Triangle};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->module_path.size() == 2);
    CHECK(result->module_path[0] == "Geometry");
    CHECK(result->module_path[1] == "Shapes");
    CHECK(result->items.size() == 2);
    CHECK(result->items[0].name == "Polygon");
    CHECK(result->items[1].name == "Triangle");
  }
}

TEST_CASE("parse_import_statement - multiple items") {
  life_lang::parser::Parser parser("import Math.{add, multiply, divide};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->module_path.size() == 1);
    CHECK(result->module_path[0] == "Math");
    CHECK(result->items.size() == 3);
    CHECK(result->items[0].name == "add");
    CHECK(result->items[1].name == "multiply");
    CHECK(result->items[2].name == "divide");
  }
}

TEST_CASE("parse_import_statement - deeply nested") {
  life_lang::parser::Parser parser("import A.B.C.D.{Item};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->module_path.size() == 4);
    CHECK(result->module_path[0] == "A");
    CHECK(result->module_path[1] == "B");
    CHECK(result->module_path[2] == "C");
    CHECK(result->module_path[3] == "D");
    CHECK(result->items.size() == 1);
    CHECK(result->items[0].name == "Item");
  }
}

TEST_CASE("parse_module - with imports") {
  life_lang::parser::Parser parser(R"(
import Geometry.{Point};
import Math.{add};

fn main(): I32 {
  return 42;
}
)");

  auto const result = parser.parse_module();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->imports.size() == 2);
    CHECK(result->imports[0].module_path[0] == "Geometry");
    CHECK(result->imports[0].items[0].name == "Point");
    CHECK(result->imports[1].module_path[0] == "Math");
    CHECK(result->imports[1].items[0].name == "add");
    CHECK(result->items.size() == 1);
    CHECK_FALSE(result->items[0].is_pub);  // No pub modifier
  }
}

TEST_CASE("parse_module - with pub modifier") {
  life_lang::parser::Parser parser(R"(
pub fn exported(): I32 {
  return 1;
}

fn internal(): I32 {
  return 2;
}
)");

  auto const result = parser.parse_module();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->items.size() == 2);
    CHECK(result->items[0].is_pub == true);   // pub fn exported
    CHECK(result->items[1].is_pub == false);  // fn internal (no pub)
  }
}

TEST_CASE("parse_module - pub struct") {
  life_lang::parser::Parser parser(R"(
pub struct Point { x: I32, y: I32 }

struct Internal { data: I32 }
)");

  auto const result = parser.parse_module();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->items.size() == 2);
    CHECK(result->items[0].is_pub == true);   // pub struct Point
    CHECK(result->items[1].is_pub == false);  // struct Internal
  }
}

TEST_CASE("parse_module - complete example with imports and pub") {
  life_lang::parser::Parser parser(R"(
import Std.IO.{println};
import Geometry.{Point, Circle};

pub struct App {
  name: String
}

pub fn main(): I32 {
  return 0;
}

fn helper(): () {
  return ();
}
)");

  auto const result = parser.parse_module();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->imports.size() == 2);
    CHECK(result->imports[0].module_path.size() == 2);
    CHECK(result->imports[0].module_path[0] == "Std");
    CHECK(result->imports[0].module_path[1] == "IO");
    CHECK(result->imports[0].items[0].name == "println");

    CHECK(result->items.size() == 3);
    CHECK(result->items[0].is_pub == true);   // pub struct App
    CHECK(result->items[1].is_pub == true);   // pub fn main
    CHECK(result->items[2].is_pub == false);  // fn helper
  }
}

TEST_CASE("parse_import_statement - with as alias") {
  life_lang::parser::Parser parser("import Geometry.{Point as P};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->module_path.size() == 1);
    CHECK(result->module_path[0] == "Geometry");
    CHECK(result->items.size() == 1);
    CHECK(result->items[0].name == "Point");
    CHECK(result->items[0].alias.has_value());
    auto const& alias = result->items[0].alias;
    if (alias.has_value()) {
      CHECK(alias.value() == "P");
    }
  }
}

TEST_CASE("parse_import_statement - mixed as and no as") {
  life_lang::parser::Parser parser("import Geometry.{Point as P, Circle, Line as L};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->items.size() == 3);
    CHECK(result->items[0].name == "Point");
    auto const& alias0 = result->items[0].alias;
    CHECK(alias0.has_value());
    if (alias0.has_value()) {
      CHECK(alias0.value() == "P");
    }

    CHECK(result->items[1].name == "Circle");
    CHECK_FALSE(result->items[1].alias.has_value());

    CHECK(result->items[2].name == "Line");
    auto const& alias2 = result->items[2].alias;
    CHECK(alias2.has_value());
    if (alias2.has_value()) {
      CHECK(alias2.value() == "L");
    }
  }
}

TEST_CASE("parse_import_statement - function with as") {
  life_lang::parser::Parser parser("import Math.{calculate_distance as dist};");
  auto const result = parser.parse_import_statement();

  REQUIRE(result.has_value());
  if (result.has_value()) {
    CHECK(result->items[0].name == "calculate_distance");
    auto const& alias = result->items[0].alias;
    CHECK(alias.has_value());
    if (alias.has_value()) {
      CHECK(alias.value() == "dist");
    }
  }
}

TEST_CASE("parse_module - sexp output with imports") {
  life_lang::parser::Parser parser(R"(
import Math.{add};

pub fn test(): I32 { return 1; }
)");

  auto const result = parser.parse_module();
  REQUIRE(result.has_value());

  if (result.has_value()) {
    // Verify structure directly
    CHECK(result->imports.size() == 1);
    CHECK(result->imports[0].module_path.size() == 1);
    CHECK(result->imports[0].module_path[0] == "Math");
    CHECK(result->imports[0].items.size() == 1);
    CHECK(result->imports[0].items[0].name == "add");

    CHECK(result->items.size() == 1);
    CHECK(result->items[0].is_pub == true);
  }
}
