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
struct Assignment_Expr;
struct Binary_Expr;
struct If_Expr;
struct While_Expr;
struct For_Expr;
struct Match_Expr;
struct Range_Expr;
struct Function_Definition;
struct Block;
struct Struct_Definition;
struct Enum_Definition;
struct Impl_Block;
struct Expr;
struct Pattern;
struct Let_Statement;

// ============================================================================
// Type Name System (for type annotations)
// ============================================================================

// Example: Std.Map<Std.String, Vec<I32>>
struct Type_Name : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Type_Name";
  std::vector<Type_Name_Segment> segments;
};

// Example: Map<String, I32> where "Map" is value, type_params = [String, I32]
struct Type_Name_Segment : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Type_Name_Segment";
  std::string value;
  std::vector<Type_Name> type_params;
};

// ============================================================================
// Variable Name System (for variables and function names)
// ============================================================================

// Example: Std.IO.println (qualified function name) or my_var (simple variable)
struct Variable_Name : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Variable_Name";
  std::vector<Variable_Name_Segment> segments;
};

// Example: println<T> where "println" is value, type_params = [T]
struct Variable_Name_Segment : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Variable_Name_Segment";
  std::string value;
  std::vector<Type_Name> type_params;
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
// Optional suffix: I8, I16, I32, I64, U8, U16, U32, U64
struct Integer : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Integer";
  std::string value;
  boost::optional<std::string> suffix;  // Type suffix like "I32", "U64", etc.
};

// Example: 3.14 or 1.0e-10 or 2.5E+3 (stored as string for arbitrary precision)
// Optional suffix: F32, F64
struct Float : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Float";
  std::string value;
  boost::optional<std::string> suffix;  // Type suffix like "F32", "F64"
};

// Example: 'a' or '\n' or '世' (stored with quotes as "'a'")
struct Char : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Char";
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

// Example: foo.bar.baz() or Point { x: 1 + 2, y: calculate(z) } or x = 42
struct Expr : boost::spirit::x3::variant<
                  Variable_Name,
                  boost::spirit::x3::forward_ast<Function_Call_Expr>,
                  boost::spirit::x3::forward_ast<Field_Access_Expr>,
                  boost::spirit::x3::forward_ast<Binary_Expr>,
                  boost::spirit::x3::forward_ast<Unary_Expr>,
                  boost::spirit::x3::forward_ast<If_Expr>,
                  boost::spirit::x3::forward_ast<While_Expr>,
                  boost::spirit::x3::forward_ast<For_Expr>,
                  boost::spirit::x3::forward_ast<Match_Expr>,
                  boost::spirit::x3::forward_ast<Range_Expr>,
                  boost::spirit::x3::forward_ast<Assignment_Expr>,
                  Struct_Literal,
                  String,
                  Integer,
                  Float,
                  Char>,
              boost::spirit::x3::position_tagged {
  using Base_Type = boost::spirit::x3::variant<
      Variable_Name,
      boost::spirit::x3::forward_ast<Function_Call_Expr>,
      boost::spirit::x3::forward_ast<Field_Access_Expr>,
      boost::spirit::x3::forward_ast<Binary_Expr>,
      boost::spirit::x3::forward_ast<Unary_Expr>,
      boost::spirit::x3::forward_ast<If_Expr>,
      boost::spirit::x3::forward_ast<While_Expr>,
      boost::spirit::x3::forward_ast<For_Expr>,
      boost::spirit::x3::forward_ast<Match_Expr>,
      boost::spirit::x3::forward_ast<Range_Expr>,
      boost::spirit::x3::forward_ast<Assignment_Expr>,
      Struct_Literal,
      String,
      Integer,
      Float,
      Char>;
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

// Example: x = 42 or point.x = 10 or arr[i] = value (future)
// Assignment requires target to be mutable (checked in semantic analysis)
struct Assignment_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Assignment_Expr";
  boost::spirit::x3::forward_ast<Expr> target;  // LHS: variable or field access
  boost::spirit::x3::forward_ast<Expr> value;   // RHS: expression to assign
};

// ============================================================================
// Statement Types
// ============================================================================

// Example: Std.print("Hello"); as a standalone statement (not an expression)
struct Function_Call_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Function_Call_Statement";
  Function_Call_Expr expr;
};

// Example: x = 42;, y = y + 1;, foo();
// Statement form of any expression - evaluates expression and discards result
// Useful for assignments, function calls, or other expressions with side effects
struct Expression_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Expression_Statement";
  boost::spirit::x3::forward_ast<Expr> expr;
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

// Example: continue;
// Skips to next iteration of the loop (Phase 3)
struct Continue_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Continue_Statement";
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

