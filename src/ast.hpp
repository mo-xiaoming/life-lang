#ifndef AST_HPP__
#define AST_HPP__

#include <boost/optional.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <concepts>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace life_lang::ast {

// ============================================================================
// Forward Declarations
// ============================================================================

struct Type_Name_Segment;
struct Type_Name;
struct Variable_Name_Segment;
struct Function_Call_Expr;
struct Field_Access_Expr;
struct Binary_Expr;
struct If_Expr;
struct While_Expr;
struct For_Expr;
struct Range_Expr;
struct Function_Definition;
struct Block;
struct Struct_Definition;
struct Expr;

// ============================================================================
// Type Name System (for type annotations)
// ============================================================================

// Example: Std.Map<Std.String, Vec<I32>>
struct Type_Name : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Type_Name";
  std::vector<Type_Name_Segment> segments;
};

// Example: Map<String, I32> where "Map" is value, template_parameters = [String, I32]
struct Type_Name_Segment : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Type_Name_Segment";
  std::string value;
  std::vector<Type_Name> template_parameters;
};

// ============================================================================
// Variable Name System (for variables and function names)
// ============================================================================

// Example: Std.IO.println (qualified function name) or my_var (simple variable)
struct Variable_Name : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Variable_Name";
  std::vector<Variable_Name_Segment> segments;
};

// Example: println<T> where "println" is value, template_parameters = [T]
struct Variable_Name_Segment : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Variable_Name_Segment";
  std::string value;
  std::vector<Type_Name> template_parameters;
};

// ============================================================================
// Literal Types
// ============================================================================

// Example: "Hello, world!" stored with quotes as "\"Hello, world!\""
struct String : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "String";
  std::string value;
};

// Example: 42 or 0x2A or 0b101010 (stored as string for arbitrary precision)
struct Integer : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Integer";
  std::string value;
};

// ============================================================================
// Struct Literal Types (for initialization)
// ============================================================================

// Example: x: 10 in struct literal Point { x: 10, y: 20 }
struct Field_Initializer : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Field_Initializer";
  std::string name;
  boost::spirit::x3::forward_ast<Expr> value;
};

// Example: Point { x: offset.x + 5, y: base.calculate() }
struct Struct_Literal : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Struct_Literal";
  std::string type_name;
  std::vector<Field_Initializer> fields;
};

// ============================================================================
// Binary Operators
// ============================================================================

// Operator precedence (from lowest to highest):
// Logical: ||
// Logical: &&
// Comparison: ==, !=, <, >, <=, >=
// Additive: +, -
// Multiplicative: *, /, %
// Unary: -, +, !, ~ (highest precedence)
enum class Binary_Op : std::uint8_t {
  // Arithmetic operators
  Add,  // +
  Sub,  // -
  Mul,  // *
  Div,  // /
  Mod,  // %

  // Comparison operators
  Eq,  // ==
  Ne,  // !=
  Lt,  // <
  Gt,  // >
  Le,  // <=
  Ge,  // >=

  // Logical operators
  And,  // &&
  Or,   // ||
};

// Example: x + y, a * (b - c), value == 42
struct Binary_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Binary_Expr";
  boost::spirit::x3::forward_ast<Expr> lhs;
  Binary_Op op;
  boost::spirit::x3::forward_ast<Expr> rhs;
};

// Unary operators (higher precedence than binary)
enum class Unary_Op : std::uint8_t {
  Neg,     // - (arithmetic negation)
  Pos,     // + (arithmetic positive/identity)
  Not,     // ! (logical NOT)
  BitNot,  // ~ (bitwise NOT)
};

// Example: -x, !flag, ~bits
struct Unary_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Unary_Expr";
  Unary_Op op;
  boost::spirit::x3::forward_ast<Expr> operand;
};

// Range expression: start..end (exclusive) or start..=end (inclusive)
// Examples: 0..10, start..end, 1..=100
struct Range_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Range_Expr";
  boost::spirit::x3::forward_ast<Expr> start;
  boost::spirit::x3::forward_ast<Expr> end;
  bool inclusive;  // false for .., true for ..=
};

// ============================================================================
// Expression Types
// ============================================================================

