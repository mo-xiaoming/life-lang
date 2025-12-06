#include "utils.hpp"

using life_lang::ast::Function_Definition;
using test_json::type_name;
using test_json::var_name;

PARSE_TEST(Function_Definition, function_definition)

namespace {

// ============================================================================
// UFCS Function Tests
// Test functions with 'self' parameter for Uniform Function Call Syntax
// ============================================================================

// Basic function with self only
constexpr auto k_self_only_input = "fn distance(self: Point): I32 { return 42; }";
inline auto const k_self_only_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "distance",
        "parameters": [{{
          "Function_Parameter": {{
            "is_mut": false,
            "name": "self",
            "type": {}
          }}
        }}],
        "returnType": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [{{
          "Return_Statement": {{
            "expr": {{
              "Integer": {{
                "value": "42"
              }}
            }}
          }}
        }}]
      }}
    }}
  }}
}})",
    type_name("Point"), type_name("I32")
);

// Function with self and additional parameters
constexpr auto k_self_with_params_input = "fn add(self: Point, x: I32, y: I32): Point { return self; }";
inline auto const k_self_with_params_expected = fmt::format(
    R"({{
  "Function_Definition": {{
    "declaration": {{
      "Function_Declaration": {{
        "name": "add",
        "parameters": [
          {{
            "Function_Parameter": {{
              "is_mut": false,
              "name": "self",
              "type": {}
            }}
          }},
          {{
            "Function_Parameter": {{
              "is_mut": false,
              "name": "x",
              "type": {}
            }}
          }},
          {{
            "Function_Parameter": {{
              "is_mut": false,
              "name": "y",
              "type": {}
            }}
          }}
        ],
        "returnType": {}
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [{{
          "Return_Statement": {{
            "expr": {}
          }}
        }}]
      }}
    }}
  }}
}})",
    type_name("Point"), type_name("I32"), type_name("I32"), type_name("Point"), var_name("self")
);

}  // namespace

TEST_CASE("Parse UFCS function with self parameter", "[parser][ufcs][function]") {
  auto const [input, expected_str] = GENERATE(
      table<std::string, std::string>({
          {k_self_only_input, k_self_only_expected},
          {k_self_with_params_input, k_self_with_params_expected},
      })
  );

  check_parse({.name = "", .input = input, .expected = expected_str, .should_succeed = true, .rest = ""});
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