// Example: Can be function def, struct def, enum def, let binding, function call, return, break, continue, if, while,
// for, or nested block
struct Statement : boost::spirit::x3::variant<
                       boost::spirit::x3::forward_ast<Function_Definition>,
                       boost::spirit::x3::forward_ast<Struct_Definition>,
                       boost::spirit::x3::forward_ast<Enum_Definition>,
                       boost::spirit::x3::forward_ast<Impl_Block>,
                       boost::spirit::x3::forward_ast<Let_Statement>,
                       Function_Call_Statement,
                       boost::spirit::x3::forward_ast<Expression_Statement>,
                       Return_Statement,
                       Break_Statement,
                       Continue_Statement,
                       boost::spirit::x3::forward_ast<If_Statement>,
                       boost::spirit::x3::forward_ast<While_Statement>,
                       boost::spirit::x3::forward_ast<For_Statement>,
                       boost::spirit::x3::forward_ast<Block>>,
                   boost::spirit::x3::position_tagged {
  using Base_Type = boost::spirit::x3::variant<
      boost::spirit::x3::forward_ast<Function_Definition>,
      boost::spirit::x3::forward_ast<Struct_Definition>,
      boost::spirit::x3::forward_ast<Enum_Definition>,
      boost::spirit::x3::forward_ast<Impl_Block>,
      boost::spirit::x3::forward_ast<Let_Statement>,
      Function_Call_Statement,
      boost::spirit::x3::forward_ast<Expression_Statement>,
      Return_Statement,
      Break_Statement,
      Continue_Statement,
      boost::spirit::x3::forward_ast<If_Statement>,
      boost::spirit::x3::forward_ast<While_Statement>,
      boost::spirit::x3::forward_ast<For_Statement>,
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

// ============================================================================
// Pattern Matching Types
// ============================================================================

// Example: item (simple variable binding in for loops)
// Wildcard pattern: _ (matches anything, doesn't bind)
struct Wildcard_Pattern : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Wildcard_Pattern";
};

// Literal pattern: 42, 3.14, "hello" (matches exact value)
struct Literal_Pattern : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Literal_Pattern";
  boost::spirit::x3::forward_ast<Expr> value;  // Integer, Float, or String literal
};

struct Simple_Pattern : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Simple_Pattern";
  std::string name;
};

// Example: x: 3 in pattern Point { x: 3, y: 4 }
struct Field_Pattern : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Field_Pattern";
  std::string name;
  boost::spirit::x3::forward_ast<Pattern> pattern;
};

// Example: Point { x: 3, y: 4 } (destructure struct fields in match expressions)
// Supports nesting: Point { x: 3, inner: Line { a: 1, b: 2 } } where fields have patterns
struct Struct_Pattern : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Struct_Pattern";
  Type_Name type_name;
  std::vector<Field_Pattern> fields;
};

// Example: (a, b, c) (destructure tuple elements in for loops)
// Supports nesting: (a, (b, c)) where elements are patterns
struct Tuple_Pattern : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Tuple_Pattern";
  std::vector<boost::spirit::x3::forward_ast<Pattern>> elements;
};

// Pattern variant supporting all pattern types
struct Pattern
    : boost::spirit::x3::variant<Wildcard_Pattern, Literal_Pattern, Simple_Pattern, Struct_Pattern, Tuple_Pattern>,
      boost::spirit::x3::position_tagged {
  using Base_Type =
      boost::spirit::x3::variant<Wildcard_Pattern, Literal_Pattern, Simple_Pattern, Struct_Pattern, Tuple_Pattern>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// ============================================================================
// Variable Binding Types
// ============================================================================

// Example: let x = 42; or let mut y: I32 = calculate(); or let (a, b) = tuple;
// Introduces a new binding with optional type annotation and optional mutability
struct Let_Statement : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Let_Statement";
  bool is_mut;                                 // true if 'mut' keyword present
  Pattern pattern;                             // Binding pattern (simple, struct, or tuple)
  boost::optional<Type_Name> type;             // Optional type annotation
  boost::spirit::x3::forward_ast<Expr> value;  // Initializer expression
};

// ============================================================================
// Loop Types
// ============================================================================

// Example: while x < 10 { x = x + 1; }
// Loop continues while condition is true
struct While_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "While_Expr";
  boost::spirit::x3::forward_ast<Expr> condition;
  boost::spirit::x3::forward_ast<Block> body;
};

// Example: for item in 0..10 { process(item); } or for (a, b) in pairs { }
// Iterates over collection or range with pattern matching
struct For_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "For_Expr";
  Pattern pattern;                                // Pattern for destructuring (simple, struct, or tuple)
  boost::spirit::x3::forward_ast<Expr> iterator;  // Collection or range expression
  boost::spirit::x3::forward_ast<Block> body;
};

