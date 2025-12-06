#include "utils.hpp"

using life_lang::ast::Function_Parameter;
using test_json::type_name;

PARSE_TEST(Function_Parameter, function_parameter)

namespace {
// Simple type
constexpr auto k_simple_type_input = "x: Int";
inline auto const k_simple_type_expected = fmt::format(
    R"({{
  "Function_Parameter": {{
    "is_mut": false,
    "name": "x",
    "type": {}
  }}
}})",
    type_name("Int")
);

// No spaces
constexpr auto k_no_spaces_input = "hello:T";
inline auto const k_no_spaces_expected = fmt::format(
    R"({{
  "Function_Parameter": {{
    "is_mut": false,
    "name": "hello",
    "type": {}
  }}
}})",
    type_name("T")
);

// Qualified type
constexpr auto k_with_namespace_input = "arg: Std.String";
constexpr auto k_with_namespace_expected = R"({
  "Function_Parameter": {
    "is_mut": false,
    "name": "arg",
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Std",
              "templateParameters": []
            }
          },
          {
            "Type_Name_Segment": {
              "value": "String",
              "templateParameters": []
            }
          }
        ]
      }
    }
  }
})";

// Template parameter
constexpr auto k_template_parameter_input = "vec: Vec<Int>";
constexpr auto k_template_parameter_expected = R"({
  "Function_Parameter": {
    "is_mut": false,
    "name": "vec",
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Vec",
              "templateParameters": [
                {
                  "Type_Name": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Int",
                          "templateParameters": []
                        }
                      }
                    ]
                  }
                }
              ]
            }
          }
        ]
      }
    }
  }
})";

// Complex nested templates
constexpr auto k_complex_nested_templates_input = "hello: A.B.Hello<Std.Array, A.B.C<Int, Double>>";
constexpr auto k_complex_nested_templates_expected = R"({
  "Function_Parameter": {
    "is_mut": false,
    "name": "hello",
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "A",
              "templateParameters": []
            }
          },
          {
            "Type_Name_Segment": {
              "value": "B",
              "templateParameters": []
            }
          },
          {
            "Type_Name_Segment": {
              "value": "Hello",
              "templateParameters": [
                {
                  "Type_Name": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Std",
                          "templateParameters": []
                        }
                      },
                      {
                        "Type_Name_Segment": {
                          "value": "Array",
                          "templateParameters": []
                        }
                      }
                    ]
                  }
                },
                {
                  "Type_Name": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "A",
                          "templateParameters": []
                        }
                      },
                      {
                        "Type_Name_Segment": {
                          "value": "B",
                          "templateParameters": []
                        }
                      },
                      {
                        "Type_Name_Segment": {
                          "value": "C",
                          "templateParameters": [
                            {
                              "Type_Name": {
                                "segments": [
                                  {
                                    "Type_Name_Segment": {
                                      "value": "Int",
                                      "templateParameters": []
                                    }
                                  }
                                ]
                              }
                            },
                            {
                              "Type_Name": {
                                "segments": [
                                  {
                                    "Type_Name_Segment": {
                                      "value": "Double",
                                      "templateParameters": []
                                    }
                                  }
                                ]
                              }
                            }
                          ]
                        }
                      }
                    ]
                  }
                }
              ]
            }
          }
        ]
      }
    }
  }
})";

// Trailing comma
constexpr auto k_with_trailing_comma_input = "x: Int,";
inline auto const k_with_trailing_comma_expected = fmt::format(
    R"({{
  "Function_Parameter": {{
    "is_mut": false,
    "name": "x",
    "type": {}
  }}
}})",
    type_name("Int")
);

// Mutable parameter - simple type
constexpr auto k_mut_simple_input = "mut x: Int";
inline auto const k_mut_simple_expected = fmt::format(
    R"({{
  "Function_Parameter": {{
    "is_mut": true,
    "name": "x",
    "type": {}
  }}
}})",
    type_name("Int")
);

// Mutable parameter - self
constexpr auto k_mut_self_input = "mut self: Point";
inline auto const k_mut_self_expected = fmt::format(
    R"({{
  "Function_Parameter": {{
    "is_mut": true,
    "name": "self",
    "type": {}
  }}
}})",
    type_name("Point")
);

// Mutable parameter - qualified type
constexpr auto k_mut_qualified_input = "mut data: Std.Array<Int>";
constexpr auto k_mut_qualified_expected = R"({
  "Function_Parameter": {
    "is_mut": true,
    "name": "data",
    "type": {
      "Type_Name": {
        "segments": [
          {
            "Type_Name_Segment": {
              "value": "Std",
              "templateParameters": []
            }
          },
          {
            "Type_Name_Segment": {
              "value": "Array",
              "templateParameters": [
                {
                  "Type_Name": {
                    "segments": [
                      {
                        "Type_Name_Segment": {
                          "value": "Int",
                          "templateParameters": []
                        }
                      }
                    ]
                  }
                }
              ]
            }
          }
        ]
      }
    }
  }
})";

// Mutable parameter - no space after mut
constexpr auto k_mut_no_space_input = "mutx:Int";
inline auto const k_mut_no_space_expected = fmt::format(
    R"({{
  "Function_Parameter": {{
    "is_mut": false,
    "name": "mutx",
    "type": {}
  }}
}})",
    type_name("Int")
);

// Invalid cases
constexpr auto k_invalid_no_colon_input = "x Int";
constexpr auto k_invalid_no_type_input = "x:";
constexpr auto k_invalid_no_name_input = ": Int";
constexpr auto k_invalid_empty_input = "";
constexpr auto k_invalid_expected = "{}";
}  // namespace

TEST_CASE("Parse Function_Parameter", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Parameter_Params>({
          {"simple type", k_simple_type_input, k_simple_type_expected, true, ""},
          {"no spaces", k_no_spaces_input, k_no_spaces_expected, true, ""},
          {"with namespace", k_with_namespace_input, k_with_namespace_expected, true, ""},
          {"template parameter", k_template_parameter_input, k_template_parameter_expected, true, ""},
          {"complex nested templates", k_complex_nested_templates_input, k_complex_nested_templates_expected, true, ""},
          {"with trailing comma", k_with_trailing_comma_input, k_with_trailing_comma_expected, true, ","},
          {"mut simple", k_mut_simple_input, k_mut_simple_expected, true, ""},
          {"mut self", k_mut_self_input, k_mut_self_expected, true, ""},
          {"mut qualified type", k_mut_qualified_input, k_mut_qualified_expected, true, ""},
          {"mut no space (mutx is identifier)", k_mut_no_space_input, k_mut_no_space_expected, true, ""},
          {"invalid - no colon", k_invalid_no_colon_input, k_invalid_expected, false, "Int"},
          {"invalid - no type", k_invalid_no_type_input, k_invalid_expected, false, ""},
          {"invalid - no name", k_invalid_no_name_input, k_invalid_expected, false, ": Int"},
          {"invalid - empty", k_invalid_empty_input, k_invalid_expected, false, ""},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}