// Example: foo.bar.baz() or Point { x: 1 + 2, y: calculate(z) }
struct Expr : boost::spirit::x3::variant<
                  Variable_Name, boost::spirit::x3::forward_ast<Function_Call_Expr>,
                  boost::spirit::x3::forward_ast<Field_Access_Expr>, boost::spirit::x3::forward_ast<Binary_Expr>,
                  boost::spirit::x3::forward_ast<Unary_Expr>, boost::spirit::x3::forward_ast<If_Expr>,
                  boost::spirit::x3::forward_ast<While_Expr>, boost::spirit::x3::forward_ast<For_Expr>,
                  boost::spirit::x3::forward_ast<Range_Expr>, Struct_Literal, String, Integer>,
              boost::spirit::x3::position_tagged {
  using Base_Type = boost::spirit::x3::variant<
      Variable_Name, boost::spirit::x3::forward_ast<Function_Call_Expr>,
      boost::spirit::x3::forward_ast<Field_Access_Expr>, boost::spirit::x3::forward_ast<Binary_Expr>,
      boost::spirit::x3::forward_ast<Unary_Expr>, boost::spirit::x3::forward_ast<If_Expr>,
      boost::spirit::x3::forward_ast<While_Expr>, boost::spirit::x3::forward_ast<For_Expr>,
      boost::spirit::x3::forward_ast<Range_Expr>, Struct_Literal, String, Integer>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: Std.print("Value: ", x, y + 2)
struct Function_Call_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Call_Expr";
  Variable_Name name;
  std::vector<Expr> parameters;
};

// Example: point.x or nested.obj.field (chained via recursive object field)
struct Field_Access_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Field_Access_Expr";
  boost::spirit::x3::forward_ast<Expr> object;
  std::string field_name;
};

// ============================================================================
// Statement Types
// ============================================================================

// Example: Std.print("Hello"); as a standalone statement (not an expression)
struct Function_Call_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Call_Statement";
  Function_Call_Expr expr;
};

// Example: return calculate(x + y, Point { a: 1, b: 2 });
struct Return_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Return_Statement";
  Expr expr;
};

// Example: break; or break result_value;
// Used to exit loops early, optionally returning a value (Phase 2)
struct Break_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Break_Statement";
  boost::optional<Expr> value;  // Optional: break can be used without value
};

// If statement wrapper for using if expressions as statements
// When if is used for side effects (not in expression context), no semicolon needed
// Example: if condition { do_something(); }
struct If_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "If_Statement";
  boost::spirit::x3::forward_ast<If_Expr> expr;
};

// While statement wrapper for using while expressions as statements
// Example: while x < 10 { process(x); }
struct While_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "While_Statement";
  boost::spirit::x3::forward_ast<While_Expr> expr;
};

// For statement wrapper for using for expressions as statements
// Example: for item in 0..10 { process(item); }
struct For_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "For_Statement";
  boost::spirit::x3::forward_ast<For_Expr> expr;
};

// Example: Can be function def, struct def, function call, return, break, if, while, for, or nested block
struct Statement
    : boost::spirit::x3::variant<
          boost::spirit::x3::forward_ast<Function_Definition>, boost::spirit::x3::forward_ast<Struct_Definition>,
          Function_Call_Statement, Return_Statement, Break_Statement, boost::spirit::x3::forward_ast<If_Statement>,
          boost::spirit::x3::forward_ast<While_Statement>, boost::spirit::x3::forward_ast<For_Statement>,
          boost::spirit::x3::forward_ast<Block>>,
      boost::spirit::x3::position_tagged {
  using Base_Type = boost::spirit::x3::variant<
      boost::spirit::x3::forward_ast<Function_Definition>, boost::spirit::x3::forward_ast<Struct_Definition>,
      Function_Call_Statement, Return_Statement, Break_Statement, boost::spirit::x3::forward_ast<If_Statement>,
      boost::spirit::x3::forward_ast<While_Statement>, boost::spirit::x3::forward_ast<For_Statement>,
      boost::spirit::x3::forward_ast<Block>>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: { Std.print(x); { nested(); } return 0; }
struct Block : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Block";
  std::vector<Statement> statements;
};

// Example: if x > 0 { x } else if x < 0 { -x } else { 0 }
// Chain structure: condition + then_block, plus optional else_ifs and final else_block
struct Else_If_Clause : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Else_If_Clause";
  boost::spirit::x3::forward_ast<Expr> condition;
  boost::spirit::x3::forward_ast<Block> then_block;
};

