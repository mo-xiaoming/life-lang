#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Tuple_Type;

PARSE_TEST(Tuple_Type, tuple_type)

namespace {

// Simple tuple types
constexpr auto k_simple_pair_should_succeed = true;
constexpr auto k_simple_pair_input = "(I32, String)";
inline auto const k_simple_pair_expected = R"({
  "Tuple_Type": {
    "element_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "value": "I32",
              "type_params": []
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "value": "String",
              "type_params": []
            }
          ]
        }
      }
    ]
  }
})";

constexpr auto k_triple_should_succeed = true;
constexpr auto k_triple_input = "(I32, I32, I32)";
inline auto const k_triple_expected = R"({
  "Tuple_Type": {
    "element_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "value": "I32",
              "type_params": []
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "value": "I32",
              "type_params": []
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "value": "I32",
              "type_params": []
            }
          ]
        }
      }
    ]
  }
})";

// Tuple with generic types
constexpr auto k_generic_tuple_should_succeed = true;
constexpr auto k_generic_tuple_input = "(Vec<I32>, Map<String, I32>)";
inline auto const k_generic_tuple_expected = R"({
  "Tuple_Type": {
    "element_types": [
      {
        "Path_Type": {
          "segments": [
            {
              "value": "Vec",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "value": "I32",
                        "type_params": []
                      }
                    ]
                  }
                }
              ]
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "value": "Map",
              "type_params": [
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "value": "String",
                        "type_params": []
                      }
                    ]
                  }
                },
                {
                  "Path_Type": {
                    "segments": [
                      {
                        "value": "I32",
                        "type_params": []
                      }
                    ]
                  }
                }
              ]
            }
          ]
        }
      }
    ]
  }
})";

// Nested tuples
constexpr auto k_nested_tuple_should_succeed = true;
constexpr auto k_nested_tuple_input = "((I32, I32), String)";
inline auto const k_nested_tuple_expected = R"({
  "Tuple_Type": {
    "element_types": [
      {
        "Tuple_Type": {
          "element_types": [
            {
              "Path_Type": {
                "segments": [
                  {
                    "value": "I32",
                    "type_params": []
                  }
                ]
              }
            },
            {
              "Path_Type": {
                "segments": [
                  {
                    "value": "I32",
                    "type_params": []
                  }
                ]
              }
            }
          ]
        }
      },
      {
        "Path_Type": {
          "segments": [
            {
              "value": "String",
              "type_params": []
            }
          ]
        }
      }
    ]
  }
})";

}  // namespace

TEST_CASE("Parse Tuple_Type", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Tuple_Type_Params>({
          {"simple pair", k_simple_pair_input, k_simple_pair_expected, k_simple_pair_should_succeed},
          {"triple", k_triple_input, k_triple_expected, k_triple_should_succeed},
          {"generic tuple", k_generic_tuple_input, k_generic_tuple_expected, k_generic_tuple_should_succeed},
          {"nested tuple", k_nested_tuple_input, k_nested_tuple_expected, k_nested_tuple_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
