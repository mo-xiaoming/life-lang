#include "internal_rules.hpp"
#include "utils.hpp"
using life_lang::ast::Expr;

PARSE_TEST(Expr, expr)

namespace {
// === Valid For Loop Expressions ===

// Simple range iteration (exclusive)
constexpr auto k_simple_range_exclusive_should_succeed = true;
constexpr auto k_simple_range_exclusive_input = "for i in 0..10 { process(i); }";
inline auto const k_simple_range_exclusive_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "i" }} }},
    "iterator": {{
      "Range_Expr": {{
        "start": {{ "Integer": {{ "value": "0" }} }},
        "end": {{ "Integer": {{ "value": "10" }} }},
        "inclusive": false
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("process"), test_json::var_name("i")
);

// Simple range iteration (inclusive)
constexpr auto k_simple_range_inclusive_should_succeed = true;
constexpr auto k_simple_range_inclusive_input = "for i in 0..=10 { process(i); }";
inline auto const k_simple_range_inclusive_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "i" }} }},
    "iterator": {{
      "Range_Expr": {{
        "start": {{ "Integer": {{ "value": "0" }} }},
        "end": {{ "Integer": {{ "value": "10" }} }},
        "inclusive": true
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("process"), test_json::var_name("i")
);

// Variable range
constexpr auto k_variable_range_should_succeed = true;
constexpr auto k_variable_range_input = "for item in start..end { work(item); }";
inline auto const k_variable_range_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "item" }} }},
    "iterator": {{
      "Range_Expr": {{
        "start": {},
        "end": {},
        "inclusive": false
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("start"), test_json::var_name("end"), test_json::var_name("work"), test_json::var_name("item")
);

// Collection variable iteration
constexpr auto k_collection_iteration_should_succeed = true;
constexpr auto k_collection_iteration_input = "for user in users { handle(user); }";
inline auto const k_collection_iteration_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "user" }} }},
    "iterator": {},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("users"), test_json::var_name("handle"), test_json::var_name("user")
);

// Empty body
constexpr auto k_empty_body_should_succeed = true;
constexpr auto k_empty_body_input = "for x in 0..10 {}";
inline auto const k_empty_body_expected = R"({
  "For_Expr": {
    "pattern": { "Simple_Pattern": { "name": "x" } },
    "iterator": {
      "Range_Expr": {
        "start": { "Integer": { "value": "0" } },
        "end": { "Integer": { "value": "10" } },
        "inclusive": false
      }
    },
    "body": {
      "Block": {
        "statements": []
      }
    }
  }
})";