struct If_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "If_Expr";
  boost::spirit::x3::forward_ast<Expr> condition;
  boost::spirit::x3::forward_ast<Block> then_block;
  std::vector<Else_If_Clause> else_ifs;
  // boost::optional to avoid use-after-free issues caused by std::optional
  boost::optional<boost::spirit::x3::forward_ast<Block>> else_block;
};

// Example: while x < 10 { x = x + 1; }
// Loop continues while condition is true
struct While_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "While_Expr";
  boost::spirit::x3::forward_ast<Expr> condition;
  boost::spirit::x3::forward_ast<Block> body;
};

// Example: for item in 0..10 { process(item); }
// Iterates over collection or range, binding each element to variable
struct For_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "For_Expr";
  std::string binding;                            // Variable name for loop item (e.g., "item")
  boost::spirit::x3::forward_ast<Expr> iterator;  // Collection or range expression
  boost::spirit::x3::forward_ast<Block> body;
};

// ============================================================================
// Function Types
// ============================================================================

// Example: items: Std.Array<T> or mut self: Point in function parameter list
struct Function_Parameter : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Parameter";
  bool is_mut;
  std::string name;
  Type_Name type;
};

// Example: fn process(data: Vec<I32>, callback: Fn<I32, Bool>): Result<String>
struct Function_Declaration : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Declaration";
  std::string name;
  std::vector<Function_Parameter> parameters;
  Type_Name return_type;
};

// Example: fn main(args: Std.Array<String>): I32 { Std.print("Hi"); return 0; }
struct Function_Definition : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Definition";
  Function_Declaration declaration;
  Block body;
};

// ============================================================================
// Struct Types
// ============================================================================

// Example: items: Std.Vec<T> in struct definition
struct Struct_Field : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Struct_Field";
  std::string name;
  Type_Name type;
};

// Example: struct Point { x: I32, y: I32, metadata: Option<String> }
struct Struct_Definition : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Struct_Definition";
  std::string name;
  std::vector<Struct_Field> fields;
};

// ============================================================================
// Module Types
// ============================================================================

// Example: Top-level container with struct defs, function defs, and statements
struct Module : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Module";
  std::vector<Statement> statements;
};

// ============================================================================
// Helper Functions for AST Construction
// ============================================================================

// Type_Name helpers
inline Type_Name_Segment make_type_name_segment(std::string&& a_value, std::vector<Type_Name>&& a_template_parameters) {
  return Type_Name_Segment{{}, std::move(a_value), std::move(a_template_parameters)};
}

inline Type_Name_Segment make_type_name_segment(std::string&& a_value) {
  return make_type_name_segment(std::move(a_value), {});
}

inline Type_Name make_type_name(std::vector<Type_Name_Segment>&& a_segments) {
  return Type_Name{{}, std::move(a_segments)};
}

// Variadic make_type_name for convenient construction
// Accepts string arguments and/or Type_Name_Segment arguments
// Examples:
//   make_type_name("Int")                                    -> Type_Name with single segment "Int"
//   make_type_name("Std", "String")                          -> Type_Name with segments ["Std", "String"]
//   make_type_name("Vec", make_type_name_segment("T", {...})) -> Type_Name with segment "Vec" followed by templated
template <typename... Args>
  requires(
      (std::same_as<std::remove_cvref_t<Args>, Type_Name_Segment> || std::constructible_from<std::string, Args>) && ...
  )
inline Type_Name make_type_name(Args&&... a_args) {
  if constexpr (sizeof...(a_args) == 0) {
    return Type_Name{{}, {}};
  } else {
    std::vector<Type_Name_Segment> segments;
    segments.reserve(sizeof...(a_args));
    (
        [&] {
          if constexpr (std::same_as<std::remove_cvref_t<Args>, Type_Name_Segment>) {
            segments.push_back(std::forward<Args>(a_args));
          } else {
            segments.push_back(make_type_name_segment(std::string(std::forward<Args>(a_args))));
          }
        }(),
        ...
    );
    return Type_Name{{}, std::move(segments)};
  }
}

// Variable_Name helpers
inline Variable_Name_Segment make_variable_name_segment(
    std::string&& a_value, std::vector<Type_Name>&& a_template_parameters
) {
  return Variable_Name_Segment{{}, std::move(a_value), std::move(a_template_parameters)};
}

inline Variable_Name_Segment make_variable_name_segment(std::string&& a_value) {
  return make_variable_name_segment(std::move(a_value), {});
}

inline Variable_Name make_variable_name(std::vector<Variable_Name_Segment>&& a_segments) {
  return Variable_Name{{}, std::move(a_segments)};
}

