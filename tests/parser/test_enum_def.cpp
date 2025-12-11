#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Enum_Def;

PARSE_TEST(Enum_Def, enum_def)

namespace {
// Unit variants
constexpr auto k_unit_single_should_succeed = true;
constexpr auto k_unit_single_input = "enum Color { Red }";
inline auto const k_unit_single_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Color",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Red",
          "kind": "unit"
        }}
      }}
    ]
  }}
}})"
);

constexpr auto k_unit_multiple_should_succeed = true;
constexpr auto k_unit_multiple_input = "enum Color { Red, Green, Blue }";
inline auto const k_unit_multiple_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Color",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Red",
          "kind": "unit"
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Green",
          "kind": "unit"
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Blue",
          "kind": "unit"
        }}
      }}
    ]
  }}
}})"
);

constexpr auto k_unit_trailing_comma_should_succeed = true;
constexpr auto k_unit_trailing_comma_input = "enum Status { Idle, Running, }";
inline auto const k_unit_trailing_comma_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Status",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Idle",
          "kind": "unit"
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Running",
          "kind": "unit"
        }}
      }}
    ]
  }}
}})"
);

// Tuple variants
constexpr auto k_tuple_single_field_should_succeed = true;
constexpr auto k_tuple_single_field_input = "enum Option { Some(I32) }";
inline auto const k_tuple_single_field_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Option",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Some",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("I32")
);

constexpr auto k_tuple_multiple_fields_should_succeed = true;
constexpr auto k_tuple_multiple_fields_input = "enum Color { Rgb(I32, I32, I32) }";
inline auto const k_tuple_multiple_fields_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Color",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Rgb",
          "kind": "tuple",
          "fields": [
            {},
            {},
            {}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("I32"),
    test_json::type_name("I32"),
    test_json::type_name("I32")
);

constexpr auto k_tuple_trailing_comma_should_succeed = true;
constexpr auto k_tuple_trailing_comma_input = "enum Data { Point(I32, I32,) }";
inline auto const k_tuple_trailing_comma_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Data",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Point",
          "kind": "tuple",
          "fields": [
            {},
            {}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("I32"),
    test_json::type_name("I32")
);

constexpr auto k_struct_single_field_should_succeed = true;
constexpr auto k_struct_single_field_input = "enum Message { Write { text: String } }";
inline auto const k_struct_single_field_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Message",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Write",
          "kind": "struct",
          "fields": [
            {{
              "Struct_Field": {{
                "name": "text",
                "type": {}
              }}
            }}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("String")
);

constexpr auto k_struct_multiple_fields_should_succeed = true;
constexpr auto k_struct_multiple_fields_input = "enum Message { Move { x: I32, y: I32 } }";
inline auto const k_struct_multiple_fields_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Message",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Move",
          "kind": "struct",
          "fields": [
            {{
              "Struct_Field": {{
                "name": "x",
                "type": {}
              }}
            }},
            {{
              "Struct_Field": {{
                "name": "y",
                "type": {}
              }}
            }}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("I32"),
    test_json::type_name("I32")
);

// Mixed variants
constexpr auto k_mixed_variants_should_succeed = true;
constexpr auto k_mixed_variants_input = "enum Message { Quit, Move { x: I32, y: I32 }, Write(String) }";
inline auto const k_mixed_variants_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Message",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Quit",
          "kind": "unit"
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Move",
          "kind": "struct",
          "fields": [
            {{
              "Struct_Field": {{
                "name": "x",
                "type": {}
              }}
            }},
            {{
              "Struct_Field": {{
                "name": "y",
                "type": {}
              }}
            }}
          ]
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Write",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("I32"),
    test_json::type_name("I32"),
    test_json::type_name("String")
);

// Generic enums
constexpr auto k_generic_single_param_should_succeed = true;
constexpr auto k_generic_single_param_input = "enum Option<T> { Some(T), None }";
inline auto const k_generic_single_param_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Option",
    "type_params": [
      {}
    ],
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Some",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "None",
          "kind": "unit"
        }}
      }}
    ]
  }}
}})",
    test_json::type_param(test_json::type_name("T")),
    test_json::type_name("T")
);

constexpr auto k_generic_multiple_params_should_succeed = true;
constexpr auto k_generic_multiple_params_input = "enum Result<T, E> { Ok(T), Err(E) }";
inline auto const k_generic_multiple_params_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Result",
    "type_params": [
      {},
      {}
    ],
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Ok",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Err",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_param(test_json::type_name("T")),
    test_json::type_param(test_json::type_name("E")),
    test_json::type_name("T"),
    test_json::type_name("E")
);

