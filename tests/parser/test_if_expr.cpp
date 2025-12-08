#include "internal_rules.hpp"
#include "utils.hpp"

using life_lang::ast::Expr;
using test_json::var_name;

PARSE_TEST(Expr, expr)

namespace {
// Basic if without else
constexpr auto k_if_only_should_succeed = true;
constexpr auto k_if_only_input = "if x { return 1; }";
inline auto const k_if_only_expected = test_json::if_expr(
    test_json::var_name("x"),
    test_json::block({test_json::return_statement(test_json::integer(1))})
);

// If with else
constexpr auto k_if_else_should_succeed = true;
constexpr auto k_if_else_input = "if condition { return 1; } else { return 2; }";
inline auto const k_if_else_expected = test_json::if_else_expr(
    test_json::var_name("condition"),
    test_json::block({test_json::return_statement(test_json::integer(1))}),
    test_json::block({test_json::return_statement(test_json::integer(2))})
);

// If with single else-if
constexpr auto k_if_elseif_should_succeed = true;
constexpr auto k_if_elseif_input = "if a { return 1; } else if b { return 2; }";
inline auto const k_if_elseif_expected = fmt::format(
    R"({{
  "If_Expr": {{
    "condition": {},
    "then_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "1"
                }}
              }}
            }}
          }}
        ]
      }}
    }},
    "else_ifs": [
      {{
        "condition": {},
        "then_block": {{
          "Block": {{
            "statements": [
              {{
                "Return_Statement": {{
                  "expr": {{
                    "Integer": {{
                      "value": "2"
                    }}
                  }}
                }}
              }}
            ]
          }}
        }}
      }}
    ]
  }}
}})",
    var_name("a"),
    var_name("b")
);

// If with else-if and final else
constexpr auto k_if_elseif_else_should_succeed = true;
constexpr auto k_if_elseif_else_input = "if a { return 1; } else if b { return 2; } else { return 3; }";
inline auto const k_if_elseif_else_expected = fmt::format(
    R"({{
  "If_Expr": {{
    "condition": {},
    "then_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "1"
                }}
              }}
            }}
          }}
        ]
      }}
    }},
    "else_ifs": [
      {{
        "condition": {},
        "then_block": {{
          "Block": {{
            "statements": [
              {{
                "Return_Statement": {{
                  "expr": {{
                    "Integer": {{
                      "value": "2"
                    }}
                  }}
                }}
              }}
            ]
          }}
        }}
      }}
    ],
    "else_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "3"
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("a"),
    var_name("b")
);

// If with multiple else-if clauses
constexpr auto k_multiple_elseif_should_succeed = true;
constexpr auto k_multiple_elseif_input =
    "if a { return 1; } else if b { return 2; } else if c { return 3; } else { return 4; }";
inline auto const k_multiple_elseif_expected = fmt::format(
    R"({{
  "If_Expr": {{
    "condition": {},
    "then_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "1"
                }}
              }}
            }}
          }}
        ]
      }}
    }},
    "else_ifs": [
      {{
        "condition": {},
        "then_block": {{
          "Block": {{
            "statements": [
              {{
                "Return_Statement": {{
                  "expr": {{
                    "Integer": {{
                      "value": "2"
                    }}
                  }}
                }}
              }}
            ]
          }}
        }}
      }},
      {{
        "condition": {},
        "then_block": {{
          "Block": {{
            "statements": [
              {{
                "Return_Statement": {{
                  "expr": {{
                    "Integer": {{
                      "value": "3"
                    }}
                  }}
                }}
              }}
            ]
          }}
        }}
      }}
    ],
    "else_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {{
                "Integer": {{
                  "value": "4"
                }}
              }}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("a"),
    var_name("b"),
    var_name("c")
);

// If expression with binary operators
constexpr auto k_if_with_comparison_should_succeed = true;
constexpr auto k_if_with_comparison_input = "if x > y { return x; } else { return y; }";
inline auto const k_if_with_comparison_expected = fmt::format(
    R"({{
  "If_Expr": {{
    "condition": {{
      "Binary_Expr": {{
        "lhs": {},
        "op": ">",
        "rhs": {}
      }}
    }},
    "then_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {}
            }}
          }}
        ]
      }}
    }},
    "else_block": {{
      "Block": {{
        "statements": [
          {{
            "Return_Statement": {{
              "expr": {}
            }}
          }}
        ]
      }}
    }}
  }}
}})",
    var_name("x"),
    var_name("y"),
    var_name("x"),
    var_name("y")
);

// Empty blocks
constexpr auto k_if_empty_blocks_should_succeed = true;
constexpr auto k_if_empty_blocks_input = "if x {} else if y {} else {}";
inline auto const k_if_empty_blocks_expected = fmt::format(
    R"({{
  "If_Expr": {{
    "condition": {},
    "then_block": {{
      "Block": {{
        "statements": []
      }}
    }},
    "else_ifs": [
      {{
        "condition": {},
        "then_block": {{
          "Block": {{
            "statements": []
          }}
        }}
      }}
    ],
    "else_block": {{
      "Block": {{
        "statements": []
      }}
    }}
  }}
}})",
    var_name("x"),
    var_name("y")
);

// Invalid: missing condition
constexpr auto k_missing_condition_should_succeed = false;
constexpr auto k_missing_condition_input = "if { return 1; }";
inline auto const k_missing_condition_expected = "";

// Invalid: missing block
constexpr auto k_missing_block_should_succeed = false;
constexpr auto k_missing_block_input = "if x";
inline auto const k_missing_block_expected = "";

// Invalid: missing else-if condition
constexpr auto k_missing_elseif_condition_should_succeed = false;
constexpr auto k_missing_elseif_condition_input = "if x { return 1; } else if { return 2; }";
inline auto const k_missing_elseif_condition_expected = "";

// Invalid: missing else-if block
constexpr auto k_missing_elseif_block_should_succeed = false;
constexpr auto k_missing_elseif_block_input = "if x { return 1; } else if y";
inline auto const k_missing_elseif_block_expected = "";

}  // namespace

TEST_CASE("Parse If_Expr", "[parser]") {
  auto const params = GENERATE(
      Catch::Generators::values<Expr_Params>({
          {"if only", k_if_only_input, k_if_only_expected, k_if_only_should_succeed},
          {"if else", k_if_else_input, k_if_else_expected, k_if_else_should_succeed},
          {"if else-if", k_if_elseif_input, k_if_elseif_expected, k_if_elseif_should_succeed},
          {"if else-if else", k_if_elseif_else_input, k_if_elseif_else_expected, k_if_elseif_else_should_succeed},
          {"multiple else-if", k_multiple_elseif_input, k_multiple_elseif_expected, k_multiple_elseif_should_succeed},
          {"if with comparison",
           k_if_with_comparison_input,
           k_if_with_comparison_expected,
           k_if_with_comparison_should_succeed},
          {"if empty blocks", k_if_empty_blocks_input, k_if_empty_blocks_expected, k_if_empty_blocks_should_succeed},
          {"missing condition",
           k_missing_condition_input,
           k_missing_condition_expected,
           k_missing_condition_should_succeed},
          {"missing block", k_missing_block_input, k_missing_block_expected, k_missing_block_should_succeed},
          {"missing else-if condition",
           k_missing_elseif_condition_input,
           k_missing_elseif_condition_expected,
           k_missing_elseif_condition_should_succeed},
          {"missing else-if block",
           k_missing_elseif_block_input,
           k_missing_elseif_block_expected,
           k_missing_elseif_block_should_succeed},
      })
  );
  DYNAMIC_SECTION(params.name) {
    check_parse(params);
  }
}