template <typename... Args>
  requires(
      (std::same_as<std::remove_cvref_t<Args>, Variable_Name_Segment> || std::constructible_from<std::string, Args>) &&
      ...
  )
inline Variable_Name make_variable_name(Args&&... a_args) {
  if constexpr (sizeof...(a_args) == 0) {
    return Variable_Name{{}, {}};
  } else {
    std::vector<Variable_Name_Segment> segments;
    segments.reserve(sizeof...(a_args));
    (
        [&] {
          if constexpr (std::same_as<std::remove_cvref_t<Args>, Variable_Name_Segment>) {
            segments.push_back(std::forward<Args>(a_args));
          } else {
            segments.push_back(make_variable_name_segment(std::string(std::forward<Args>(a_args))));
          }
        }(),
        ...
    );
    return Variable_Name{{}, std::move(segments)};
  }
}

// Literal helpers
inline String make_string(std::string&& a_value) { return String{{}, std::move(a_value)}; }

inline Integer make_integer(std::string a_value) noexcept { return Integer{{}, std::move(a_value)}; }

// Struct literal helpers
inline Field_Initializer make_field_initializer(std::string&& a_name, Expr&& a_value) {
  return Field_Initializer{{}, std::move(a_name), std::move(a_value)};
}

inline Struct_Literal make_struct_literal(std::string&& a_type_name, std::vector<Field_Initializer>&& a_fields) {
  return Struct_Literal{{}, std::move(a_type_name), std::move(a_fields)};
}

// Expression helpers
inline Expr make_expr(Variable_Name&& a_name) { return Expr{std::move(a_name)}; }
inline Expr make_expr(String&& a_str) { return Expr{std::move(a_str)}; }
inline Expr make_expr(Integer&& a_integer) noexcept { return Expr{std::move(a_integer)}; }
inline Expr make_expr(Function_Call_Expr&& a_call) { return Expr{std::move(a_call)}; }
inline Expr make_expr(Field_Access_Expr&& a_access) { return Expr{std::move(a_access)}; }
inline Expr make_expr(Binary_Expr&& a_binary) { return Expr{std::move(a_binary)}; }
inline Expr make_expr(Unary_Expr&& a_unary) { return Expr{std::move(a_unary)}; }
inline Expr make_expr(If_Expr&& a_if) { return Expr{std::move(a_if)}; }
inline Expr make_expr(While_Expr&& a_while) { return Expr{std::move(a_while)}; }
inline Expr make_expr(Range_Expr&& a_range) { return Expr{std::move(a_range)}; }
inline Expr make_expr(Struct_Literal&& a_literal) { return Expr{std::move(a_literal)}; }

inline Function_Call_Expr make_function_call_expr(Variable_Name&& a_name, std::vector<Expr>&& a_parameters) {
  return Function_Call_Expr{{}, std::move(a_name), std::move(a_parameters)};
}

inline Field_Access_Expr make_field_access_expr(Expr&& a_object, std::string&& a_field_name) {
  return Field_Access_Expr{{}, std::move(a_object), std::move(a_field_name)};
}

inline Else_If_Clause make_else_if_clause(Expr&& a_condition, Block&& a_then_block) {
  return Else_If_Clause{{}, std::move(a_condition), std::move(a_then_block)};
}

inline If_Expr make_if_expr(
    Expr&& a_condition, Block&& a_then_block, std::vector<Else_If_Clause>&& a_else_ifs = {},
    boost::optional<Block>&& a_else_block = boost::none
) {
  boost::optional<boost::spirit::x3::forward_ast<Block>> else_block_wrapped;
  if (a_else_block.has_value()) {
    else_block_wrapped = std::move(*a_else_block);
  }
  return If_Expr{
      {}, std::move(a_condition), std::move(a_then_block), std::move(a_else_ifs), std::move(else_block_wrapped)
  };
}

inline While_Expr make_while_expr(Expr&& a_condition, Block&& a_body) {
  return While_Expr{{}, std::move(a_condition), std::move(a_body)};
}

inline For_Expr make_for_expr(std::string&& a_binding, Expr&& a_iterator, Block&& a_body) {
  return For_Expr{{}, std::move(a_binding), std::move(a_iterator), std::move(a_body)};
}

inline Range_Expr make_range_expr(Expr&& a_start, Expr&& a_end, bool a_inclusive) {
  return Range_Expr{{}, std::move(a_start), std::move(a_end), a_inclusive};
}