// Complex nested types
constexpr auto k_nested_types_should_succeed = true;
constexpr auto k_nested_types_input = "enum Tree<T> { Leaf(T), Node(Tree<T>, Tree<T>) }";
inline auto const k_nested_types_expected = R"({
  "Enum_Def": {
    "name": "Tree",
    "type_params": [
      {"Type_Param": {"name": {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}}}
    ],
    "variants": [
      {
        "Enum_Variant": {
          "name": "Leaf",
          "kind": "tuple",
          "fields": [
            {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}
          ]
        }
      },
      {
        "Enum_Variant": {
          "name": "Node",
          "kind": "tuple",
          "fields": [
            {
              "Type_Name": {
                "segments": [
                  {
                    "Type_Name_Segment": {
                      "type_params": [
                        {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}
                      ],
                      "value": "Tree"
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
                      "type_params": [
                        {"Type_Name": {"segments": [{"Type_Name_Segment": {"type_params": [], "value": "T"}}]}}
                      ],
                      "value": "Tree"
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
})";

// Qualified types in variants
constexpr auto k_qualified_types_should_succeed = true;
constexpr auto k_qualified_types_input = "enum Value { Int(Std.I32), Str(Std.String) }";
inline auto const k_qualified_types_expected = fmt::format(
    R"({{
  "Enum_Def": {{
    "name": "Value",
    "variants": [
      {{
        "Enum_Variant": {{
          "name": "Int",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }},
      {{
        "Enum_Variant": {{
          "name": "Str",
          "kind": "tuple",
          "fields": [
            {}
          ]
        }}
      }}
    ]
  }}
}})",
    test_json::type_name("Std", "I32"),
    test_json::type_name("Std", "String")
);

// Error cases (semantic errors, not parse errors - empty enums would be caught in semantic analysis)
constexpr auto k_empty_variants_should_succeed = true;
constexpr auto k_empty_variants_input = "enum Empty { }";
inline auto const k_empty_variants_expected = R"({
  "Enum_Def": {
    "name": "Empty",
    "variants": []
  }
})";

constexpr auto k_missing_brace_should_succeed = false;
constexpr auto k_missing_brace_input = "enum Color { Red";
inline auto const k_missing_brace_expected = "";

constexpr auto k_missing_name_should_succeed = false;
constexpr auto k_missing_name_input = "enum { Red, Blue }";
inline auto const k_missing_name_expected = "";

}  // namespace

TEST_CASE("Parse Enum_Def", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Enum_Def_Params>({
          {"unit single", k_unit_single_input, k_unit_single_expected, k_unit_single_should_succeed},
          {"unit multiple", k_unit_multiple_input, k_unit_multiple_expected, k_unit_multiple_should_succeed},
          {"unit trailing comma",
           k_unit_trailing_comma_input,
           k_unit_trailing_comma_expected,
           k_unit_trailing_comma_should_succeed},
          {"tuple single field",
           k_tuple_single_field_input,
           k_tuple_single_field_expected,
           k_tuple_single_field_should_succeed},
          {"tuple multiple fields",
           k_tuple_multiple_fields_input,
           k_tuple_multiple_fields_expected,
           k_tuple_multiple_fields_should_succeed},
          {"tuple trailing comma",
           k_tuple_trailing_comma_input,
           k_tuple_trailing_comma_expected,
           k_tuple_trailing_comma_should_succeed},
          {"struct single field",
           k_struct_single_field_input,
           k_struct_single_field_expected,
           k_struct_single_field_should_succeed},
          {"struct multiple fields",
           k_struct_multiple_fields_input,
           k_struct_multiple_fields_expected,
           k_struct_multiple_fields_should_succeed},
          {"mixed variants", k_mixed_variants_input, k_mixed_variants_expected, k_mixed_variants_should_succeed},
          {"generic single param",
           k_generic_single_param_input,
           k_generic_single_param_expected,
           k_generic_single_param_should_succeed},
          {"generic multiple params",
           k_generic_multiple_params_input,
           k_generic_multiple_params_expected,
           k_generic_multiple_params_should_succeed},
          {"nested types", k_nested_types_input, k_nested_types_expected, k_nested_types_should_succeed},
          {"qualified types", k_qualified_types_input, k_qualified_types_expected, k_qualified_types_should_succeed},
          {"empty variants error", k_empty_variants_input, k_empty_variants_expected, k_empty_variants_should_succeed},
          {"missing brace error", k_missing_brace_input, k_missing_brace_expected, k_missing_brace_should_succeed},
          {"missing name error", k_missing_name_input, k_missing_name_expected, k_missing_name_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