// Example: Point { x: 0, y } if y > 0 => "positive"
// Single arm in a match expression with optional guard
struct Match_Arm : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Match_Arm";
  Pattern pattern;                                              // Pattern to match against
  boost::optional<boost::spirit::x3::forward_ast<Expr>> guard;  // Optional guard condition (if guard_expr)
  boost::spirit::x3::forward_ast<Expr> result;                  // Expression to evaluate if pattern matches
};

// Example: match value { 0 => "zero", n if n > 0 => "positive", _ => "other" }
// Pattern matching expression with exhaustive case analysis
struct Match_Expr : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Match_Expr";
  boost::spirit::x3::forward_ast<Expr> scrutinee;  // Expression to match against
  std::vector<Match_Arm> arms;                     // Match arms (pattern => result)
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
  std::vector<Type_Name> type_params;  // Generic parameters: <T>, <T, U>
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
// Example: struct Box<T> { value: T }
struct Struct_Definition : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Struct_Definition";
  std::string name;
  std::vector<Type_Name> type_params;  // Generic parameters: <T>, <K, V>
  std::vector<Struct_Field> fields;
};

// ============================================================================
// Enum Types
// ============================================================================

// Unit variant: Red, None, False
struct Unit_Variant : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Unit_Variant";
  std::string name;  // Variant name (must be Camel_Snake_Case)
};

// Tuple variant: Some(T), Rgb(I32, I32, I32)
struct Tuple_Variant : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Tuple_Variant";
  std::string name;                     // Variant name (must be Camel_Snake_Case)
  std::vector<Type_Name> tuple_fields;  // Positional field types
};

// Struct variant: Point { x: I32, y: I32 }
struct Struct_Variant : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Struct_Variant";
  std::string name;                         // Variant name (must be Camel_Snake_Case)
  std::vector<Struct_Field> struct_fields;  // Named fields
};

// Example: Some(value), None, Red, Rgb(255, 0, 0), Point { x, y }
// Represents a single variant in an enum definition
// Uses boost::variant for type safety and consistency with rest of AST
struct Enum_Variant : boost::spirit::x3::variant<Unit_Variant, Tuple_Variant, Struct_Variant>,
                      boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Enum_Variant";
  using Base_Type = boost::spirit::x3::variant<Unit_Variant, Tuple_Variant, Struct_Variant>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: enum Option<T> { Some(T), None }
// Example: enum Color { Red, Green, Blue, Rgb(I32, I32, I32) }
// Example: enum Result<T, E> { Ok(T), Err(E) }
struct Enum_Definition : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Enum_Definition";
  std::string name;                    // Enum name (must be Camel_Snake_Case)
  std::vector<Type_Name> type_params;  // Generic parameters: <T>, <T, E>
  std::vector<Enum_Variant> variants;  // List of variants
};

// ============================================================================
// Impl Blocks
// ============================================================================