// Statement helpers
inline Function_Call_Statement make_function_call_statement(Function_Call_Expr&& a_expr) {
  return Function_Call_Statement{{}, std::move(a_expr)};
}

inline Return_Statement make_return_statement(Expr&& a_expr) { return Return_Statement{{}, std::move(a_expr)}; }

inline Break_Statement make_break_statement(boost::optional<Expr>&& a_value) {
  return Break_Statement{{}, std::move(a_value)};
}

inline If_Statement make_if_statement(If_Expr&& a_expr) { return If_Statement{{}, std::move(a_expr)}; }

inline While_Statement make_while_statement(While_Expr&& a_expr) { return While_Statement{{}, std::move(a_expr)}; }

inline For_Statement make_for_statement(For_Expr&& a_expr) { return For_Statement{{}, std::move(a_expr)}; }

inline Statement make_statement(Function_Call_Statement&& a_call) { return Statement{std::move(a_call)}; }
inline Statement make_statement(Return_Statement&& a_ret) { return Statement{std::move(a_ret)}; }
inline Statement make_statement(Block&& a_block) { return Statement{std::move(a_block)}; }
inline Statement make_statement(Function_Definition&& a_def) { return Statement{std::move(a_def)}; }
inline Statement make_statement(Struct_Definition&& a_def) { return Statement{std::move(a_def)}; }

inline Block make_block(std::vector<Statement>&& a_statements) { return Block{{}, std::move(a_statements)}; }

// Binary expression helper
inline Binary_Expr make_binary_expr(Expr&& a_lhs, Binary_Op a_op, Expr&& a_rhs) {
  return Binary_Expr{{}, std::move(a_lhs), a_op, std::move(a_rhs)};
}

// Unary expression helper
inline Unary_Expr make_unary_expr(Unary_Op a_op, Expr&& a_operand) {
  return Unary_Expr{{}, a_op, std::move(a_operand)};
}

// Function helpers
inline Function_Parameter make_function_parameter(bool a_is_mut, std::string&& a_name, Type_Name&& a_type) {
  return Function_Parameter{{}, a_is_mut, std::move(a_name), std::move(a_type)};
}

inline Function_Declaration make_function_declaration(
    std::string&& a_name, std::vector<Function_Parameter>&& a_parameters, Type_Name&& a_return_type
) {
  return Function_Declaration{{}, std::move(a_name), std::move(a_parameters), std::move(a_return_type)};
}

inline Function_Definition make_function_definition(Function_Declaration&& a_decl, Block&& a_body) {
  return Function_Definition{{}, std::move(a_decl), std::move(a_body)};
}

// Struct helpers
inline Struct_Field make_struct_field(std::string&& a_name, Type_Name&& a_type) {
  return Struct_Field{{}, std::move(a_name), std::move(a_type)};
}

inline Struct_Definition make_struct_definition(std::string&& a_name, std::vector<Struct_Field>&& a_fields) {
  return Struct_Definition{{}, std::move(a_name), std::move(a_fields)};
}

// Module helpers
inline Module make_module(std::vector<Statement>&& a_statements) { return Module{{}, std::move(a_statements)}; }

// ============================================================================
// JSON Serialization (using explicit construction for robustness)
// ============================================================================

// Forward declarations for recursive types
void to_json(nlohmann::json& a_json, Type_Name_Segment const& a_segment);
void to_json(nlohmann::json& a_json, Variable_Name_Segment const& a_segment);
void to_json(nlohmann::json& a_json, Expr const& a_expr);
void to_json(nlohmann::json& a_json, Binary_Expr const& a_expr);
void to_json(nlohmann::json& a_json, Unary_Expr const& a_expr);
void to_json(nlohmann::json& a_json, Function_Call_Expr const& a_call);
void to_json(nlohmann::json& a_json, Field_Access_Expr const& a_access);
void to_json(nlohmann::json& a_json, Field_Initializer const& a_initializer);
void to_json(nlohmann::json& a_json, Function_Definition const& a_def);
void to_json(nlohmann::json& a_json, Struct_Definition const& a_def);
void to_json(nlohmann::json& a_json, Block const& a_block);

// Type_Name serialization (Type_Name must be declared before Type_Name_Segment due to recursion)
inline void to_json(nlohmann::json& a_json, Type_Name const& a_type);