// Nested for loops
constexpr auto k_nested_for_loops_should_succeed = true;
constexpr auto k_nested_for_loops_input = "for i in 0..3 { for j in 0..3 { process(i, j); } }";
inline auto const k_nested_for_loops_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "i" }} }},
    "iterator": {{
      "Range_Expr": {{
        "start": {{ "Integer": {{ "value": "0" }} }},
        "end": {{ "Integer": {{ "value": "3" }} }},
        "inclusive": false
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "For_Statement": {{
              "expr": {{
                "For_Expr": {{
                  "pattern": {{ "Simple_Pattern": {{ "name": "j" }} }},
                  "iterator": {{
                    "Range_Expr": {{
                      "start": {{ "Integer": {{ "value": "0" }} }},
                      "end": {{ "Integer": {{ "value": "3" }} }},
                      "inclusive": false
                    }}
                  }},
                  "body": {{
                    "Block": {{
                      "statements": [
                        {{
                          "Function_Call_Statement": {{
                            "expr": {{
                              "Function_Call_Expr": {{
                                "name": {},
                                "parameters": [{}, {}]
                              }}
                            }}
                          }}
                        }}
                      ]
                    }}
                  }}
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("process"), test_json::var_name("i"), test_json::var_name("j")
);

// For loop with multiple statements
constexpr auto k_multiple_statements_should_succeed = true;
constexpr auto k_multiple_statements_input = "for x in 0..5 { print(x); log(x); }";
inline auto const k_multiple_statements_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "x" }} }},
    "iterator": {{
      "Range_Expr": {{
        "start": {{ "Integer": {{ "value": "0" }} }},
        "end": {{ "Integer": {{ "value": "5" }} }},
        "inclusive": false
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }},
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("print"), test_json::var_name("x"), test_json::var_name("log"), test_json::var_name("x")
);

// For with function call as iterator
constexpr auto k_function_call_iterator_should_succeed = true;
constexpr auto k_function_call_iterator_input = "for item in get_items() { process(item); }";
inline auto const k_function_call_iterator_expected = fmt::format(
    R"({{
  "For_Expr": {{
    "pattern": {{ "Simple_Pattern": {{ "name": "item" }} }},
    "iterator": {{
      "Function_Call_Expr": {{
        "name": {},
        "parameters": []
      }}
    }},
    "body": {{
      "Block": {{
        "statements": [
          {{
            "Function_Call_Statement": {{
              "expr": {{
                "Function_Call_Expr": {{
                  "name": {},
                  "parameters": [{}]
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    test_json::var_name("get_items"), test_json::var_name("process"), test_json::var_name("item")
);

// For with spaces
constexpr auto k_with_spaces_should_succeed = true;
constexpr auto k_with_spaces_input = "for   x   in   0..10   {   work();   }";
inline auto const k_with_spaces_expected = R"({
  "For_Expr": {
    "pattern": { "Simple_Pattern": { "name": "x" } },
    "iterator": {
      "Range_Expr": {
        "start": { "Integer": { "value": "0" } },
        "end": { "Integer": { "value": "10" } },
        "inclusive": false
      }
    },
    "body": {
      "Block": {
        "statements": [
          {
            "Function_Call_Statement": {
              "expr": {
                "Function_Call_Expr": {
                  "name": {
                    "Variable_Name": {
                      "segments": [
                        {
                          "Variable_Name_Segment": {
                            "template_parameters": [],
                            "value": "work"
                          }
                        }
                      ]
                    }
                  },
                  "parameters": []
                }
              }
            }
          }
        ]
      }
    }
  }
})";

// === Invalid For Loop Expressions ===

// Missing 'in' keyword
constexpr auto k_missing_in_should_succeed = false;
constexpr auto k_missing_in_input = "for x 0..10 { work(); }";

// Missing binding variable
constexpr auto k_missing_binding_should_succeed = false;
constexpr auto k_missing_binding_input = "for in 0..10 { work(); }";

// Missing iterator expression
constexpr auto k_missing_iterator_should_succeed = false;
constexpr auto k_missing_iterator_input = "for x in { work(); }";

// Missing body
constexpr auto k_missing_body_should_succeed = false;
constexpr auto k_missing_body_input = "for x in 0..10";

// Missing opening brace
constexpr auto k_missing_open_brace_should_succeed = false;
constexpr auto k_missing_open_brace_input = "for x in 0..10 work(); }";

// Reserved keyword as binding
constexpr auto k_reserved_keyword_binding_should_succeed = false;
constexpr auto k_reserved_keyword_binding_input = "for if in 0..10 { work(); }";

}  // namespace

TEST_CASE("Parse For_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          // Valid cases
          {"simple range exclusive", k_simple_range_exclusive_input, k_simple_range_exclusive_expected,
           k_simple_range_exclusive_should_succeed},
          {"simple range inclusive", k_simple_range_inclusive_input, k_simple_range_inclusive_expected,
           k_simple_range_inclusive_should_succeed},
          {"variable range", k_variable_range_input, k_variable_range_expected, k_variable_range_should_succeed},
          {"collection iteration", k_collection_iteration_input, k_collection_iteration_expected,
           k_collection_iteration_should_succeed},
          {"empty body", k_empty_body_input, k_empty_body_expected, k_empty_body_should_succeed},
          {"nested for loops", k_nested_for_loops_input, k_nested_for_loops_expected,
           k_nested_for_loops_should_succeed},
          {"multiple statements", k_multiple_statements_input, k_multiple_statements_expected,
           k_multiple_statements_should_succeed},
          {"function call iterator", k_function_call_iterator_input, k_function_call_iterator_expected,
           k_function_call_iterator_should_succeed},
          {"with spaces", k_with_spaces_input, k_with_spaces_expected, k_with_spaces_should_succeed},

          // Invalid cases
          {"invalid - missing in", k_missing_in_input, "", k_missing_in_should_succeed},
          {"invalid - missing binding", k_missing_binding_input, "", k_missing_binding_should_succeed},
          {"invalid - missing iterator", k_missing_iterator_input, "", k_missing_iterator_should_succeed},
          {"invalid - missing body", k_missing_body_input, "", k_missing_body_should_succeed},
          {"invalid - missing open brace", k_missing_open_brace_input, "", k_missing_open_brace_should_succeed},
          {"invalid - reserved keyword binding", k_reserved_keyword_binding_input, "",
           k_reserved_keyword_binding_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) { check_parse(params); }
}
