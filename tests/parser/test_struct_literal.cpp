#include "utils.hpp"

using life_lang::ast::Struct_Literal;

PARSE_TEST(Struct_Literal, struct_literal)

TEST_CASE("Parse Struct_Literal", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Struct_Literal_Params>({
          // Valid cases - empty struct
          {"empty", "Point { }", R"({"Struct_Literal": {"typeName": "Point", "fields": []}})", true, ""},
          {"empty with spaces", "Point {  }", R"({"Struct_Literal": {"typeName": "Point", "fields": []}})", true, ""},

          // Valid cases - single field
          {"single field int", "Point { x: 42 }",
           R"({"Struct_Literal": {"typeName": "Point", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "42"}}}}]}})",
           true, ""},
          {"single field string", "Name { value: \"test\" }",
           R"({"Struct_Literal": {"typeName": "Name", "fields": [{"Field_Initializer": {"name": "value", "value": {"String": {"value": "\"test\""}}}}]}})",
           true, ""},

          // Valid cases - multiple fields
          {"two fields", "Point { x: 1, y: 2 }",
           R"({"Struct_Literal": {"typeName": "Point", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "1"}}}}, {"Field_Initializer": {"name": "y", "value": {"Integer": {"value": "2"}}}}]}})",
           true, ""},
          {"three fields", "Vec3 { x: 1, y: 2, z: 3 }",
           R"({"Struct_Literal": {"typeName": "Vec3", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "1"}}}}, {"Field_Initializer": {"name": "y", "value": {"Integer": {"value": "2"}}}}, {"Field_Initializer": {"name": "z", "value": {"Integer": {"value": "3"}}}}]}})",
           true, ""},

          // Valid cases - trailing comma
          {"trailing comma single", "Point { x: 42, }",
           R"({"Struct_Literal": {"typeName": "Point", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "42"}}}}]}})",
           true, ""},
          {"trailing comma multiple", "Point { x: 1, y: 2, }",
           R"({"Struct_Literal": {"typeName": "Point", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "1"}}}}, {"Field_Initializer": {"name": "y", "value": {"Integer": {"value": "2"}}}}]}})",
           true, ""},

          // Valid cases - with trailing content
          {"with trailing content", "Point { x: 1 } other",
           R"({"Struct_Literal": {"typeName": "Point", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "1"}}}}]}})",
           true, "other"},

          // Valid cases - field access in initializer value
          {"field access in value", "Obj { value: some.path }",
           R"({"Struct_Literal": {"typeName": "Obj", "fields": [{"Field_Initializer": {"name": "value", "value": {"Field_Access_Expr": {"fieldName": "path", "object": {"Variable_Name": {"segments": [{"Variable_Name_Segment": {"templateParameters": [], "value": "some"}}]}}}}}}]}})",
           true, ""},

          // Parser accepts any identifier - naming conventions checked at semantic analysis
          {"lowercase type name accepted", "point { x: 1 }",
           R"({"Struct_Literal": {"typeName": "point", "fields": [{"Field_Initializer": {"name": "x", "value": {"Integer": {"value": "1"}}}}]}})",
           true, ""},

          // Invalid cases
          {"invalid - missing open brace", "Point x: 1 }", R"({"Struct_Literal": {"typeName": "", "fields": []}})",
           false, "Point x: 1 }"},
          {"invalid - missing close brace", "Point { x: 1", R"({"Struct_Literal": {"typeName": "", "fields": []}})",
           false, ""},
          {"invalid - missing colon", "Point { x 1 }", R"({"Struct_Literal": {"typeName": "", "fields": []}})", false,
           "x 1 }"},
          {"invalid - missing value", "Point { x: }", R"({"Struct_Literal": {"typeName": "", "fields": []}})", false,
           "x: }"},
          {"invalid - missing comma", "Point { x: 1 y: 2 }", R"({"Struct_Literal": {"typeName": "", "fields": []}})",
           false, "y: 2 }"},
          {"invalid - empty", "", R"({"Struct_Literal": {"typeName": "", "fields": []}})", false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
