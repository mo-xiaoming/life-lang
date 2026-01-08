#ifndef LIFE_LANG_AST_HPP
#define LIFE_LANG_AST_HPP

#include <memory>
#include <optional>
#include <variant>

#include <string>
#include <vector>

#include "../diagnostics.hpp"

namespace life_lang::ast {

// ============================================================================
// Forward Declarations
// ============================================================================

struct Type_Name_Segment;
struct Type_Name;
struct Function_Type;
struct Array_Type;
struct Trait_Bound;
struct Type_Param;
struct Where_Predicate;
struct Where_Clause;
struct Var_Name_Segment;
struct Func_Call_Expr;
struct Field_Access_Expr;
struct Array_Literal;
struct Index_Expr;
struct Binary_Expr;
struct If_Expr;
struct While_Expr;
struct For_Expr;
struct Match_Expr;
struct Range_Expr;
struct Func_Def;
struct Block;
struct Struct_Def;
struct Enum_Def;
struct Impl_Block;
struct Assoc_Type_Decl;
struct Assoc_Type_Impl;
struct Trait_Def;
struct Trait_Impl;
struct Type_Alias;
struct Expr;
struct Pattern;
struct Let_Statement;

// ============================================================================
// Type Name System (for type annotations)
// ============================================================================

// Function type: fn(T, U): R
// Examples: fn(I32): Bool, fn(String, I32): Result<T, E>
struct Function_Type {
  static constexpr std::string_view k_name = "Function_Type";
  Source_Range span{};
  std::vector<std::shared_ptr<Type_Name>> param_types;  // Parameter types
  std::shared_ptr<Type_Name> return_type;               // Return type
};

// Path-based type name: Std.Map<String, I32>
// This is the original Type_Name functionality extracted into its own struct
struct Path_Type {
  static constexpr std::string_view k_name = "Path_Type";
  Source_Range span{};
  std::vector<Type_Name_Segment> segments;
};

// Array type: [T; N]
// Examples: [I32; 4], [String; 10], [Vec<I32>; 3]
struct Array_Type {
  static constexpr std::string_view k_name = "Array_Type";
  Source_Range span{};
  std::shared_ptr<Type_Name> element_type;  // Element type
  std::optional<std::string> size;          // Array size (optional for unsized arrays like [T])
};

// Tuple type: (T, U, V, ...)
// Examples: (I32, String), (Bool, I32, I32), ((I32, I32), String)
// Note: Empty tuple () is represented as Path_Type with value "()", not Tuple_Type
// Note: Single-element tuple (T,) is represented as Tuple_Type with one element
struct Tuple_Type {
  static constexpr std::string_view k_name = "Tuple_Type";
  Source_Range span{};
  std::vector<Type_Name> element_types;  // Element types (must have at least 1)
};

// Type name: either a path-based type, function type, array type, or tuple type
// Examples: I32, Vec<T>, Std.String, fn(I32): Bool, [I32; 4], (I32, String)
struct Type_Name : std::variant<Path_Type, Function_Type, Array_Type, Tuple_Type> {
  static constexpr std::string_view k_name = "Type_Name";
  using Base_Type = std::variant<Path_Type, Function_Type, Array_Type, Tuple_Type>;
  using Base_Type::Base_Type;  // NOLINT(modernize-use-equals-default,hicpp-use-equals-default)
  using Base_Type::operator=;
};

// Example: Map<String, I32> where "Map" is value, type_params = [String, I32]
struct Type_Name_Segment {
  static constexpr std::string_view k_name = "Type_Name_Segment";
  Source_Range span{};
  std::string value;
  std::vector<Type_Name> type_params;
};

// ============================================================================
// Trait Bounds (for generic constraints)
// ============================================================================

// Example: Display (in T: Display)
struct Trait_Bound {
  static constexpr std::string_view k_name = "Trait_Bound";
  Source_Range span{};
  Type_Name trait_name;  // The trait being required (e.g., Display, Iterator<T>)
};

// Type parameter with optional inline trait bounds (in angle brackets)
// Inline bounds are limited to simple type parameters (T, U, Key, etc.)
// Example: T (no bounds), T: Display (single bound), T: Display + Clone (multiple bounds)
struct Type_Param {
  static constexpr std::string_view k_name = "Type_Param";
  Source_Range span{};
  Type_Name name;                   // Parameter name (e.g., T, U, Item) - always a simple identifier
  std::vector<Trait_Bound> bounds;  // Optional trait bounds (e.g., Display, Display + Clone)
};

// Where clause predicate: type constraint in where clause
// Where predicates support more complex type expressions than inline bounds:
// - Simple type parameters: T: Display
// - Associated types (future): T.Item: Display, <T as Iterator>.Item: Clone
// Example: T: Display + Clone (in "where T: Display + Clone")
struct Where_Predicate {
  static constexpr std::string_view k_name = "Where_Predicate";
  Source_Range span{};
  Type_Name type_name;              // Type being constrained (e.g., T, U.Item, <T as Iterator>.Item)
  std::vector<Trait_Bound> bounds;  // Required trait bounds
};

// Where clause: collection of predicates
// Where clauses enable complex constraints not expressible with inline bounds
// Example: where T: Display, U: Clone, V: Eq + Ord
struct Where_Clause {
  static constexpr std::string_view k_name = "Where_Clause";
  Source_Range span{};
  std::vector<Where_Predicate> predicates;
};

// ============================================================================
// Variable Name System (for variables and function names)
// ============================================================================

// Example: Std.IO.println (qualified function name) or my_var (simple variable)
struct Var_Name {
  static constexpr std::string_view k_name = "Var_Name";
  Source_Range span{};
  std::vector<Var_Name_Segment> segments;
};

// Example: println<T> where "println" is value, type_params = [T]
struct Var_Name_Segment {
  static constexpr std::string_view k_name = "Var_Name_Segment";
  Source_Range span{};
  std::string value;
  std::vector<Type_Name> type_params;
};

// ============================================================================
// Literal Types
// ============================================================================

// Example: "Hello, world!" stored with quotes as "\"Hello, world!\""
struct String {
  static constexpr std::string_view k_name = "String";
  Source_Range span{};
  std::string value;
};

// String interpolation part: either a literal string segment or an expression
// Example: "result: {x + 1}" has parts: ["result: ", <expr: x+1>, ""]
struct String_Interp_Part : std::variant<std::string, std::shared_ptr<Expr>> {
  using Base_Type = std::variant<std::string, std::shared_ptr<Expr>>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
  static constexpr std::string_view k_name = "String_Interp_Part";
};

// String interpolation: "Hello, {name}! You are {age} years old."
// Represented as alternating string literals and expressions
// Example: ["Hello, ", <name>, "! You are ", <age>, " years old."]
struct String_Interpolation {
  static constexpr std::string_view k_name = "String_Interpolation";
  Source_Range span{};
  std::vector<String_Interp_Part> parts;
};

// Example: 42 or 0x2A or 0b101010 (stored as string for arbitrary precision)
// Optional suffix: I8, I16, I32, I64, U8, U16, U32, U64
struct Integer {
  static constexpr std::string_view k_name = "Integer";
  Source_Range span{};
  std::string value;
  std::optional<std::string> suffix;  // Type suffix like "I32", "U64", etc.
};

// Example: 3.14 or 1.0e-10 or 2.5E+3 (stored as string for arbitrary precision)
// Optional suffix: F32, F64
struct Float {
  static constexpr std::string_view k_name = "Float";
  Source_Range span{};
  std::string value;
  std::optional<std::string> suffix;  // Type suffix like "F32", "F64"
};

// Example: 'a' or '\n' or '世' (stored with quotes as "'a'")
struct Char {
  static constexpr std::string_view k_name = "Char";
  Source_Range span{};
  std::string value;
};

// Boolean literal: true or false
struct Bool_Literal {
  static constexpr std::string_view k_name = "Bool_Literal";
  Source_Range span{};
  bool value{};
};

// Unit literal: () - represents "no value" or empty tuple
// Used in return statements for functions with unit return type: return ();
struct Unit_Literal {
  static constexpr std::string_view k_name = "Unit_Literal";
  Source_Range span{};
};

// ============================================================================
// Struct Literal Types (for initialization)
// ============================================================================

// Example: x: 10 in struct literal Point { x: 10, y: 20 }
struct Field_Initializer {
  static constexpr std::string_view k_name = "Field_Initializer";
  Source_Range span{};
  std::string name;
  std::shared_ptr<Expr> value;
};

// Example: Point { x: offset.x + 5, y: base.calculate() }
struct Struct_Literal {
  static constexpr std::string_view k_name = "Struct_Literal";
  Source_Range span{};
  std::string type_name;
  std::vector<Field_Initializer> fields;
};

// Array literal: [expr, expr, ...]
// Examples: [1, 2, 3], [x, y + 1, calculate()], [] (empty array)
struct Array_Literal {
  static constexpr std::string_view k_name = "Array_Literal";
  Source_Range span{};
  std::vector<Expr> elements;
};

// Tuple literal: (expr, expr, ...)
// Examples: (1, 2), ("name", 42, true), (x, y + 1)
// Note: Single element requires trailing comma: (x,) - otherwise it's a parenthesized expression
// Note: Empty tuple () is Unit_Literal, not Tuple_Literal
struct Tuple_Literal {
  static constexpr std::string_view k_name = "Tuple_Literal";
  Source_Range span{};
  std::vector<Expr> elements;
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

