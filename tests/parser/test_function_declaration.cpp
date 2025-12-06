#include "utils.hpp"

using life_lang::ast::Function_Declaration;
using test_json::type_name;

PARSE_TEST(Function_Declaration, function_declaration)

namespace {
// Basic declarations - no parameters
constexpr auto k_no_parameters_input = "fn foo(): Int";
inline auto const k_no_parameters_expected = fmt::format(
    R"({{
  "Function_Declaration": {{
    "name": "foo",
    "parameters": [],
    "returnType": {}
  }}
}})",
    type_name("Int")
);

// One parameter
constexpr auto k_one_parameter_input = "fn foo(x: Int): Int";
inline auto const k_one_parameter_expected = fmt::format(
    R"({{
  "Function_Declaration": {{
    "name": "foo",
    "parameters": [
      {{
        "Function_Parameter": {{
          "is_mut": false,
          "name": "x",
          "type": {}
        }}
      }}
    ],
    "returnType": {}
  }}
}})",
    type_name("Int"), type_name("Int")
);

// Two parameters
constexpr auto k_two_parameters_input = "fn foo(hello: T, world: U): Int";
inline auto const k_two_parameters_expected = fmt::format(
    R"({{
  "Function_Declaration": {{
    "name": "foo",
    "parameters": [
      {{
        "Function_Parameter": {{
          "is_mut": false,
          "name": "hello",
          "type": {}
        }}
      }},
      {{
        "Function_Parameter": {{
          "is_mut": false,
          "name": "world",
          "type": {}
        }}
      }}
    ],
    "returnType": {}
  }}
}})",
    type_name("T"), type_name("U"), type_name("Int")
);

// Namespace qualified return type
constexpr auto k_namespace_return_type_input = "fn foo(): Std.String";
constexpr auto k_namespace_return_type_expected = R"({
  "Function_Declaration": {
    "name": "foo",
    "parameters": [],
    "returnType": {
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

// Template return type
constexpr auto k_template_return_type_input = "fn foo(): Vec<Int>";
constexpr auto k_template_return_type_expected = R"({
  "Function_Declaration": {
    "name": "foo",
    "parameters": [],
    "returnType": {
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
constexpr auto k_complex_templates_input = "fn foo(hello: A.B.Hello<Std.Array, B.C<Int, Double>>): A.B.C<Int>";
constexpr auto k_complex_templates_expected = R"({
  "Function_Declaration": {
    "name": "foo",
    "parameters": [
      {
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
      }
    ],
    "returnType": {
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
                }
              ]
            }
          }
        ]
      }
    }
  }
})";

// Whitespace handling
constexpr auto k_with_spaces_input = "fn  foo  (  x  :  Int  )  :  Int";
inline auto const k_with_spaces_expected = fmt::format(
    R"({{
  "Function_Declaration": {{
    "name": "foo",
    "parameters": [
      {{
        "Function_Parameter": {{
          "is_mut": false,
          "name": "x",
          "type": {}
        }}
      }}
    ],
    "returnType": {}
  }}
}})",
    type_name("Int"), type_name("Int")
);

// Trailing content
constexpr auto k_trailing_body_input = "fn foo(): Int {";
inline auto const k_trailing_body_expected = fmt::format(
    R"({{
  "Function_Declaration": {{
    "name": "foo",
    "parameters": [],
    "returnType": {}
  }}
}})",
    type_name("Int")
);

// Invalid cases
constexpr auto k_invalid_no_fn_keyword_input = "foo(): Int";
constexpr auto k_invalid_no_return_type_input = "fn foo()";
constexpr auto k_invalid_no_parentheses_input = "fn foo: Int";
constexpr auto k_invalid_expected = R"({
  "Function_Declaration": {
    "name": "",
    "parameters": [],
    "returnType": {
      "Type_Name": {
        "segments": []
      }
    }
  }
})";
}  // namespace

TEST_CASE("Parse Function_Declaration", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Function_Declaration_Params>({
          {"no parameters", k_no_parameters_input, k_no_parameters_expected, true, ""},
          {"one parameter", k_one_parameter_input, k_one_parameter_expected, true, ""},
          {"two parameters", k_two_parameters_input, k_two_parameters_expected, true, ""},
          {"namespace return type", k_namespace_return_type_input, k_namespace_return_type_expected, true, ""},
          {"template return type", k_template_return_type_input, k_template_return_type_expected, true, ""},
          {"complex templates", k_complex_templates_input, k_complex_templates_expected, true, ""},
          {"with spaces", k_with_spaces_input, k_with_spaces_expected, true, ""},
          {"trailing body", k_trailing_body_input, k_trailing_body_expected, true, "{"},
          {"invalid - no fn keyword", k_invalid_no_fn_keyword_input, k_invalid_expected, false, "foo(): Int"},
          {"invalid - no return type", k_invalid_no_return_type_input, k_invalid_expected, false, ""},
          {"invalid - no parentheses", k_invalid_no_parentheses_input, k_invalid_expected, false, ": Int"},
      })
  );

  DYNAMIC_SECTION(params.name) { check_parse(params); }
}