inline void to_json(nlohmann::json& a_json, Type_Name_Segment const& a_segment) {
  nlohmann::json obj;
  obj["value"] = a_segment.value;
  nlohmann::json template_params = nlohmann::json::array();
  for (auto const& type : a_segment.template_parameters) {
    nlohmann::json type_json;
    to_json(type_json, type);
    template_params.push_back(type_json);
  }
  obj["template_parameters"] = template_params;
  a_json[Type_Name_Segment::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Type_Name const& a_type) {
  nlohmann::json obj;
  nlohmann::json segments = nlohmann::json::array();
  for (auto const& segment : a_type.segments) {
    nlohmann::json segment_json;
    to_json(segment_json, segment);
    segments.push_back(segment_json);
  }
  obj["segments"] = segments;
  a_json[Type_Name::k_name] = obj;
}

// Variable_Name serialization
inline void to_json(nlohmann::json& a_json, Variable_Name_Segment const& a_segment) {
  nlohmann::json obj;
  obj["value"] = a_segment.value;
  nlohmann::json template_params = nlohmann::json::array();
  for (auto const& type : a_segment.template_parameters) {
    nlohmann::json type_json;
    to_json(type_json, type);
    template_params.push_back(type_json);
  }
  obj["template_parameters"] = template_params;
  a_json[Variable_Name_Segment::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Variable_Name const& a_name) {
  nlohmann::json obj;
  nlohmann::json segments = nlohmann::json::array();
  for (auto const& segment : a_name.segments) {
    nlohmann::json segment_json;
    to_json(segment_json, segment);
    segments.push_back(segment_json);
  }
  obj["segments"] = segments;
  a_json[Variable_Name::k_name] = obj;
}

// Literal serialization
inline void to_json(nlohmann::json& a_json, String const& a_str) { a_json[String::k_name] = {{"value", a_str.value}}; }

inline void to_json(nlohmann::json& a_json, Integer const& a_integer) {
  a_json[Integer::k_name] = {{"value", a_integer.value}};
}

// Struct literal serialization
inline void to_json(nlohmann::json& a_json, Field_Initializer const& a_initializer) {
  nlohmann::json obj;
  obj["name"] = a_initializer.name;
  nlohmann::json value_json;
  to_json(value_json, a_initializer.value.get());
  obj["value"] = value_json;
  a_json[Field_Initializer::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Struct_Literal const& a_literal) {
  nlohmann::json obj;
  obj["typeName"] = a_literal.type_name;
  nlohmann::json fields = nlohmann::json::array();
  for (auto const& field : a_literal.fields) {
    nlohmann::json field_json;
    to_json(field_json, field);
    fields.push_back(field_json);
  }
  obj["fields"] = fields;
  a_json[Struct_Literal::k_name] = obj;
}

// Unary operator serialization
inline void to_json(nlohmann::json& a_json, Unary_Op a_op) {
  switch (a_op) {
    case Unary_Op::Neg:
      a_json = "-";
      break;
    case Unary_Op::Pos:
      a_json = "+";
      break;
    case Unary_Op::Not:
      a_json = "!";
      break;
    case Unary_Op::BitNot:
      a_json = "~";
      break;
  }
}

// Binary operator serialization
inline void to_json(nlohmann::json& a_json, Binary_Op a_op) {
  switch (a_op) {
    case Binary_Op::Add:
      a_json = "+";
      break;
    case Binary_Op::Sub:
      a_json = "-";
      break;
    case Binary_Op::Mul:
      a_json = "*";
      break;
    case Binary_Op::Div:
      a_json = "/";
      break;
    case Binary_Op::Mod:
      a_json = "%";
      break;
    case Binary_Op::Eq:
      a_json = "==";
      break;
    case Binary_Op::Ne:
      a_json = "!=";
      break;
    case Binary_Op::Lt:
      a_json = "<";
      break;
    case Binary_Op::Gt:
      a_json = ">";
      break;
    case Binary_Op::Le:
      a_json = "<=";
      break;
    case Binary_Op::Ge:
      a_json = ">=";
      break;
    case Binary_Op::And:
      a_json = "&&";
      break;
    case Binary_Op::Or:
      a_json = "||";
      break;
  }
}

inline void to_json(nlohmann::json& a_json, Binary_Expr const& a_expr) {
  nlohmann::json obj;
  nlohmann::json lhs_json;
  to_json(lhs_json, a_expr.lhs.get());
  obj["lhs"] = lhs_json;
  nlohmann::json op_json;
  to_json(op_json, a_expr.op);
  obj["op"] = op_json;
  nlohmann::json rhs_json;
  to_json(rhs_json, a_expr.rhs.get());
  obj["rhs"] = rhs_json;
  a_json[Binary_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Unary_Expr const& a_expr) {
  nlohmann::json obj;
  nlohmann::json op_json;
  to_json(op_json, a_expr.op);
  obj["op"] = op_json;
  nlohmann::json operand_json;
  to_json(operand_json, a_expr.operand.get());
  obj["operand"] = operand_json;
  a_json[Unary_Expr::k_name] = obj;
}

// Expression serialization (implementations after all parts are declared)
inline void to_json(nlohmann::json& a_json, Function_Call_Expr const& a_call) {
  nlohmann::json obj;
  nlohmann::json name_json;
  to_json(name_json, a_call.name);
  obj["name"] = name_json;
  nlohmann::json params = nlohmann::json::array();
  for (auto const& param : a_call.parameters) {
    nlohmann::json param_json;
    to_json(param_json, param);
    params.push_back(param_json);
  }
  obj["parameters"] = params;
  a_json[Function_Call_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Field_Access_Expr const& a_access) {
  nlohmann::json obj;
  nlohmann::json object_json;
  to_json(object_json, a_access.object.get());
  obj["object"] = object_json;
  obj["field_name"] = a_access.field_name;
  a_json[Field_Access_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Else_If_Clause const& a_else_if) {
  nlohmann::json obj;
  nlohmann::json condition_json;
  to_json(condition_json, a_else_if.condition.get());
  obj["condition"] = condition_json;
  nlohmann::json then_json;
  to_json(then_json, a_else_if.then_block.get());
  obj["then_block"] = then_json;
  a_json = obj;
}

inline void to_json(nlohmann::json& a_json, If_Expr const& a_if) {
  nlohmann::json obj;
  nlohmann::json condition_json;
  to_json(condition_json, a_if.condition.get());
  obj["condition"] = condition_json;
  nlohmann::json then_json;
  to_json(then_json, a_if.then_block.get());
  obj["then_block"] = then_json;

  if (!a_if.else_ifs.empty()) {
    nlohmann::json else_ifs_json = nlohmann::json::array();
    for (auto const& else_if : a_if.else_ifs) {
      nlohmann::json else_if_json;
      to_json(else_if_json, else_if);
      else_ifs_json.push_back(else_if_json);
    }
    obj["else_ifs"] = else_ifs_json;
  }

  if (a_if.else_block.has_value()) {
    nlohmann::json else_json;
    to_json(else_json, a_if.else_block->get());
    obj["else_block"] = else_json;
  }
  a_json[If_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, While_Expr const& a_while) {
  nlohmann::json obj;
  nlohmann::json condition_json;
  to_json(condition_json, a_while.condition.get());
  obj["condition"] = condition_json;

  nlohmann::json body_json;
  to_json(body_json, a_while.body.get());
  obj["body"] = body_json;

  a_json[While_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, For_Expr const& a_for) {
  nlohmann::json obj;
  obj["binding"] = a_for.binding;

  nlohmann::json iterator_json;
  to_json(iterator_json, a_for.iterator.get());
  obj["iterator"] = iterator_json;

  nlohmann::json body_json;
  to_json(body_json, a_for.body.get());
  obj["body"] = body_json;

  a_json[For_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Range_Expr const& a_range) {
  nlohmann::json obj;
  nlohmann::json start_json;
  to_json(start_json, a_range.start.get());
  obj["start"] = start_json;

  nlohmann::json end_json;
  to_json(end_json, a_range.end.get());
  obj["end"] = end_json;

  obj["inclusive"] = a_range.inclusive;

  a_json[Range_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Expr const& a_expr) {
  boost::apply_visitor([&a_json](auto const& a_value) { to_json(a_json, a_value); }, a_expr);
}

// Statement serialization
inline void to_json(nlohmann::json& a_json, Function_Call_Statement const& a_call) {
  nlohmann::json obj;
  nlohmann::json expr_json;
  to_json(expr_json, a_call.expr);
  obj["expr"] = expr_json;
  a_json[Function_Call_Statement::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Return_Statement const& a_ret) {
  nlohmann::json obj;
  nlohmann::json expr_json;
  to_json(expr_json, a_ret.expr);
  obj["expr"] = expr_json;
  a_json[Return_Statement::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Break_Statement const& a_break) {
  nlohmann::json obj;
  if (a_break.value) {
    nlohmann::json value_json;
    to_json(value_json, *a_break.value);
    obj["value"] = value_json;
  } else {
    obj["value"] = nullptr;
  }
  a_json[Break_Statement::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, If_Statement const& a_if) {
  nlohmann::json obj;
  nlohmann::json expr_json;
  to_json(expr_json, a_if.expr.get());
  obj["expr"] = expr_json;
  a_json[If_Statement::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, While_Statement const& a_while) {
  nlohmann::json obj;
  nlohmann::json expr_json;
  to_json(expr_json, a_while.expr.get());
  obj["expr"] = expr_json;
  a_json[While_Statement::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, For_Statement const& a_for) {
  nlohmann::json obj;
  nlohmann::json expr_json;
  to_json(expr_json, a_for.expr.get());
  obj["expr"] = expr_json;
  a_json[For_Statement::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Statement const& a_stmt) {
  boost::apply_visitor([&a_json](auto const& a_value) { to_json(a_json, a_value); }, a_stmt);
}

inline void to_json(nlohmann::json& a_json, Block const& a_block) {
  nlohmann::json obj;
  nlohmann::json statements = nlohmann::json::array();
  for (auto const& stmt : a_block.statements) {
    nlohmann::json stmt_json;
    to_json(stmt_json, stmt);
    statements.push_back(stmt_json);
  }
  obj["statements"] = statements;
  a_json[Block::k_name] = obj;
}

// Function serialization
inline void to_json(nlohmann::json& a_json, Function_Parameter const& a_param) {
  nlohmann::json obj;
  obj["is_mut"] = a_param.is_mut;
  obj["name"] = a_param.name;
  nlohmann::json type_json;
  to_json(type_json, a_param.type);
  obj["type"] = type_json;
  a_json[Function_Parameter::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Function_Declaration const& a_decl) {
  nlohmann::json obj;
  obj["name"] = a_decl.name;
  nlohmann::json params = nlohmann::json::array();
  for (auto const& param : a_decl.parameters) {
    nlohmann::json param_json;
    to_json(param_json, param);
    params.push_back(param_json);
  }
  obj["parameters"] = params;
  nlohmann::json return_type_json;
  to_json(return_type_json, a_decl.return_type);
  obj["returnType"] = return_type_json;
  a_json[Function_Declaration::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Function_Definition const& a_def) {
  nlohmann::json obj;
  nlohmann::json decl_json;
  to_json(decl_json, a_def.declaration);
  obj["declaration"] = decl_json;
  nlohmann::json body_json;
  to_json(body_json, a_def.body);
  obj["body"] = body_json;
  a_json[Function_Definition::k_name] = obj;
}

// Struct serialization
inline void to_json(nlohmann::json& a_json, Struct_Field const& a_field) {
  nlohmann::json obj;
  obj["name"] = a_field.name;
  nlohmann::json type_json;
  to_json(type_json, a_field.type);
  obj["type"] = type_json;
  a_json[Struct_Field::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Struct_Definition const& a_def) {
  nlohmann::json obj;
  obj["name"] = a_def.name;
  nlohmann::json fields = nlohmann::json::array();
  for (auto const& field : a_def.fields) {
    nlohmann::json field_json;
    to_json(field_json, field);
    fields.push_back(field_json);
  }
  obj["fields"] = fields;
  a_json[Struct_Definition::k_name] = obj;
}

// Module serialization
inline void to_json(nlohmann::json& a_json, Module const& a_module) {
  nlohmann::json obj;
  nlohmann::json statements = nlohmann::json::array();
  for (auto const& stmt : a_module.statements) {
    nlohmann::json stmt_json;
    to_json(stmt_json, stmt);
    statements.push_back(stmt_json);
  }
  obj["statements"] = statements;
  a_json[Module::k_name] = obj;
}

// ============================================================================
// Utility
// ============================================================================

template <typename T>
concept JsonStringConvertible = requires(nlohmann::json& a_j, T const& a_t) {
  { life_lang::ast::to_json(a_j, a_t) };
};

std::string to_json_string(JsonStringConvertible auto const& a_t, int a_indent) {
  nlohmann::json json;
  life_lang::ast::to_json(json, a_t);
  return json.dump(a_indent);
}

}  // namespace life_lang::ast

#endif