// Example: impl Point { fn distance(self): F64 { ... } }
// Example: impl<T> Array<T> { fn len(self): I32 { ... } }
struct Impl_Block : boost::spirit::x3::position_tagged {
  static constexpr std::string_view k_name = "Impl_Block";
  Type_Name type_name;                       // Type being implemented (e.g., Point, Array<T>)
  std::vector<Type_Name> type_params;        // Generic parameters: <T>, <K, V>
  std::vector<Function_Definition> methods;  // Methods in the impl block
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
inline Type_Name_Segment make_type_name_segment(std::string&& a_value, std::vector<Type_Name>&& a_type_parameters) {
  return Type_Name_Segment{{}, std::move(a_value), std::move(a_type_parameters)};
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
inline Variable_Name_Segment
make_variable_name_segment(std::string&& a_value, std::vector<Type_Name>&& a_type_parameters) {
  return Variable_Name_Segment{{}, std::move(a_value), std::move(a_type_parameters)};
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
inline String make_string(std::string&& a_value) {
  return String{{}, std::move(a_value)};
}

inline Integer make_integer(std::string a_value, boost::optional<std::string> a_suffix = boost::none) noexcept {
  return Integer{{}, std::move(a_value), std::move(a_suffix)};
}

inline Float make_float(std::string a_value, boost::optional<std::string> a_suffix = boost::none) noexcept {
  return Float{{}, std::move(a_value), std::move(a_suffix)};
}

inline Char make_char(std::string&& a_value) {
  return Char{{}, std::move(a_value)};
}

// Struct literal helpers
inline Field_Initializer make_field_initializer(std::string&& a_name, Expr&& a_value) {
  return Field_Initializer{{}, std::move(a_name), std::move(a_value)};
}

inline Struct_Literal make_struct_literal(std::string&& a_type_name, std::vector<Field_Initializer>&& a_fields) {
  return Struct_Literal{{}, std::move(a_type_name), std::move(a_fields)};
}

// Expression helpers
inline Expr make_expr(Variable_Name&& a_name) {
  return Expr{std::move(a_name)};
}
inline Expr make_expr(String&& a_str) {
  return Expr{std::move(a_str)};
}
inline Expr make_expr(Integer&& a_integer) noexcept {
  return Expr{std::move(a_integer)};
}
inline Expr make_expr(Float&& a_float) noexcept {
  return Expr{std::move(a_float)};
}
inline Expr make_expr(Char&& a_char) {
  return Expr{std::move(a_char)};
}
inline Expr make_expr(Function_Call_Expr&& a_call) {
  return Expr{std::move(a_call)};
}
inline Expr make_expr(Field_Access_Expr&& a_access) {
  return Expr{std::move(a_access)};
}
inline Expr make_expr(Assignment_Expr&& a_assignment) {
  return Expr{std::move(a_assignment)};
}
inline Expr make_expr(Binary_Expr&& a_binary) {
  return Expr{std::move(a_binary)};
}
inline Expr make_expr(Unary_Expr&& a_unary) {
  return Expr{std::move(a_unary)};
}
inline Expr make_expr(If_Expr&& a_if) {
  return Expr{std::move(a_if)};
}
inline Expr make_expr(While_Expr&& a_while) {
  return Expr{std::move(a_while)};
}
inline Expr make_expr(For_Expr&& a_for) {
  return Expr{std::move(a_for)};
}
inline Expr make_expr(Match_Expr&& a_match) {
  return Expr{std::move(a_match)};
}
inline Expr make_expr(Range_Expr&& a_range) {
  return Expr{std::move(a_range)};
}
inline Expr make_expr(Struct_Literal&& a_literal) {
  return Expr{std::move(a_literal)};
}

inline Function_Call_Expr make_function_call_expr(Variable_Name&& a_name, std::vector<Expr>&& a_parameters) {
  return Function_Call_Expr{{}, std::move(a_name), std::move(a_parameters)};
}

inline Field_Access_Expr make_field_access_expr(Expr&& a_object, std::string&& a_field_name) {
  return Field_Access_Expr{{}, std::move(a_object), std::move(a_field_name)};
}

inline Assignment_Expr make_assignment_expr(Expr&& a_target, Expr&& a_value) {
  return Assignment_Expr{{}, std::move(a_target), std::move(a_value)};
}

inline Else_If_Clause make_else_if_clause(Expr&& a_condition, Block&& a_then_block) {
  return Else_If_Clause{{}, std::move(a_condition), std::move(a_then_block)};
}

inline If_Expr make_if_expr(
    Expr&& a_condition,
    Block&& a_then_block,
    std::vector<Else_If_Clause>&& a_else_ifs = {},
    boost::optional<Block>&& a_else_block = boost::none
) {
  boost::optional<boost::spirit::x3::forward_ast<Block>> else_block_wrapped;
  if (a_else_block.has_value()) {
    else_block_wrapped = std::move(*std::move(a_else_block));
  }
  return If_Expr{
      {},
      std::move(a_condition),
      std::move(a_then_block),
      std::move(a_else_ifs),
      std::move(else_block_wrapped)
  };
}

inline While_Expr make_while_expr(Expr&& a_condition, Block&& a_body) {
  return While_Expr{{}, std::move(a_condition), std::move(a_body)};
}

// Pattern helpers
inline Wildcard_Pattern make_wildcard_pattern() {
  return Wildcard_Pattern{{}};
}

inline Literal_Pattern make_literal_pattern(Expr&& a_value) {
  return Literal_Pattern{{}, std::move(a_value)};
}

inline Simple_Pattern make_simple_pattern(std::string&& a_name) {
  return Simple_Pattern{{}, std::move(a_name)};
}

inline Field_Pattern make_field_pattern(std::string&& a_name, Pattern&& a_pattern) {
  return Field_Pattern{{}, std::move(a_name), std::move(a_pattern)};
}

inline Struct_Pattern make_struct_pattern(Type_Name&& a_type_name, std::vector<Field_Pattern>&& a_fields) {
  return Struct_Pattern{{}, std::move(a_type_name), std::move(a_fields)};
}

inline Tuple_Pattern make_tuple_pattern(std::vector<boost::spirit::x3::forward_ast<Pattern>>&& a_elements) {
  return Tuple_Pattern{{}, std::move(a_elements)};
}

inline Pattern make_pattern(Wildcard_Pattern const& a_pattern) {
  return Pattern{a_pattern};
}
inline Pattern make_pattern(Literal_Pattern&& a_pattern) {
  return Pattern{std::move(a_pattern)};
}
inline Pattern make_pattern(Simple_Pattern&& a_pattern) {
  return Pattern{std::move(a_pattern)};
}
inline Pattern make_pattern(Struct_Pattern&& a_pattern) {
  return Pattern{std::move(a_pattern)};
}
inline Pattern make_pattern(Tuple_Pattern&& a_pattern) {
  return Pattern{std::move(a_pattern)};
}

inline For_Expr make_for_expr(Pattern&& a_pattern, Expr&& a_iterator, Block&& a_body) {
  return For_Expr{{}, std::move(a_pattern), std::move(a_iterator), std::move(a_body)};
}

inline Match_Arm make_match_arm(Pattern&& a_pattern, boost::optional<Expr>&& a_guard, Expr&& a_result) {
  boost::optional<boost::spirit::x3::forward_ast<Expr>> guard_wrapped;
  if (a_guard) {
    guard_wrapped = *std::move(a_guard);
  }
  return Match_Arm{{}, std::move(a_pattern), std::move(guard_wrapped), std::move(a_result)};
}

inline Match_Expr make_match_expr(Expr&& a_scrutinee, std::vector<Match_Arm>&& a_arms) {
  return Match_Expr{{}, std::move(a_scrutinee), std::move(a_arms)};
}

inline Range_Expr make_range_expr(Expr&& a_start, Expr&& a_end, bool a_inclusive) {
  return Range_Expr{{}, std::move(a_start), std::move(a_end), a_inclusive};
}

// Statement helpers
inline Function_Call_Statement make_function_call_statement(Function_Call_Expr&& a_expr) {
  return Function_Call_Statement{{}, std::move(a_expr)};
}

inline Expression_Statement make_expression_statement(Expr&& a_expr) {
  return Expression_Statement{{}, std::move(a_expr)};
}

inline Return_Statement make_return_statement(Expr&& a_expr) {
  return Return_Statement{{}, std::move(a_expr)};
}

inline Break_Statement make_break_statement(boost::optional<Expr>&& a_value) {
  return Break_Statement{{}, std::move(a_value)};
}

inline Continue_Statement make_continue_statement() {
  return Continue_Statement{{}};
}

inline If_Statement make_if_statement(If_Expr&& a_expr) {
  return If_Statement{{}, std::move(a_expr)};
}

inline While_Statement make_while_statement(While_Expr&& a_expr) {
  return While_Statement{{}, std::move(a_expr)};
}

inline For_Statement make_for_statement(For_Expr&& a_expr) {
  return For_Statement{{}, std::move(a_expr)};
}

inline Let_Statement
make_let_statement(bool a_is_mut, Pattern&& a_pattern, boost::optional<Type_Name>&& a_type, Expr&& a_value) {
  return Let_Statement{{}, a_is_mut, std::move(a_pattern), std::move(a_type), std::move(a_value)};
}

inline Statement make_statement(Function_Call_Statement&& a_call) {
  return Statement{std::move(a_call)};
}
inline Statement make_statement(Expression_Statement&& a_expr) {
  return Statement{std::move(a_expr)};
}
inline Statement make_statement(Return_Statement&& a_ret) {
  return Statement{std::move(a_ret)};
}
inline Statement make_statement(Let_Statement&& a_let) {
  return Statement{std::move(a_let)};
}
inline Statement make_statement(Block&& a_block) {
  return Statement{std::move(a_block)};
}
inline Statement make_statement(Function_Definition&& a_def) {
  return Statement{std::move(a_def)};
}
inline Statement make_statement(Struct_Definition&& a_def) {
  return Statement{std::move(a_def)};
}

inline Block make_block(std::vector<Statement>&& a_statements) {
  return Block{{}, std::move(a_statements)};
}

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
    std::string&& a_name,
    std::vector<Type_Name>&& a_type_params,
    std::vector<Function_Parameter>&& a_parameters,
    Type_Name&& a_return_type
) {
  return Function_Declaration{
      {},
      std::move(a_name),
      std::move(a_type_params),
      std::move(a_parameters),
      std::move(a_return_type)
  };
}

inline Function_Definition make_function_definition(Function_Declaration&& a_decl, Block&& a_body) {
  return Function_Definition{{}, std::move(a_decl), std::move(a_body)};
}

// Struct helpers
inline Struct_Field make_struct_field(std::string&& a_name, Type_Name&& a_type) {
  return Struct_Field{{}, std::move(a_name), std::move(a_type)};
}

inline Struct_Definition make_struct_definition(
    std::string&& a_name,
    std::vector<Type_Name>&& a_type_parameters,
    std::vector<Struct_Field>&& a_fields
) {
  return Struct_Definition{{}, std::move(a_name), std::move(a_type_parameters), std::move(a_fields)};
}

inline Struct_Definition make_struct_definition(std::string&& a_name, std::vector<Struct_Field>&& a_fields) {
  return make_struct_definition(std::move(a_name), {}, std::move(a_fields));
}

// Enum helpers
inline Enum_Variant make_enum_variant(std::string&& a_name) {
  return Enum_Variant{Unit_Variant{{}, std::move(a_name)}};
}

inline Enum_Variant make_enum_variant(std::string&& a_name, std::vector<Type_Name>&& a_tuple_fields) {
  return Enum_Variant{Tuple_Variant{{}, std::move(a_name), std::move(a_tuple_fields)}};
}

inline Enum_Variant make_enum_variant(std::string&& a_name, std::vector<Struct_Field>&& a_struct_fields) {
  return Enum_Variant{Struct_Variant{{}, std::move(a_name), std::move(a_struct_fields)}};
}

inline Enum_Definition make_enum_definition(
    std::string&& a_name,
    std::vector<Type_Name>&& a_type_parameters,
    std::vector<Enum_Variant>&& a_variants
) {
  return Enum_Definition{{}, std::move(a_name), std::move(a_type_parameters), std::move(a_variants)};
}

inline Enum_Definition make_enum_definition(std::string&& a_name, std::vector<Enum_Variant>&& a_variants) {
  return make_enum_definition(std::move(a_name), {}, std::move(a_variants));
}

// Impl block helpers
inline Impl_Block make_impl_block(
    Type_Name&& a_type_name,
    std::vector<Type_Name>&& a_type_params,
    std::vector<Function_Definition>&& a_methods
) {
  return Impl_Block{{}, std::move(a_type_name), std::move(a_type_params), std::move(a_methods)};
}

inline Impl_Block make_impl_block(Type_Name&& a_type_name, std::vector<Function_Definition>&& a_methods) {
  return make_impl_block(std::move(a_type_name), {}, std::move(a_methods));
}

// Module helpers
inline Module make_module(std::vector<Statement>&& a_statements) {
  return Module{{}, std::move(a_statements)};
}

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
void to_json(nlohmann::json& a_json, Assignment_Expr const& a_assignment);
void to_json(nlohmann::json& a_json, Field_Initializer const& a_initializer);
void to_json(nlohmann::json& a_json, Let_Statement const& a_let);
void to_json(nlohmann::json& a_json, Match_Arm const& a_arm);
void to_json(nlohmann::json& a_json, Match_Expr const& a_match);
void to_json(nlohmann::json& a_json, Function_Definition const& a_def);
void to_json(nlohmann::json& a_json, Struct_Definition const& a_def);
void to_json(nlohmann::json& a_json, Enum_Variant const& a_variant);
void to_json(nlohmann::json& a_json, Enum_Definition const& a_def);
void to_json(nlohmann::json& a_json, Impl_Block const& a_impl);
void to_json(nlohmann::json& a_json, Block const& a_block);

// Type_Name serialization (Type_Name must be declared before Type_Name_Segment due to recursion)
inline void to_json(nlohmann::json& a_json, Type_Name const& a_type);

inline void to_json(nlohmann::json& a_json, Type_Name_Segment const& a_segment) {
  nlohmann::json obj;
  obj["value"] = a_segment.value;
  nlohmann::json template_params = nlohmann::json::array();
  for (auto const& type : a_segment.type_params) {
    nlohmann::json type_json;
    to_json(type_json, type);
    template_params.push_back(type_json);
  }
  obj["type_params"] = template_params;
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
  for (auto const& type : a_segment.type_params) {
    nlohmann::json type_json;
    to_json(type_json, type);
    template_params.push_back(type_json);
  }
  obj["type_params"] = template_params;
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
inline void to_json(nlohmann::json& a_json, String const& a_str) {
  a_json[String::k_name] = {{"value", a_str.value}};
}

inline void to_json(nlohmann::json& a_json, Integer const& a_integer) {
  nlohmann::json obj;
  obj["value"] = a_integer.value;
  if (a_integer.suffix) {
    obj["suffix"] = *a_integer.suffix;
  }
  a_json[Integer::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Float const& a_float) {
  nlohmann::json obj;
  obj["value"] = a_float.value;
  if (a_float.suffix) {
    obj["suffix"] = *a_float.suffix;
  }
  a_json[Float::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Char const& a_char) {
  a_json[Char::k_name] = {{"value", a_char.value}};
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

inline void to_json(nlohmann::json& a_json, Assignment_Expr const& a_assignment) {
  nlohmann::json obj;
  nlohmann::json target_json;
  to_json(target_json, a_assignment.target.get());
  obj["target"] = target_json;
  nlohmann::json value_json;
  to_json(value_json, a_assignment.value.get());
  obj["value"] = value_json;
  a_json[Assignment_Expr::k_name] = obj;
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

// Pattern JSON serialization (forward declare for recursion)
inline void to_json(nlohmann::json& a_json, Pattern const& a_pattern);

inline void to_json(nlohmann::json& a_json, Wildcard_Pattern const& /*a_pattern*/) {
  a_json[Wildcard_Pattern::k_name] = nlohmann::json::object();
}

inline void to_json(nlohmann::json& a_json, Literal_Pattern const& a_pattern) {
  nlohmann::json obj;
  nlohmann::json value_json;
  to_json(value_json, a_pattern.value.get());
  obj["value"] = value_json;
  a_json[Literal_Pattern::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Simple_Pattern const& a_pattern) {
  nlohmann::json obj;
  obj["name"] = a_pattern.name;
  a_json[Simple_Pattern::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Field_Pattern const& a_pattern) {
  nlohmann::json obj;
  obj["name"] = a_pattern.name;
  nlohmann::json pattern_json;
  to_json(pattern_json, a_pattern.pattern.get());
  obj["pattern"] = pattern_json;
  a_json[Field_Pattern::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Struct_Pattern const& a_pattern) {
  nlohmann::json obj;
  nlohmann::json type_json;
  to_json(type_json, a_pattern.type_name);
  obj["type_name"] = type_json;

  nlohmann::json fields_array = nlohmann::json::array();
  for (auto const& field : a_pattern.fields) {
    nlohmann::json field_json;
    to_json(field_json, field);
    fields_array.push_back(field_json);
  }
  obj["fields"] = fields_array;
  a_json[Struct_Pattern::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Tuple_Pattern const& a_pattern) {
  nlohmann::json obj;
  nlohmann::json elements_array = nlohmann::json::array();
  for (auto const& element : a_pattern.elements) {
    nlohmann::json element_json;
    to_json(element_json, element.get());
    elements_array.push_back(element_json);
  }
  obj["elements"] = elements_array;
  a_json[Tuple_Pattern::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Pattern const& a_pattern) {
  boost::apply_visitor([&a_json](auto const& a_variant) { to_json(a_json, a_variant); }, a_pattern);
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
  nlohmann::json pattern_json;
  to_json(pattern_json, a_for.pattern);
  obj["pattern"] = pattern_json;

  nlohmann::json iterator_json;
  to_json(iterator_json, a_for.iterator.get());
  obj["iterator"] = iterator_json;

  nlohmann::json body_json;
  to_json(body_json, a_for.body.get());
  obj["body"] = body_json;

  a_json[For_Expr::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Match_Arm const& a_arm) {
  nlohmann::json obj;
  nlohmann::json pattern_json;
  to_json(pattern_json, a_arm.pattern);
  obj["pattern"] = pattern_json;

  if (a_arm.guard) {
    nlohmann::json guard_json;
    to_json(guard_json, a_arm.guard.get().get());
    obj["guard"] = guard_json;
  }

  nlohmann::json result_json;
  to_json(result_json, a_arm.result.get());
  obj["result"] = result_json;

  a_json[Match_Arm::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Match_Expr const& a_match) {
  nlohmann::json obj;
  nlohmann::json scrutinee_json;
  to_json(scrutinee_json, a_match.scrutinee.get());
  obj["scrutinee"] = scrutinee_json;

  nlohmann::json arms_json = nlohmann::json::array();
  for (auto const& arm : a_match.arms) {
    nlohmann::json arm_json;
    to_json(arm_json, arm);
    arms_json.push_back(arm_json);
  }
  obj["arms"] = arms_json;

  a_json[Match_Expr::k_name] = obj;
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

inline void to_json(nlohmann::json& a_json, Expression_Statement const& a_stmt) {
  nlohmann::json obj;
  nlohmann::json expr_json;
  to_json(expr_json, a_stmt.expr.get());
  obj["expr"] = expr_json;
  a_json[Expression_Statement::k_name] = obj;
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

inline void to_json(nlohmann::json& a_json, [[maybe_unused]] Continue_Statement const& a_continue) {
  a_json[Continue_Statement::k_name] = nullptr;
}

inline void to_json(nlohmann::json& a_json, Let_Statement const& a_let) {
  nlohmann::json obj;
  obj["is_mut"] = a_let.is_mut;
  nlohmann::json pattern_json;
  to_json(pattern_json, a_let.pattern);
  obj["pattern"] = pattern_json;
  if (a_let.type.has_value()) {
    nlohmann::json type_json;
    to_json(type_json, *a_let.type);
    obj["type"] = type_json;
  } else {
    obj["type"] = nullptr;
  }
  nlohmann::json value_json;
  to_json(value_json, a_let.value.get());
  obj["value"] = value_json;
  a_json[Let_Statement::k_name] = obj;
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
  nlohmann::json type_params = nlohmann::json::array();
  for (auto const& tp : a_decl.type_params) {
    nlohmann::json tp_json;
    to_json(tp_json, tp);
    type_params.push_back(tp_json);
  }
  obj["type_params"] = type_params;
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

  // Type parameters (optional)
  if (!a_def.type_params.empty()) {
    nlohmann::json type_params = nlohmann::json::array();
    for (auto const& param : a_def.type_params) {
      nlohmann::json param_json;
      to_json(param_json, param);
      type_params.push_back(param_json);
    }
    obj["type_params"] = type_params;
  }

  nlohmann::json fields = nlohmann::json::array();
  for (auto const& field : a_def.fields) {
    nlohmann::json field_json;
    to_json(field_json, field);
    fields.push_back(field_json);
  }
  obj["fields"] = fields;
  a_json[Struct_Definition::k_name] = obj;
}

// Enum serialization - individual variant types
inline void to_json(nlohmann::json& a_json, Unit_Variant const& a_variant) {
  nlohmann::json obj;
  obj["name"] = a_variant.name;
  obj["kind"] = "unit";
  a_json[Unit_Variant::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Tuple_Variant const& a_variant) {
  nlohmann::json obj;
  obj["name"] = a_variant.name;
  obj["kind"] = "tuple";
  nlohmann::json fields = nlohmann::json::array();
  for (auto const& type : a_variant.tuple_fields) {
    nlohmann::json type_json;
    to_json(type_json, type);
    fields.push_back(type_json);
  }
  obj["fields"] = fields;
  a_json[Tuple_Variant::k_name] = obj;
}

inline void to_json(nlohmann::json& a_json, Struct_Variant const& a_variant) {
  nlohmann::json obj;
  obj["name"] = a_variant.name;
  obj["kind"] = "struct";
  nlohmann::json fields = nlohmann::json::array();
  for (auto const& field : a_variant.struct_fields) {
    nlohmann::json field_json;
    to_json(field_json, field);
    fields.push_back(field_json);
  }
  obj["fields"] = fields;
  a_json[Struct_Variant::k_name] = obj;
}

// Enum_Variant serialization using visitor
inline void to_json(nlohmann::json& a_json, Enum_Variant const& a_variant) {
  boost::apply_visitor(
      [&a_json](auto const& variant) {
        nlohmann::json obj;
        to_json(obj, variant);
        // Extract the inner object from the wrapper
        a_json[Enum_Variant::k_name] = obj.begin().value();
      },
      a_variant
  );
}

inline void to_json(nlohmann::json& a_json, Enum_Definition const& a_def) {
  nlohmann::json obj;
  obj["name"] = a_def.name;

  // Type parameters (optional)
  if (!a_def.type_params.empty()) {
    nlohmann::json type_params = nlohmann::json::array();
    for (auto const& param : a_def.type_params) {
      nlohmann::json param_json;
      to_json(param_json, param);
      type_params.push_back(param_json);
    }
    obj["type_params"] = type_params;
  }

  // Variants
  nlohmann::json variants = nlohmann::json::array();
  for (auto const& variant : a_def.variants) {
    nlohmann::json variant_json;
    to_json(variant_json, variant);
    variants.push_back(variant_json);
  }
  obj["variants"] = variants;

  a_json[Enum_Definition::k_name] = obj;
}

// Impl block serialization
inline void to_json(nlohmann::json& a_json, Impl_Block const& a_impl) {
  nlohmann::json obj;

  // Type name
  nlohmann::json type_name_json;
  to_json(type_name_json, a_impl.type_name);
  obj["type_name"] = type_name_json;

  // Type parameters (optional)
  if (!a_impl.type_params.empty()) {
    nlohmann::json type_params = nlohmann::json::array();
    for (auto const& param : a_impl.type_params) {
      nlohmann::json param_json;
      to_json(param_json, param);
      type_params.push_back(param_json);
    }
    obj["type_params"] = type_params;
  }

  // Methods
  nlohmann::json methods = nlohmann::json::array();
  for (auto const& method : a_impl.methods) {
    nlohmann::json method_json;
    to_json(method_json, method);
    methods.push_back(method_json);
  }
  obj["methods"] = methods;

  a_json[Impl_Block::k_name] = obj;
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