  // Bitwise operators
  Bit_And,  // &
  Bit_Or,   // |
  Bit_Xor,  // ^
  Shl,      // <<
  Shr,      // >>
};

// Example: x + y, a * (b - c), value == 42
struct Binary_Expr {
  static constexpr std::string_view k_name = "Binary_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> lhs;
  Binary_Op op{};
  std::shared_ptr<Expr> rhs;
};

// Unary operators (higher precedence than binary)
enum class Unary_Op : std::uint8_t {
  Neg,     // - (arithmetic negation)
  Pos,     // + (arithmetic positive/identity)
  Not,     // ! (logical NOT)
  BitNot,  // ~ (bitwise NOT)
};

// Example: -x, !flag, ~bits
struct Unary_Expr {
  static constexpr std::string_view k_name = "Unary_Expr";
  Source_Range span{};
  Unary_Op op{};
  std::shared_ptr<Expr> operand;
};

// Range expression: start..end (exclusive) or start..=end (inclusive)
// Examples: 0..10, start..end, 1..=100
struct Range_Expr {
  static constexpr std::string_view k_name = "Range_Expr";
  Source_Range span{};
  std::optional<std::shared_ptr<Expr>> start;  // None for unbounded start (..)
  std::optional<std::shared_ptr<Expr>> end;    // None for unbounded end (a..)
  bool inclusive{};                            // false for .., true for ..=
};

// Type cast expression: expr as Type
// Examples: x as I64, (y + 1) as F32, ptr as U64
// Performs explicit type conversion (validity checked in semantic analysis)
struct Cast_Expr {
  static constexpr std::string_view k_name = "Cast_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> expr;
  Type_Name target_type;
};

// ============================================================================
// Expression Types
// ============================================================================

// Example: foo.bar.baz() or Point { x: 1 + 2, y: calculate(z) } or x = 42
struct Expr : std::variant<
                  Var_Name,
                  std::shared_ptr<Func_Call_Expr>,
                  std::shared_ptr<Field_Access_Expr>,
                  std::shared_ptr<Index_Expr>,
                  std::shared_ptr<Binary_Expr>,
                  std::shared_ptr<Unary_Expr>,
                  std::shared_ptr<Cast_Expr>,
                  std::shared_ptr<If_Expr>,
                  std::shared_ptr<While_Expr>,
                  std::shared_ptr<For_Expr>,
                  std::shared_ptr<Match_Expr>,
                  std::shared_ptr<Block>,
                  std::shared_ptr<Range_Expr>,
                  Struct_Literal,
                  Array_Literal,
                  Tuple_Literal,
                  Unit_Literal,
                  Bool_Literal,
                  String,
                  String_Interpolation,
                  Integer,
                  Float,
                  Char> {
  using Base_Type = std::variant<
      Var_Name,
      std::shared_ptr<Func_Call_Expr>,
      std::shared_ptr<Field_Access_Expr>,
      std::shared_ptr<Index_Expr>,
      std::shared_ptr<Binary_Expr>,
      std::shared_ptr<Unary_Expr>,
      std::shared_ptr<Cast_Expr>,
      std::shared_ptr<If_Expr>,
      std::shared_ptr<While_Expr>,
      std::shared_ptr<For_Expr>,
      std::shared_ptr<Match_Expr>,
      std::shared_ptr<Block>,
      std::shared_ptr<Range_Expr>,
      Struct_Literal,
      Array_Literal,
      Tuple_Literal,
      Unit_Literal,
      Bool_Literal,
      String,
      String_Interpolation,
      Integer,
      Float,
      Char>;
  using Base_Type::Base_Type;  // NOLINT(modernize-use-equals-default,hicpp-use-equals-default)
  using Base_Type::operator=;
};

// Example: Std.print("Value: ", x, y + 2)
struct Func_Call_Expr {
  static constexpr std::string_view k_name = "Func_Call_Expr";
  Source_Range span{};
  Var_Name name;
  std::vector<Expr> params;
};

// Example: point.x or nested.obj.field (chained via recursive object field)
struct Field_Access_Expr {
  static constexpr std::string_view k_name = "Field_Access_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> object;
  std::string field_name;
};

// Index expression: array[index]
// Examples: arr[0], matrix[i][j], get_array()[x + 1]
struct Index_Expr {
  static constexpr std::string_view k_name = "Index_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> object;  // The array/indexable expression
  std::shared_ptr<Expr> index;   // The index expression
};

// ============================================================================
// Statement Types
// ============================================================================

// Example: x = 42 or point.x = 10 or arr[i] = value
// Assignment is a statement, not an expression - prevents confusing patterns
// like `x = y = z` and aligns with immutability-by-default philosophy
struct Assignment_Statement {
  static constexpr std::string_view k_name = "Assignment_Statement";
  Source_Range span{};
  std::shared_ptr<Expr> target;  // LHS: variable or field access
  std::shared_ptr<Expr> value;   // RHS: expression to assign
};

// Example: Std.print("Hello"); as a standalone statement (not an expression)
struct Func_Call_Statement {
  static constexpr std::string_view k_name = "Func_Call_Statement";
  Source_Range span{};
  Func_Call_Expr expr;
};

// Example: x = 42;, y = y + 1;, foo();
// Statement form of any expression - evaluates expression and discards result
// Useful for assignments, function calls, or other expressions with side effects
struct Expr_Statement {
  static constexpr std::string_view k_name = "Expr_Statement";
  Source_Range span{};
  std::shared_ptr<Expr> expr;
};

// Example: return calculate(x + y, Point { a: 1, b: 2 });
struct Return_Statement {
  static constexpr std::string_view k_name = "Return_Statement";
  Source_Range span{};
  Expr expr;
};

// Example: break; or break result_value;
// Used to exit loops early, optionally returning a value
struct Break_Statement {
  static constexpr std::string_view k_name = "Break_Statement";
  Source_Range span{};
  std::optional<Expr> value;  // Optional: break can be used without value
};

// Example: continue;
// Skips to next iteration of the loop
struct Continue_Statement {
  static constexpr std::string_view k_name = "Continue_Statement";
  Source_Range span{};
};

// If statement wrapper for using if expressions as statements
// When if is used for side effects (not in expression context), no semicolon needed
// Example: if condition { do_something(); }
struct If_Statement {
  static constexpr std::string_view k_name = "If_Statement";
  Source_Range span{};
  std::shared_ptr<If_Expr> expr;
};

// While statement wrapper for using while expressions as statements
// Example: while x < 10 { process(x); }
struct While_Statement {
  static constexpr std::string_view k_name = "While_Statement";
  Source_Range span{};
  std::shared_ptr<While_Expr> expr;
};

// For statement wrapper for using for expressions as statements
// Example: for item in 0..10 { process(item); }
struct For_Statement {
  static constexpr std::string_view k_name = "For_Statement";
  Source_Range span{};
  std::shared_ptr<For_Expr> expr;
};

// Example: Can be function def, struct def, enum def, let binding, function call, return, break, continue, if, while,
// for, or nested block
struct Statement : std::variant<
                       std::shared_ptr<Func_Def>,
                       std::shared_ptr<Struct_Def>,
                       std::shared_ptr<Enum_Def>,
                       std::shared_ptr<Impl_Block>,
                       std::shared_ptr<Trait_Def>,
                       std::shared_ptr<Trait_Impl>,
                       std::shared_ptr<Type_Alias>,
                       std::shared_ptr<Let_Statement>,
                       std::shared_ptr<Assignment_Statement>,
                       Func_Call_Statement,
                       std::shared_ptr<Expr_Statement>,
                       Return_Statement,
                       Break_Statement,
                       Continue_Statement,
                       std::shared_ptr<If_Statement>,
                       std::shared_ptr<While_Statement>,
                       std::shared_ptr<For_Statement>,
                       std::shared_ptr<Block>> {
  using Base_Type = std::variant<
      std::shared_ptr<Func_Def>,
      std::shared_ptr<Struct_Def>,
      std::shared_ptr<Enum_Def>,
      std::shared_ptr<Impl_Block>,
      std::shared_ptr<Trait_Def>,
      std::shared_ptr<Trait_Impl>,
      std::shared_ptr<Type_Alias>,
      std::shared_ptr<Let_Statement>,
      std::shared_ptr<Assignment_Statement>,
      Func_Call_Statement,
      std::shared_ptr<Expr_Statement>,
      Return_Statement,
      Break_Statement,
      Continue_Statement,
      std::shared_ptr<If_Statement>,
      std::shared_ptr<While_Statement>,
      std::shared_ptr<For_Statement>,
      std::shared_ptr<Block>>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: { Std.print(x); { nested(); } return 0; }
struct Block {
  static constexpr std::string_view k_name = "Block";
  Source_Range span{};
  std::vector<Statement> statements;
  std::optional<std::shared_ptr<Expr>> trailing_expr;  // Optional trailing expression
};

// Example: if x > 0 { x } else if x < 0 { -x } else { 0 }
// Chain structure: condition + then_block, plus optional else_ifs and final else_block
struct Else_If_Clause {
  static constexpr std::string_view k_name = "Else_If_Clause";
  Source_Range span{};
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Block> then_block;
};

struct If_Expr {
  static constexpr std::string_view k_name = "If_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Block> then_block;
  std::vector<Else_If_Clause> else_ifs;
  std::optional<std::shared_ptr<Block>> else_block;
};

// ============================================================================
// Pattern Matching Types
// ============================================================================

// Example: item (simple variable binding in for loops)
// Wildcard pattern: _ (matches anything, doesn't bind)
struct Wildcard_Pattern {
  static constexpr std::string_view k_name = "Wildcard_Pattern";
  Source_Range span{};
};

// Literal pattern: 42, 3.14, "hello" (matches exact value)
struct Literal_Pattern {
  static constexpr std::string_view k_name = "Literal_Pattern";
  Source_Range span{};
  std::shared_ptr<Expr> value;  // Integer, Float, or String literal
};

// Simple identifier pattern: binds matched value to a variable
// Examples: x, value, item (in let x = 42; or match expr { x => ... } or for item in items { ... })
struct Simple_Pattern {
  static constexpr std::string_view k_name = "Simple_Pattern";
  Source_Range span{};
  std::string name;
};

// Example: x: 3 in pattern Point { x: 3, y: 4 }
struct Field_Pattern {
  static constexpr std::string_view k_name = "Field_Pattern";
  Source_Range span{};
  std::string name;
  std::shared_ptr<Pattern> pattern;
};

// Example: Point { x: 3, y: 4 } (destructure struct fields in match expressions)
// Supports nesting: Point { x: 3, inner: Line { a: 1, b: 2 } } where fields have patterns
struct Struct_Pattern {
  static constexpr std::string_view k_name = "Struct_Pattern";
  Source_Range span{};
  Type_Name type_name;
  std::vector<Field_Pattern> fields;
  bool has_rest = false;  // true if pattern contains .. to ignore remaining fields
};

// Example: (a, b, c) (destructure tuple elements in for loops)
// Supports nesting: (a, (b, c)) where elements are patterns
struct Tuple_Pattern {
  static constexpr std::string_view k_name = "Tuple_Pattern";
  Source_Range span{};
  std::vector<std::shared_ptr<Pattern>> elements;
};

// Enum pattern: matches enum variants with optional tuple arguments
// Examples:
//   Option.Some(x)        - enum tuple variant with one arg
//   Result.Ok(value)      - qualified enum variant
//   Color.Rgb(r, g, b)    - multiple tuple args
//   Status.Active         - unit variant (no args)
struct Enum_Pattern {
  static constexpr std::string_view k_name = "Enum_Pattern";
  Source_Range span{};
  Type_Name type_name;                             // Enum variant name (can be qualified)
  std::vector<std::shared_ptr<Pattern>> patterns;  // Optional tuple patterns (empty for unit variants)
};

// Or pattern: matches any of multiple alternatives
// Examples:
//   1 | 2 | 3                  - simple alternatives
//   Some(1) | Some(2) | None   - top-level alternatives (different variants)
//   Some(1 | 2 | 3)            - nested alternatives (same variant, different values)
// Semantic constraint: All alternatives must bind the same variables with the same types
struct Or_Pattern {
  static constexpr std::string_view k_name = "Or_Pattern";
  Source_Range span{};
  std::vector<std::shared_ptr<Pattern>> alternatives;  // At least 2 alternatives
};

// Pattern variant supporting all pattern types
struct Pattern : std::variant<
                     Wildcard_Pattern,
                     Literal_Pattern,
                     Simple_Pattern,
                     Struct_Pattern,
                     Tuple_Pattern,
                     Enum_Pattern,
                     Or_Pattern> {
  using Base_Type = std::variant<
      Wildcard_Pattern,
      Literal_Pattern,
      Simple_Pattern,
      Struct_Pattern,
      Tuple_Pattern,
      Enum_Pattern,
      Or_Pattern>;
  using Base_Type::Base_Type;  // NOLINT(modernize-use-equals-default,hicpp-use-equals-default)
  using Base_Type::operator=;
};

// ============================================================================
// Variable Binding Types
// ============================================================================

// Example: let x = 42; or let mut y: I32 = calculate(); or let (a, b) = tuple;
// Introduces a new binding with optional type annotation and optional mutability
struct Let_Statement {
  static constexpr std::string_view k_name = "Let_Statement";
  Source_Range span{};
  bool is_mut{false};             // true if 'mut' keyword present
  Pattern pattern;                // Binding pattern (simple, struct, or tuple)
  std::optional<Type_Name> type;  // Optional type annotation
  std::shared_ptr<Expr> value;    // Initializer expression
};

// ============================================================================
// Loop Types
// ============================================================================

// Example: while x < 10 { x = x + 1; }
// Loop continues while condition is true
struct While_Expr {
  static constexpr std::string_view k_name = "While_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Block> body;
};

// Example: for item in 0..10 { process(item); } or for (a, b) in pairs { }
// Iterates over collection or range with pattern matching
struct For_Expr {
  static constexpr std::string_view k_name = "For_Expr";
  Source_Range span{};
  Pattern pattern;                 // Pattern for destructuring (simple, struct, or tuple)
  std::shared_ptr<Expr> iterator;  // Collection or range expression
  std::shared_ptr<Block> body;
};

// Example: Point { x: 0, y } if y > 0 => "positive"
// Single arm in a match expression with optional guard
struct Match_Arm {
  static constexpr std::string_view k_name = "Match_Arm";
  Source_Range span{};
  Pattern pattern;                             // Pattern to match against
  std::optional<std::shared_ptr<Expr>> guard;  // Optional guard condition (if guard_expr)
  std::shared_ptr<Expr> result;                // Expression to evaluate if pattern matches
};

// Example: match value { 0 => "zero", n if n > 0 => "positive", _ => "other" }
// Pattern matching expression with exhaustive case analysis
struct Match_Expr {
  static constexpr std::string_view k_name = "Match_Expr";
  Source_Range span{};
  std::shared_ptr<Expr> scrutinee;  // Expression to match against
  std::vector<Match_Arm> arms;      // Match arms (pattern => result)
};

// ============================================================================
// Function Types
// ============================================================================

// Example: items: Std.Array<T> or mut self: Point or self (type optional for self in impl blocks)
struct Func_Param {
  static constexpr std::string_view k_name = "Func_Param";
  Source_Range span{};
  bool is_mut{false};
  std::string name;
  std::optional<Type_Name> type;  // Optional for self parameter in impl blocks
};

// Example: fn process(data: Vec<I32>, callback: Fn<I32, Bool>): Result<String>
struct Func_Decl {
  static constexpr std::string_view k_name = "Func_Decl";
  Source_Range span{};
  std::string name;
  std::vector<Type_Param> type_params;  // Generic parameters: <T>, <T: Display>, <T, U: Iterator<T>>
  std::vector<Func_Param> func_params;
  Type_Name return_type;
  std::optional<Where_Clause> where_clause;  // Optional where clause
};

// Example: fn main(args: Std.Array<String>): I32 { Std.print("Hi"); return 0; }
// Example: pub fn distance(self): F64 { ... } in impl block
struct Func_Def {
  static constexpr std::string_view k_name = "Func_Def";
  Source_Range span{};
  bool is_pub{false};  // true if prefixed with 'pub' (for impl methods)
  Func_Decl declaration;
  Block body;
};

// ============================================================================
// Struct Types
// ============================================================================

// Example: pub x: I32 or y: I32 in struct definition
struct Struct_Field {
  static constexpr std::string_view k_name = "Struct_Field";
  Source_Range span{};
  bool is_pub{false};  // true if prefixed with 'pub'
  std::string name;
  Type_Name type;
};

// Example: struct Point { x: I32, y: I32, metadata: Option<String> }
// Example: struct Cache<K, V> where K: Eq + Hash, V: Clone { items: Map<K, V> }
struct Struct_Def {
  static constexpr std::string_view k_name = "Struct_Def";
  Source_Range span{};
  std::string name;
  std::vector<Type_Param> type_params;  // Generic parameters: <T>, <T: Display>, <K, V: Eq>
  std::vector<Struct_Field> fields;
  std::optional<Where_Clause> where_clause;  // Optional where clause
};

// ============================================================================
// Enum Types
// ============================================================================

// Unit variant: Red, None, False
struct Unit_Variant {
  static constexpr std::string_view k_name = "Unit_Variant";
  Source_Range span{};
  std::string name;  // Variant name (must be Camel_Snake_Case)
};

// Tuple variant: Some(T), Rgb(I32, I32, I32)
struct Tuple_Variant {
  static constexpr std::string_view k_name = "Tuple_Variant";
  Source_Range span{};
  std::string name;                     // Variant name (must be Camel_Snake_Case)
  std::vector<Type_Name> tuple_fields;  // Positional field types
};

// Struct variant: Point { x: I32, y: I32 }
struct Struct_Variant {
  static constexpr std::string_view k_name = "Struct_Variant";
  Source_Range span{};
  std::string name;                         // Variant name (must be Camel_Snake_Case)
  std::vector<Struct_Field> struct_fields;  // Named fields
};

// Example: Some(value), None, Red, Rgb(255, 0, 0), Point { x, y }
// Represents a single variant in an enum definition
struct Enum_Variant : std::variant<Unit_Variant, Tuple_Variant, Struct_Variant> {
  static constexpr std::string_view k_name = "Enum_Variant";
  using Base_Type = std::variant<Unit_Variant, Tuple_Variant, Struct_Variant>;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: enum Option<T> { Some(T), None }
// Example: enum Color { Red, Green, Blue, Rgb(I32, I32, I32) }
// Example: enum Result<T, E> { Ok(T), Err(E) }
struct Enum_Def {
  static constexpr std::string_view k_name = "Enum_Def";
  Source_Range span{};
  std::string name;                          // Enum name (must be Camel_Snake_Case)
  std::vector<Type_Param> type_params;       // Generic parameters: <T>, <T: Display>, <T, E>
  std::vector<Enum_Variant> variants;        // List of variants
  std::optional<Where_Clause> where_clause;  // Optional where clause
};

// ============================================================================
// Impl Blocks
// ============================================================================

// Example: impl Point { fn distance(self): F64 { ... } }  // self type optional in impl
// Example: impl<T> Array<T> { fn len(self): I32 { ... } }  // self type inferred as Array<T>
struct Impl_Block {
  static constexpr std::string_view k_name = "Impl_Block";
  Source_Range span{};
  Type_Name type_name;                       // Type being implemented (e.g., Point, Array<T>)
  std::vector<Type_Param> type_params;       // Generic parameters: <T>, <T: Display>, <K, V>
  std::vector<Func_Def> methods;             // Methods in the impl block
  std::optional<Where_Clause> where_clause;  // Optional where clause
};

// ============================================================================
// Trait Types
// ============================================================================

// Associated type declaration within a trait
// Example: type Item; (in trait Iterator)
// Example: type Item: Display; (with bound)
struct Assoc_Type_Decl {
  static constexpr std::string_view k_name = "Assoc_Type_Decl";
  Source_Range span{};
  std::string name;                 // Associated type name (e.g., Item, Output)
  std::vector<Trait_Bound> bounds;  // Optional trait bounds (e.g., Display, Clone + Send)
};

// Example: type Item = I32; (in impl Iterator for Vec)
// Example: type Output = String; (in impl Transformer for Converter)
struct Assoc_Type_Impl {
  static constexpr std::string_view k_name = "Assoc_Type_Impl";
  Source_Range span{};
  std::string name;      // Associated type name (e.g., Item, Output)
  Type_Name type_value;  // Concrete type assigned (e.g., I32, String)
};

// Example: trait Display { fn to_string(self): String; }
// Example: trait Iterator { type Item; fn next(mut self): Option<Item>; }
struct Trait_Def {
  static constexpr std::string_view k_name = "Trait_Def";
  Source_Range span{};
  std::string name;                          // Trait name (e.g., Display, Iterator)
  std::vector<Type_Param> type_params;       // Generic parameters: <T>, <T: Display>, <K, V>
  std::vector<Assoc_Type_Decl> assoc_types;  // Associated type declarations: type Item, type Output
  std::vector<Func_Decl> methods;            // Method signatures in the trait
  std::optional<Where_Clause> where_clause;  // Optional where clause
};

// Example: impl Display for Point { fn to_string(self): String { ... } }
// Example: impl<T> Iterator<T> for Array<T> where T: Display { type Item = T; ... }
struct Trait_Impl {
  static constexpr std::string_view k_name = "Trait_Impl";
  Source_Range span{};
  Type_Name trait_name;                           // Trait being implemented (e.g., Display, Iterator<T>)
  Type_Name type_name;                            // Type implementing the trait (e.g., Point, Array<T>)
  std::vector<Type_Param> type_params;            // Generic parameters: <T>, <T: Display>, <K, V>
  std::vector<Assoc_Type_Impl> assoc_type_impls;  // Associated type implementations: type Item = T
  std::vector<Func_Def> methods;                  // Method implementations
  std::optional<Where_Clause> where_clause;       // Optional where clause
};

// ============================================================================
// Type Alias
// ============================================================================

// Example: type String_Map<T> = Map<String, T>;
// Example: type Result<T> = Result<T, Error>;
// Example: type Handler = fn(I32): Bool;
struct Type_Alias {
  static constexpr std::string_view k_name = "Type_Alias";
  Source_Range span{};
  std::string name;                     // Alias name (must be Camel_Snake_Case)
  std::vector<Type_Param> type_params;  // Generic parameters: <T>, <K, V>
  Type_Name aliased_type;               // The type being aliased
};

// ============================================================================
// Module Types
// ============================================================================

// Import statement: import Module.Path.{Item1, Item2 as Alias};
// Example: import Geometry.{Point, Circle};
// Example: import Geometry.{Point as P, Circle as C};
// Example: import Geometry.Shapes.{Polygon, Triangle as Tri};
struct Import_Item {
  static constexpr std::string_view k_name = "Import_Item";
  Source_Range span{};
  std::string name;                  // Original name in the module
  std::optional<std::string> alias;  // Optional alias (if 'as' used)
};

struct Import_Statement {
  static constexpr std::string_view k_name = "Import_Statement";
  Source_Range span{};
  std::vector<std::string> module_path;  // ["Geometry", "Shapes"]
  std::vector<Import_Item> items;        // [{"Point", "P"}, {"Circle", std::nullopt}]
};

// Item wrapper that includes visibility
// Example: pub struct Point { ... }
// Example: fn helper() { ... } (no pub = module-internal)
struct Item {
  static constexpr std::string_view k_name = "Item";
  Source_Range span{};
  bool is_pub{};   // true if prefixed with 'pub'
  Statement item;  // The actual item (func_def, struct_def, etc.)
};

// Example: Top-level container with imports and items
struct Module {
  static constexpr std::string_view k_name = "Module";
  Source_Range span{};
  std::vector<Import_Statement> imports;  // Import statements
  std::vector<Item> items;                // Top-level items (functions, structs, etc.)
};

}  // namespace life_lang::ast

#endif  // LIFE_LANG_AST_HPP
