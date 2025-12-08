#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Function_Definition;

namespace {

// ============================================================================
// UFCS Function Tests
// Test functions with 'self' parameter for Uniform Function Call Syntax
// ============================================================================

// Note: Function definition helpers are too complex to add at this time
// These tests use the raw parse_function_definition function instead

}  // namespace

TEST_CASE("Parse UFCS function with self parameter", "[parser][ufcs][function]") {
  // Test that functions with self parameter are parsed correctly
  // Note: These are syntactic tests - semantic analysis will handle UFCS desugaring

  std::string const input1 = "fn distance(self: Point): I32 { return 42; }";
  auto input_start1 = input1.cbegin();
  auto const result1 = life_lang::internal::parse_function_definition(input_start1, input1.cend());
  REQUIRE(result1.has_value());
  CHECK(result1->declaration.name == "distance");
  CHECK(result1->declaration.parameters.size() == 1);
  CHECK(result1->declaration.parameters[0].name == "self");

  std::string const input2 = "fn add(self: Point, x: I32, y: I32): Point { return self; }";
  auto input_start2 = input2.cbegin();
  auto const result2 = life_lang::internal::parse_function_definition(input_start2, input2.cend());
  REQUIRE(result2.has_value());
  CHECK(result2->declaration.name == "add");
  CHECK(result2->declaration.parameters.size() == 3);
  CHECK(result2->declaration.parameters[0].name == "self");
  CHECK(result2->declaration.parameters[1].name == "x");
  CHECK(result2->declaration.parameters[2].name == "y");
}

TEST_CASE("Parse module with UFCS functions", "[parser][ufcs][module]") {
  using life_lang::ast::Function_Definition;
  using life_lang::ast::Struct_Definition;
  using life_lang::parser::parse_module;

  std::string const input =
      "struct Point { x: I32, y: I32 }\n"
      "\n"
      "fn new_point(x: I32, y: I32): Point {\n"
      "  return Point { x: x, y: y };\n"
      "}\n"
      "\n"
      "fn distance(self: Point): I32 {\n"
      "  return 42;\n"
      "}\n"
      "\n"
      "fn add(self: Point, other: Point): Point {\n"
      "  return Point { x: self.x, y: other.x };\n"
      "}\n";

  auto result = parse_module(input, "ufcs_test.life");
  if (!result.has_value()) {
    UNSCOPED_INFO(result.error());
  }
  REQUIRE(result.has_value());

  auto const& mod = result.value();
  REQUIRE(mod.statements.size() == 4);

  // First statement is struct definition
  auto const* struct_def_ptr = boost::get<boost::spirit::x3::forward_ast<Struct_Definition>>(mod.statements.data());
  REQUIRE(struct_def_ptr != nullptr);

  // Second statement is regular function
  auto const* new_point_ptr =
      boost::get<boost::spirit::x3::forward_ast<Function_Definition>>(mod.statements.data() + 1);
  REQUIRE(new_point_ptr != nullptr);
  auto const& new_point = new_point_ptr->get();
  REQUIRE(new_point.declaration.name == "new_point");
  REQUIRE(new_point.declaration.parameters.size() == 2);
  REQUIRE(new_point.declaration.parameters[0].name == "x");
  REQUIRE(new_point.declaration.parameters[1].name == "y");

  // Third statement is UFCS function with self only
  auto const* distance_ptr = boost::get<boost::spirit::x3::forward_ast<Function_Definition>>(mod.statements.data() + 2);
  REQUIRE(distance_ptr != nullptr);
  auto const& distance = distance_ptr->get();
  REQUIRE(distance.declaration.name == "distance");
  REQUIRE(distance.declaration.parameters.size() == 1);
  REQUIRE(distance.declaration.parameters[0].name == "self");
  REQUIRE(distance.declaration.parameters[0].type.segments.size() == 1);
  REQUIRE(distance.declaration.parameters[0].type.segments[0].value == "Point");

  // Fourth statement is UFCS function with self and other parameters
  auto const* add_ptr = boost::get<boost::spirit::x3::forward_ast<Function_Definition>>(mod.statements.data() + 3);
  REQUIRE(add_ptr != nullptr);
  auto const& add = add_ptr->get();
  REQUIRE(add.declaration.name == "add");
  REQUIRE(add.declaration.parameters.size() == 2);
  REQUIRE(add.declaration.parameters[0].name == "self");
  REQUIRE(add.declaration.parameters[1].name == "other");
}
