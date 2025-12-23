#ifndef LIFE_LANG_AST_HPP
#define LIFE_LANG_AST_HPP

#include <concepts>
#include <memory>
#include <optional>
#include <variant>

#include <string>
#include <utility>
#include <vector>

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
struct Assignment_Expr;
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
  std::vector<std::shared_ptr<Type_Name>> param_types;  // Parameter types
  std::shared_ptr<Type_Name> return_type;               // Return type
};

// Path-based type name: Std.Map<String, I32>
// This is the original Type_Name functionality extracted into its own struct
struct Path_Type {
  static constexpr std::string_view k_name = "Path_Type";
  std::vector<Type_Name_Segment> segments;
};

// Array type: [T; N]
// Examples: [I32; 4], [String; 10], [Vec<I32>; 3]
struct Array_Type {
  static constexpr std::string_view k_name = "Array_Type";
  std::shared_ptr<Type_Name> element_type;  // Element type
  std::string size;                         // Array size (stored as string to preserve literal)
};

// Tuple type: (T, U, V, ...)
// Examples: (I32, String), (Bool, I32, I32), ((I32, I32), String)
// Note: Empty tuple () is represented as Path_Type with value "()", not Tuple_Type
// Note: Single-element tuple (T,) is represented as Tuple_Type with one element
struct Tuple_Type {
  static constexpr std::string_view k_name = "Tuple_Type";
  std::vector<Type_Name> element_types;  // Element types (must have at least 1)
};

// Type name: either a path-based type, function type, array type, or tuple type
// Examples: I32, Vec<T>, Std.String, fn(I32): Bool, [I32; 4], (I32, String)
struct Type_Name : std::variant<Path_Type, Function_Type, Array_Type, Tuple_Type> {
  static constexpr std::string_view k_name = "Type_Name";
  using Base_Type = std::variant<Path_Type, Function_Type, Array_Type, Tuple_Type>;
  using Base_Type::Base_Type;  // NOLINT(modernize-use-equals-default,hicpp-use-equals-default)
  using Base_Type::operator=;

  // Helper to access segments when Type_Name holds a Path_Type
  // Throws std::bad_variant_access if Type_Name is not a Path_Type
  [[nodiscard]] std::vector<Type_Name_Segment> const& segments() const { return std::get<Path_Type>(*this).segments; }

  [[nodiscard]] std::vector<Type_Name_Segment>& segments() { return std::get<Path_Type>(*this).segments; }
};

// Example: Map<String, I32> where "Map" is value, type_params = [String, I32]
struct Type_Name_Segment {
  static constexpr std::string_view k_name = "Type_Name_Segment";
  std::string value;
  std::vector<Type_Name> type_params;
};

// ============================================================================
// Trait Bounds (for generic constraints)
// ============================================================================

// Example: Display (in T: Display)
struct Trait_Bound {
  static constexpr std::string_view k_name = "Trait_Bound";
  Type_Name trait_name;  // The trait being required (e.g., Display, Iterator<T>)
};

// Type parameter with optional inline trait bounds (in angle brackets)
// Inline bounds are limited to simple type parameters (T, U, Key, etc.)
// Example: T (no bounds), T: Display (single bound), T: Display + Clone (multiple bounds)
struct Type_Param {
  static constexpr std::string_view k_name = "Type_Param";
  Type_Name name;                   // Parameter name (e.g., T, U, Item) - always a simple identifier
  std::vector<Trait_Bound> bounds;  // Optional trait bounds (e.g., Display, Display + Clone)
};

// Where clause predicate: type constraint in where clause
// Where predicates support more complex type expressions than inline bounds:
// - Simple type parameters: T: Display
// - Associated types (future): T::Item: Display, <T as Iterator>::Item: Clone
// Example: T: Display + Clone (in "where T: Display + Clone")
struct Where_Predicate {
  static constexpr std::string_view k_name = "Where_Predicate";
  Type_Name type_name;              // Type being constrained (e.g., T, U::Item, <T as Iterator>::Item)
  std::vector<Trait_Bound> bounds;  // Required trait bounds
};

// Where clause: collection of predicates
// Where clauses enable complex constraints not expressible with inline bounds
// Example: where T: Display, U: Clone, V: Eq + Ord
struct Where_Clause {
  static constexpr std::string_view k_name = "Where_Clause";
  std::vector<Where_Predicate> predicates;
};

// ============================================================================
// Variable Name System (for variables and function names)
// ============================================================================

// Example: Std.IO.println (qualified function name) or my_var (simple variable)
struct Var_Name {
  static constexpr std::string_view k_name = "Var_Name";
  std::vector<Var_Name_Segment> segments;
};

// Example: println<T> where "println" is value, type_params = [T]
struct Var_Name_Segment {
  static constexpr std::string_view k_name = "Var_Name_Segment";
  std::string value;
  std::vector<Type_Name> type_params;
};

// ============================================================================
// Literal Types
// ============================================================================

// Example: "Hello, world!" stored with quotes as "\"Hello, world!\""
struct String {
  static constexpr std::string_view k_name = "String";
  std::string value;
};

// String interpolation part: either a literal string segment or an expression
// Example: "result: {x + 1}" has parts: ["result: ", <expr: x+1>, ""]
struct String_Interp_Part : std::variant<std::string, std::shared_ptr<Expr>> {
  using Base_Type = std::variant<std::string, std::shared_ptr<Expr>>;
  String_Interp_Part() = default;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
  static constexpr std::string_view k_name = "String_Interp_Part";
};

// String interpolation: "Hello, {name}! You are {age} years old."
// Represented as alternating string literals and expressions
// Example: ["Hello, ", <name>, "! You are ", <age>, " years old."]
struct String_Interpolation {
  static constexpr std::string_view k_name = "String_Interpolation";
  std::vector<String_Interp_Part> parts;
};

// Example: 42 or 0x2A or 0b101010 (stored as string for arbitrary precision)
// Optional suffix: I8, I16, I32, I64, U8, U16, U32, U64
struct Integer {
  static constexpr std::string_view k_name = "Integer";
  std::string value;
  std::optional<std::string> suffix;  // Type suffix like "I32", "U64", etc.
};

// Example: 3.14 or 1.0e-10 or 2.5E+3 (stored as string for arbitrary precision)
// Optional suffix: F32, F64
struct Float {
  static constexpr std::string_view k_name = "Float";
  std::string value;
  std::optional<std::string> suffix;  // Type suffix like "F32", "F64"
};

// Example: 'a' or '\n' or '世' (stored with quotes as "'a'")
struct Char {
  static constexpr std::string_view k_name = "Char";
  std::string value;
};

// Boolean literal: true or false
struct Bool_Literal {
  static constexpr std::string_view k_name = "Bool_Literal";
  bool value{};
};

// Unit literal: () - represents "no value" or empty tuple
// Used in return statements for functions with unit return type: return ();
struct Unit_Literal {
  static constexpr std::string_view k_name = "Unit_Literal";
};

// ============================================================================
// Struct Literal Types (for initialization)
// ============================================================================

// Example: x: 10 in struct literal Point { x: 10, y: 20 }
struct Field_Initializer {
  static constexpr std::string_view k_name = "Field_Initializer";
  std::string name;
  std::shared_ptr<Expr> value;
};

// Example: Point { x: offset.x + 5, y: base.calculate() }
struct Struct_Literal {
  static constexpr std::string_view k_name = "Struct_Literal";
  std::string type_name;
  std::vector<Field_Initializer> fields;
};

// Array literal: [expr, expr, ...]
// Examples: [1, 2, 3], [x, y + 1, calculate()], [] (empty array)
struct Array_Literal {
  static constexpr std::string_view k_name = "Array_Literal";
  std::vector<Expr> elements;
};

// Tuple literal: (expr, expr, ...)
// Examples: (1, 2), ("name", 42, true), (x, y + 1)
// Note: Single element requires trailing comma: (x,) - otherwise it's a parenthesized expression
// Note: Empty tuple () is Unit_Literal, not Tuple_Literal
struct Tuple_Literal {
  static constexpr std::string_view k_name = "Tuple_Literal";
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
  Unary_Op op{};
  std::shared_ptr<Expr> operand;
};

// Range expression: start..end (exclusive) or start..=end (inclusive)
// Examples: 0..10, start..end, 1..=100
struct Range_Expr {
  static constexpr std::string_view k_name = "Range_Expr";
  std::optional<std::shared_ptr<Expr>> start;  // None for unbounded start (..)
  std::optional<std::shared_ptr<Expr>> end;    // None for unbounded end (a..)
  bool inclusive{};                            // false for .., true for ..=
};

// Type cast expression: expr as Type
// Examples: x as I64, (y + 1) as F32, ptr as U64
// Performs explicit type conversion (validity checked in semantic analysis)
struct Cast_Expr {
  static constexpr std::string_view k_name = "Cast_Expr";
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
                  std::shared_ptr<Assignment_Expr>,
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
      std::shared_ptr<Assignment_Expr>,
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
  Expr() = default;
  using Base_Type::Base_Type;  // NOLINT(modernize-use-equals-default,hicpp-use-equals-default)
  using Base_Type::operator=;
};

// Example: Std.print("Value: ", x, y + 2)
struct Func_Call_Expr {
  static constexpr std::string_view k_name = "Func_Call_Expr";
  Var_Name name;
  std::vector<Expr> params;
};

// Example: point.x or nested.obj.field (chained via recursive object field)
struct Field_Access_Expr {
  static constexpr std::string_view k_name = "Field_Access_Expr";
  std::shared_ptr<Expr> object;
  std::string field_name;
};

// Index expression: array[index]
// Examples: arr[0], matrix[i][j], get_array()[x + 1]
struct Index_Expr {
  static constexpr std::string_view k_name = "Index_Expr";
  std::shared_ptr<Expr> object;  // The array/indexable expression
  std::shared_ptr<Expr> index;   // The index expression
};

// Example: x = 42 or point.x = 10 or arr[i] = value (future)
// Assignment requires target to be mutable (checked in semantic analysis)
struct Assignment_Expr {
  static constexpr std::string_view k_name = "Assignment_Expr";
  std::shared_ptr<Expr> target;  // LHS: variable or field access
  std::shared_ptr<Expr> value;   // RHS: expression to assign
};

// ============================================================================
// Statement Types
// ============================================================================

// Example: Std.print("Hello"); as a standalone statement (not an expression)
struct Func_Call_Statement {
  static constexpr std::string_view k_name = "Func_Call_Statement";
  Func_Call_Expr expr;
};

// Example: x = 42;, y = y + 1;, foo();
// Statement form of any expression - evaluates expression and discards result
// Useful for assignments, function calls, or other expressions with side effects
struct Expr_Statement {
  static constexpr std::string_view k_name = "Expr_Statement";
  std::shared_ptr<Expr> expr;
};

// Example: return calculate(x + y, Point { a: 1, b: 2 });
struct Return_Statement {
  static constexpr std::string_view k_name = "Return_Statement";
  Expr expr;
};

// Example: break; or break result_value;
// Used to exit loops early, optionally returning a value
struct Break_Statement {
  static constexpr std::string_view k_name = "Break_Statement";
  std::optional<Expr> value;  // Optional: break can be used without value
};

// Example: continue;
// Skips to next iteration of the loop
struct Continue_Statement {
  static constexpr std::string_view k_name = "Continue_Statement";
};

// If statement wrapper for using if expressions as statements
// When if is used for side effects (not in expression context), no semicolon needed
// Example: if condition { do_something(); }
struct If_Statement {
  static constexpr std::string_view k_name = "If_Statement";
  std::shared_ptr<If_Expr> expr;
};

// While statement wrapper for using while expressions as statements
// Example: while x < 10 { process(x); }
struct While_Statement {
  static constexpr std::string_view k_name = "While_Statement";
  std::shared_ptr<While_Expr> expr;
};

// For statement wrapper for using for expressions as statements
// Example: for item in 0..10 { process(item); }
struct For_Statement {
  static constexpr std::string_view k_name = "For_Statement";
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
      Func_Call_Statement,
      std::shared_ptr<Expr_Statement>,
      Return_Statement,
      Break_Statement,
      Continue_Statement,
      std::shared_ptr<If_Statement>,
      std::shared_ptr<While_Statement>,
      std::shared_ptr<For_Statement>,
      std::shared_ptr<Block>>;
  Statement() = default;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: { Std.print(x); { nested(); } return 0; }
struct Block {
  static constexpr std::string_view k_name = "Block";
  std::vector<Statement> statements;
  std::optional<std::shared_ptr<Expr>> trailing_expr;  // Optional trailing expression
};

// Example: if x > 0 { x } else if x < 0 { -x } else { 0 }
// Chain structure: condition + then_block, plus optional else_ifs and final else_block
struct Else_If_Clause {
  static constexpr std::string_view k_name = "Else_If_Clause";
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Block> then_block;
};

struct If_Expr {
  static constexpr std::string_view k_name = "If_Expr";
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Block> then_block;
  std::vector<Else_If_Clause> else_ifs;
  // std::optional to avoid use-after-free issues caused by std::optional
  std::optional<std::shared_ptr<Block>> else_block;
};

// ============================================================================
// Pattern Matching Types
// ============================================================================

// Example: item (simple variable binding in for loops)
// Wildcard pattern: _ (matches anything, doesn't bind)
struct Wildcard_Pattern {
  static constexpr std::string_view k_name = "Wildcard_Pattern";
};

// Literal pattern: 42, 3.14, "hello" (matches exact value)
struct Literal_Pattern {
  static constexpr std::string_view k_name = "Literal_Pattern";
  std::shared_ptr<Expr> value;  // Integer, Float, or String literal
};

struct Simple_Pattern {
  static constexpr std::string_view k_name = "Simple_Pattern";
  std::string name;
};

// Example: x: 3 in pattern Point { x: 3, y: 4 }
struct Field_Pattern {
  static constexpr std::string_view k_name = "Field_Pattern";
  std::string name;
  std::shared_ptr<Pattern> pattern;
};

// Example: Point { x: 3, y: 4 } (destructure struct fields in match expressions)
// Supports nesting: Point { x: 3, inner: Line { a: 1, b: 2 } } where fields have patterns
struct Struct_Pattern {
  static constexpr std::string_view k_name = "Struct_Pattern";
  Type_Name type_name;
  std::vector<Field_Pattern> fields;
  bool has_rest = false;  // true if pattern contains .. to ignore remaining fields
};

// Example: (a, b, c) (destructure tuple elements in for loops)
// Supports nesting: (a, (b, c)) where elements are patterns
struct Tuple_Pattern {
  static constexpr std::string_view k_name = "Tuple_Pattern";
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
  Pattern() = default;
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
  std::shared_ptr<Expr> condition;
  std::shared_ptr<Block> body;
};

// Example: for item in 0..10 { process(item); } or for (a, b) in pairs { }
// Iterates over collection or range with pattern matching
struct For_Expr {
  static constexpr std::string_view k_name = "For_Expr";
  Pattern pattern;                 // Pattern for destructuring (simple, struct, or tuple)
  std::shared_ptr<Expr> iterator;  // Collection or range expression
  std::shared_ptr<Block> body;
};

// Example: Point { x: 0, y } if y > 0 => "positive"
// Single arm in a match expression with optional guard
struct Match_Arm {
  static constexpr std::string_view k_name = "Match_Arm";
  Pattern pattern;                             // Pattern to match against
  std::optional<std::shared_ptr<Expr>> guard;  // Optional guard condition (if guard_expr)
  std::shared_ptr<Expr> result;                // Expression to evaluate if pattern matches
};

// Example: match value { 0 => "zero", n if n > 0 => "positive", _ => "other" }
// Pattern matching expression with exhaustive case analysis
struct Match_Expr {
  static constexpr std::string_view k_name = "Match_Expr";
  std::shared_ptr<Expr> scrutinee;  // Expression to match against
  std::vector<Match_Arm> arms;      // Match arms (pattern => result)
};

// ============================================================================
// Function Types
// ============================================================================

// Example: items: Std.Array<T> or mut self: Point or self (type optional for self in impl blocks)
struct Func_Param {
  static constexpr std::string_view k_name = "Func_Param";
  bool is_mut{false};
  std::string name;
  std::optional<Type_Name> type;  // Optional for self parameter in impl blocks
};

// Example: fn process(data: Vec<I32>, callback: Fn<I32, Bool>): Result<String>
struct Func_Decl {
  static constexpr std::string_view k_name = "Func_Decl";
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
  bool is_pub{false};  // true if prefixed with 'pub'
  std::string name;
  Type_Name type;
};

// Example: struct Point { x: I32, y: I32, metadata: Option<String> }
// Example: struct Box<T> { value: T }
struct Struct_Def {
  static constexpr std::string_view k_name = "Struct_Def";
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
  std::string name;  // Variant name (must be Camel_Snake_Case)
};

// Tuple variant: Some(T), Rgb(I32, I32, I32)
struct Tuple_Variant {
  static constexpr std::string_view k_name = "Tuple_Variant";
  std::string name;                     // Variant name (must be Camel_Snake_Case)
  std::vector<Type_Name> tuple_fields;  // Positional field types
};

// Struct variant: Point { x: I32, y: I32 }
struct Struct_Variant {
  static constexpr std::string_view k_name = "Struct_Variant";
  std::string name;                         // Variant name (must be Camel_Snake_Case)
  std::vector<Struct_Field> struct_fields;  // Named fields
};

// Example: Some(value), None, Red, Rgb(255, 0, 0), Point { x, y }
// Represents a single variant in an enum definition
struct Enum_Variant : std::variant<Unit_Variant, Tuple_Variant, Struct_Variant> {
  static constexpr std::string_view k_name = "Enum_Variant";
  using Base_Type = std::variant<Unit_Variant, Tuple_Variant, Struct_Variant>;
  Enum_Variant() = default;
  using Base_Type::Base_Type;
  using Base_Type::operator=;
};

// Example: enum Option<T> { Some(T), None }
// Example: enum Color { Red, Green, Blue, Rgb(I32, I32, I32) }
// Example: enum Result<T, E> { Ok(T), Err(E) }
struct Enum_Def {
  static constexpr std::string_view k_name = "Enum_Def";
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
  std::string name;                 // Associated type name (e.g., Item, Output)
  std::vector<Trait_Bound> bounds;  // Optional trait bounds (e.g., Display, Clone + Send)
};

// Example: type Item = I32; (in impl Iterator for Vec)
// Example: type Output = String; (in impl Transformer for Converter)
struct Assoc_Type_Impl {
  static constexpr std::string_view k_name = "Assoc_Type_Impl";
  std::string name;      // Associated type name (e.g., Item, Output)
  Type_Name type_value;  // Concrete type assigned (e.g., I32, String)
};

// Example: trait Display { fn to_string(self): String; }
// Example: trait Iterator { type Item; fn next(mut self): Option<Item>; }
struct Trait_Def {
  static constexpr std::string_view k_name = "Trait_Def";
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
  std::string name;                  // Original name in the module
  std::optional<std::string> alias;  // Optional alias (if 'as' used)
};

struct Import_Statement {
  static constexpr std::string_view k_name = "Import_Statement";
  std::vector<std::string> module_path;  // ["Geometry", "Shapes"]
  std::vector<Import_Item> items;        // [{"Point", "P"}, {"Circle", std::nullopt}]
};

// Item wrapper that includes visibility
// Example: pub struct Point { ... }
// Example: fn helper() { ... } (no pub = module-internal)
struct Item {
  static constexpr std::string_view k_name = "Item";
  bool is_pub{};   // true if prefixed with 'pub'
  Statement item;  // The actual item (func_def, struct_def, etc.)
};

// Example: Top-level container with imports and items
struct Module {
  static constexpr std::string_view k_name = "Module";
  std::vector<Import_Statement> imports;  // Import statements
  std::vector<Item> items;                // Top-level items (functions, structs, etc.)
};

// ============================================================================
// Helper Functions for AST Construction
// ============================================================================

// Trait_Bound helpers
inline Trait_Bound make_trait_bound(Type_Name&& trait_name_) {
  return Trait_Bound{std::move(trait_name_)};
}

// Type_Param helpers
inline Type_Param make_type_param(Type_Name&& name_, std::vector<Trait_Bound>&& bounds_) {
  return Type_Param{.name = std::move(name_), .bounds = std::move(bounds_)};
}

// Where_Predicate helpers
inline Where_Predicate make_where_predicate(Type_Name&& type_name_, std::vector<Trait_Bound>&& bounds_) {
  return Where_Predicate{.type_name = std::move(type_name_), .bounds = std::move(bounds_)};
}

// Where_Clause helpers
inline Where_Clause make_where_clause(std::vector<Where_Predicate>&& predicates_) {
  return Where_Clause{std::move(predicates_)};
}

// Type_Name helpers
inline Type_Name_Segment make_type_name_segment(std::string&& value_, std::vector<Type_Name>&& type_params_) {
  return Type_Name_Segment{.value = std::move(value_), .type_params = std::move(type_params_)};
}

// Path_Type helpers
inline Path_Type make_path_type(std::vector<Type_Name_Segment>&& segments_) {
  return Path_Type{std::move(segments_)};
}

// Variadic make_path_type for convenient construction
// Accepts string arguments and/or Type_Name_Segment arguments
// Examples:
//   make_path_type("Int")                                    -> Path_Type with single segment "Int"
//   make_path_type("Std", "String")                          -> Path_Type with segments ["Std", "String"]
//   make_path_type("Vec", make_type_name_segment("T", {...})) -> Path_Type with templated segment
template <typename... Args>
  requires(
      (std::same_as<std::remove_cvref_t<Args>, Type_Name_Segment> || std::constructible_from<std::string, Args>) && ...
  )
inline Path_Type make_path_type(Args&&... args_) {
  if constexpr (sizeof...(args_) == 0) {
    return Path_Type{{}};
  } else {
    std::vector<Type_Name_Segment> segments;
    segments.reserve(sizeof...(args_));
    (
        [&] {
          if constexpr (std::same_as<std::remove_cvref_t<Args>, Type_Name_Segment>) {
            segments.push_back(std::forward<Args>(args_));
          } else {
            segments.push_back(make_type_name_segment(std::string(std::forward<Args>(args_)), {}));
          }
        }(),
        ...
    );
    return Path_Type{std::move(segments)};
  }
}

// Array_Type helper
inline Array_Type make_array_type(Type_Name&& element_type_, std::string&& size_) {
  return Array_Type{.element_type = std::make_shared<Type_Name>(std::move(element_type_)), .size = std::move(size_)};
}

inline Tuple_Type make_tuple_type(std::vector<Type_Name>&& element_types_) {
  return Tuple_Type{.element_types = std::move(element_types_)};
}

inline Type_Name make_type_name(std::vector<Type_Name_Segment>&& segments_) {
  return Type_Name(Path_Type{std::move(segments_)});
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
inline Type_Name make_type_name(Args&&... args_) {
  if constexpr (sizeof...(args_) == 0) {
    return Type_Name(Path_Type{{}});
  } else {
    std::vector<Type_Name_Segment> segments;
    segments.reserve(sizeof...(args_));
    (
        [&] {
          if constexpr (std::same_as<std::remove_cvref_t<Args>, Type_Name_Segment>) {
            segments.push_back(std::forward<Args>(args_));
          } else {
            segments.push_back(make_type_name_segment(std::string(std::forward<Args>(args_)), {}));
          }
        }(),
        ...
    );
    return Type_Name(Path_Type{std::move(segments)});
  }
}

// Function_Type helpers
inline Function_Type make_function_type(std::vector<Type_Name> param_types_, Type_Name&& return_type_) {
  // Convert Type_Name vector to shared_ptr vector
  std::vector<std::shared_ptr<Type_Name>> param_types;
  param_types.reserve(param_types_.size());
  for (auto& param: param_types_) {
    param_types.emplace_back(std::make_shared<Type_Name>(std::move(param)));
  }
  return Function_Type{
      .param_types = std::move(param_types),
      .return_type = std::make_shared<Type_Name>(std::move(return_type_))
  };
}

// Var_Name helpers
inline Var_Name_Segment make_var_name_segment(std::string&& value_, std::vector<Type_Name>&& type_params_) {
  return Var_Name_Segment{.value = std::move(value_), .type_params = std::move(type_params_)};
}

inline Var_Name make_var_name(std::vector<Var_Name_Segment>&& segments_) {
  return Var_Name{std::move(segments_)};
}

template <typename... Args>
  requires(
      (std::same_as<std::remove_cvref_t<Args>, Var_Name_Segment> || std::constructible_from<std::string, Args>) && ...
  )
inline Var_Name make_var_name(Args&&... args_) {
  if constexpr (sizeof...(args_) == 0) {
    return Var_Name{{}};
  } else {
    std::vector<Var_Name_Segment> segments;
    segments.reserve(sizeof...(args_));
    (
        [&] {
          if constexpr (std::same_as<std::remove_cvref_t<Args>, Var_Name_Segment>) {
            segments.push_back(std::forward<Args>(args_));
          } else {
            segments.push_back(make_var_name_segment(std::string(std::forward<Args>(args_)), {}));
          }
        }(),
        ...
    );
    return Var_Name{std::move(segments)};
  }
}

// Literal helpers
inline String make_string(std::string&& value_) {
  return String{std::move(value_)};
}

inline Integer make_integer(std::string value_, std::optional<std::string> suffix_ = std::nullopt) noexcept {
  return Integer{.value = std::move(value_), .suffix = std::move(suffix_)};
}

inline Float make_float(std::string value_, std::optional<std::string> suffix_ = std::nullopt) noexcept {
  return Float{.value = std::move(value_), .suffix = std::move(suffix_)};
}

inline Unit_Literal make_unit_literal() noexcept {
  return Unit_Literal{};
}

inline Bool_Literal make_bool_literal(bool value_) noexcept {
  return Bool_Literal{value_};
}

inline Char make_char(std::string&& value_) {
  return Char{std::move(value_)};
}

// Struct literal helpers
inline Field_Initializer make_field_initializer(std::string&& name_, Expr&& value_) {
  return Field_Initializer{.name = std::move(name_), .value = std::make_shared<Expr>(std::move(value_))};
}

inline Struct_Literal make_struct_literal(std::string&& type_name_, std::vector<Field_Initializer>&& fields_) {
  return Struct_Literal{.type_name = std::move(type_name_), .fields = std::move(fields_)};
}

// Expression helpers
inline Expr make_expr(Var_Name&& name_) {
  return Expr{std::move(name_)};
}
inline Expr make_expr(String&& str_) {
  return Expr{std::move(str_)};
}
inline Expr make_expr(Integer&& integer_) noexcept {
  return Expr{std::move(integer_)};
}
inline Expr make_expr(Float&& float_) noexcept {
  return Expr{std::move(float_)};
}
inline Expr make_expr(Char&& char_) {
  return Expr{std::move(char_)};
}
inline Expr make_expr(Func_Call_Expr&& call_) {
  return Expr{std::make_shared<Func_Call_Expr>(std::move(call_))};
}
inline Expr make_expr(Field_Access_Expr&& access_) {
  return Expr{std::make_shared<Field_Access_Expr>(std::move(access_))};
}
inline Expr make_expr(Assignment_Expr&& assignment_) {
  return Expr{std::make_shared<Assignment_Expr>(std::move(assignment_))};
}
inline Expr make_expr(Binary_Expr&& binary_) {
  return Expr{std::make_shared<Binary_Expr>(std::move(binary_))};
}
inline Expr make_expr(Unary_Expr&& unary_) {
  return Expr{std::make_shared<Unary_Expr>(std::move(unary_))};
}
inline Expr make_expr(If_Expr&& if_) {
  return Expr{std::make_shared<If_Expr>(std::move(if_))};
}
inline Expr make_expr(While_Expr&& while_) {
  return Expr{std::make_shared<While_Expr>(std::move(while_))};
}
inline Expr make_expr(For_Expr&& for_) {
  return Expr{std::make_shared<For_Expr>(std::move(for_))};
}
inline Expr make_expr(Match_Expr&& match_) {
  return Expr{std::make_shared<Match_Expr>(std::move(match_))};
}
inline Expr make_expr(Block&& block_) {
  return Expr{std::make_shared<Block>(std::move(block_))};
}
inline Expr make_expr(Range_Expr&& range_) {
  return Expr{std::make_shared<Range_Expr>(std::move(range_))};
}
inline Expr make_expr(Struct_Literal&& literal_) {
  return Expr{std::move(literal_)};
}
inline Expr make_expr(Array_Literal&& literal_) {
  return Expr{std::move(literal_)};
}
inline Expr make_expr(Unit_Literal const& unit_) {
  return Expr{unit_};
}

inline Func_Call_Expr make_func_call_expr(Var_Name&& name_, std::vector<Expr>&& params_) {
  return Func_Call_Expr{.name = std::move(name_), .params = std::move(params_)};
}

inline Field_Access_Expr make_field_access_expr(Expr&& object_, std::string&& field_name_) {
  return Field_Access_Expr{.object = std::make_shared<Expr>(std::move(object_)), .field_name = std::move(field_name_)};
}

inline Array_Literal make_array_literal(std::vector<Expr>&& elements_) {
  return Array_Literal{.elements = std::move(elements_)};
}

inline Tuple_Literal make_tuple_literal(std::vector<Expr>&& elements_) {
  return Tuple_Literal{.elements = std::move(elements_)};
}

inline Index_Expr make_index_expr(Expr&& object_, Expr&& index_) {
  return Index_Expr{
      .object = std::make_shared<Expr>(std::move(object_)),
      .index = std::make_shared<Expr>(std::move(index_))
  };
}

inline Assignment_Expr make_assignment_expr(Expr&& target_, Expr&& value_) {
  return Assignment_Expr{
      .target = std::make_shared<Expr>(std::move(target_)),
      .value = std::make_shared<Expr>(std::move(value_))
  };
}

inline Else_If_Clause make_else_if_clause(Expr&& condition_, Block&& then_block_) {
  return Else_If_Clause{
      .condition = std::make_shared<Expr>(std::move(condition_)),
      .then_block = std::make_shared<Block>(std::move(then_block_))
  };
}

inline If_Expr make_if_expr(
    Expr&& condition_,
    Block&& then_block_,
    std::vector<Else_If_Clause>&& else_ifs_ = {},
    std::optional<Block>&& else_block_ = std::nullopt
) {
  std::optional<std::shared_ptr<Block>> else_block_wrapped;
  if (else_block_.has_value()) {
    else_block_wrapped = std::make_shared<Block>(std::move(std::move(else_block_).value()));
  }
  return If_Expr{
      .condition = std::make_shared<Expr>(std::move(condition_)),
      .then_block = std::make_shared<Block>(std::move(then_block_)),
      .else_ifs = std::move(else_ifs_),
      .else_block = std::move(else_block_wrapped)
  };
}

inline While_Expr make_while_expr(Expr&& condition_, Block&& body_) {
  return While_Expr{
      .condition = std::make_shared<Expr>(std::move(condition_)),
      .body = std::make_shared<Block>(std::move(body_))
  };
}

// Pattern helpers
inline Wildcard_Pattern make_wildcard_pattern() {
  return Wildcard_Pattern{};
}

inline Literal_Pattern make_literal_pattern(Expr&& value_) {
  return Literal_Pattern{std::make_shared<Expr>(std::move(value_))};
}

inline Simple_Pattern make_simple_pattern(std::string&& name_) {
  return Simple_Pattern{std::move(name_)};
}

inline Field_Pattern make_field_pattern(std::string&& name_, Pattern&& pattern_) {
  return Field_Pattern{.name = std::move(name_), .pattern = std::make_shared<Pattern>(std::move(pattern_))};
}

inline Struct_Pattern
make_struct_pattern(Type_Name&& type_name_, std::vector<Field_Pattern>&& fields_, bool has_rest_ = false) {
  return Struct_Pattern{.type_name = std::move(type_name_), .fields = std::move(fields_), .has_rest = has_rest_};
}

inline Tuple_Pattern make_tuple_pattern(std::vector<std::shared_ptr<Pattern>>&& elements_) {
  return Tuple_Pattern{.elements = std::move(elements_)};
}

inline Enum_Pattern make_enum_pattern(Type_Name&& type_name_, std::vector<std::shared_ptr<Pattern>>&& patterns_) {
  return Enum_Pattern{.type_name = std::move(type_name_), .patterns = std::move(patterns_)};
}

inline Or_Pattern make_or_pattern(std::vector<std::shared_ptr<Pattern>>&& alternatives_) {
  return Or_Pattern{.alternatives = std::move(alternatives_)};
}

inline Pattern make_pattern(Wildcard_Pattern const& pattern_) {
  return Pattern{pattern_};
}
inline Pattern make_pattern(Literal_Pattern&& pattern_) {
  return Pattern{std::move(pattern_)};
}
inline Pattern make_pattern(Simple_Pattern&& pattern_) {
  return Pattern{std::move(pattern_)};
}
inline Pattern make_pattern(Struct_Pattern&& pattern_) {
  return Pattern{std::move(pattern_)};
}
inline Pattern make_pattern(Tuple_Pattern&& pattern_) {
  return Pattern{std::move(pattern_)};
}
inline Pattern make_pattern(Enum_Pattern&& pattern_) {
  return Pattern{std::move(pattern_)};
}

inline For_Expr make_for_expr(Pattern&& pattern_, Expr&& iterator_, Block&& body_) {
  return For_Expr{
      .pattern = std::move(pattern_),
      .iterator = std::make_shared<Expr>(std::move(iterator_)),
      .body = std::make_shared<Block>(std::move(body_))
  };
}

inline Match_Arm make_match_arm(Pattern&& pattern_, std::optional<Expr>&& guard_, Expr&& result_) {
  std::optional<std::shared_ptr<Expr>> guard_wrapped;
  if (guard_) {
    guard_wrapped = std::make_shared<Expr>(std::move(std::move(guard_).value()));
  }
  return Match_Arm{
      .pattern = std::move(pattern_),
      .guard = std::move(guard_wrapped),
      .result = std::make_shared<Expr>(std::move(result_))
  };
}

inline Match_Expr make_match_expr(Expr&& scrutinee_, std::vector<Match_Arm>&& arms_) {
  return Match_Expr{.scrutinee = std::make_shared<Expr>(std::move(scrutinee_)), .arms = std::move(arms_)};
}

inline Range_Expr make_range_expr(Expr&& start_, Expr&& end_, bool inclusive_) {
  return Range_Expr{
      .start = std::make_shared<Expr>(std::move(start_)),
      .end = std::make_shared<Expr>(std::move(end_)),
      .inclusive = inclusive_
  };
}

// Statement helpers
inline Func_Call_Statement make_func_call_statement(Func_Call_Expr&& expr_) {
  return Func_Call_Statement{.expr = std::move(expr_)};
}

inline Expr_Statement make_expr_statement(Expr&& expr_) {
  return Expr_Statement{std::make_shared<Expr>(std::move(expr_))};
}

inline Return_Statement make_return_statement(Expr&& expr_) {
  return Return_Statement{std::move(expr_)};
}

inline Break_Statement make_break_statement(std::optional<Expr>&& value_) {
  return Break_Statement{std::move(value_)};
}

inline Continue_Statement make_continue_statement() {
  return Continue_Statement{};
}

inline If_Statement make_if_statement(If_Expr&& expr_) {
  return If_Statement{std::make_shared<If_Expr>(std::move(expr_))};
}

inline While_Statement make_while_statement(While_Expr&& expr_) {
  return While_Statement{std::make_shared<While_Expr>(std::move(expr_))};
}

inline For_Statement make_for_statement(For_Expr&& expr_) {
  return For_Statement{std::make_shared<For_Expr>(std::move(expr_))};
}

inline Let_Statement
make_let_statement(bool is_mut_, Pattern&& pattern_, std::optional<Type_Name>&& type_, Expr&& value_) {
  return Let_Statement{
      .is_mut = is_mut_,
      .pattern = std::move(pattern_),
      .type = std::move(type_),
      .value = std::make_shared<Expr>(std::move(value_))
  };
}

inline Statement make_statement(Func_Call_Statement&& call_) {
  return Statement{std::move(call_)};
}
inline Statement make_statement(Expr_Statement&& expr_) {
  return Statement{std::make_shared<Expr_Statement>(std::move(expr_))};
}
inline Statement make_statement(Return_Statement&& ret_) {
  return Statement{std::move(ret_)};
}
inline Statement make_statement(Let_Statement&& let_) {
  return Statement{std::make_shared<Let_Statement>(std::move(let_))};
}
inline Statement make_statement(Block&& block_) {
  return Statement{std::make_shared<Block>(std::move(block_))};
}
inline Statement make_statement(Func_Def&& def_) {
  return Statement{std::make_shared<Func_Def>(std::move(def_))};
}
inline Statement make_statement(Struct_Def&& def_) {
  return Statement{std::make_shared<Struct_Def>(std::move(def_))};
}
inline Statement make_statement(Trait_Def&& def_) {
  return Statement{std::make_shared<Trait_Def>(std::move(def_))};
}
inline Statement make_statement(Trait_Impl&& impl_) {
  return Statement{std::make_shared<Trait_Impl>(std::move(impl_))};
}

inline Block make_block(std::vector<Statement>&& statements_, std::optional<Expr>&& trailing_expr_ = std::nullopt) {
  std::optional<std::shared_ptr<Expr>> trailing_wrapped;
  if (trailing_expr_.has_value()) {
    trailing_wrapped = std::make_shared<Expr>(std::move(std::move(trailing_expr_).value()));
  }
  return Block{.statements = std::move(statements_), .trailing_expr = std::move(trailing_wrapped)};
}

// Binary expression helper
inline Binary_Expr make_binary_expr(Expr&& lhs_, Binary_Op op_, Expr&& rhs_) {
  return Binary_Expr{
      .lhs = std::make_shared<Expr>(std::move(lhs_)),
      .op = op_,
      .rhs = std::make_shared<Expr>(std::move(rhs_))
  };
}

// Unary expression helper
inline Unary_Expr make_unary_expr(Unary_Op op_, Expr&& operand_) {
  return Unary_Expr{.op = op_, .operand = std::make_shared<Expr>(std::move(operand_))};
}

// Cast expression helper
inline Cast_Expr make_cast_expr(Expr&& expr_, Type_Name&& target_type_) {
  return Cast_Expr{.expr = std::make_shared<Expr>(std::move(expr_)), .target_type = std::move(target_type_)};
}

// Function helpers
inline Func_Param make_func_param(bool is_mut_, std::string&& name_, std::optional<Type_Name>&& type_) {
  return Func_Param{.is_mut = is_mut_, .name = std::move(name_), .type = std::move(type_)};
}

inline Func_Decl make_func_decl(
    std::string&& name_,
    std::vector<Type_Param>&& type_params_,
    std::vector<Func_Param>&& params_,
    Type_Name&& return_type_,
    std::optional<Where_Clause>&& where_clause_ = std::nullopt
) {
  return Func_Decl{
      .name = std::move(name_),
      .type_params = std::move(type_params_),
      .func_params = std::move(params_),
      .return_type = std::move(return_type_),
      .where_clause = std::move(where_clause_)
  };
}

inline Func_Def make_func_def(Func_Decl&& decl_, Block&& body_, bool is_pub_ = false) {
  return Func_Def{.is_pub = is_pub_, .declaration = std::move(decl_), .body = std::move(body_)};
}

// Struct helpers
inline Struct_Field make_struct_field(std::string&& name_, Type_Name&& type_, bool is_pub_ = false) {
  return Struct_Field{.is_pub = is_pub_, .name = std::move(name_), .type = std::move(type_)};
}

inline Struct_Def make_struct_def(
    std::string&& name_,
    std::vector<Type_Param>&& type_params_,
    std::vector<Struct_Field>&& fields_,
    std::optional<Where_Clause>&& where_clause_ = std::nullopt
) {
  return Struct_Def{
      .name = std::move(name_),
      .type_params = std::move(type_params_),
      .fields = std::move(fields_),
      .where_clause = std::move(where_clause_)
  };
}

inline Struct_Def make_struct_def(std::string&& name_, std::vector<Struct_Field>&& fields_) {
  return make_struct_def(std::move(name_), {}, std::move(fields_), std::nullopt);
}

// Enum helpers
inline Enum_Variant make_enum_variant(std::string&& name_) {
  return Enum_Variant{Unit_Variant{.name = std::move(name_)}};
}

inline Enum_Variant make_enum_variant(std::string&& name_, std::vector<Type_Name>&& tuple_fields_) {
  return Enum_Variant{Tuple_Variant{.name = std::move(name_), .tuple_fields = std::move(tuple_fields_)}};
}

inline Enum_Variant make_enum_variant(std::string&& name_, std::vector<Struct_Field>&& struct_fields_) {
  return Enum_Variant{Struct_Variant{.name = std::move(name_), .struct_fields = std::move(struct_fields_)}};
}

inline Enum_Def make_enum_def(
    std::string&& name_,
    std::vector<Type_Param>&& type_params_,
    std::vector<Enum_Variant>&& variants_,
    std::optional<Where_Clause>&& where_clause_ = std::nullopt
) {
  return Enum_Def{
      .name = std::move(name_),
      .type_params = std::move(type_params_),
      .variants = std::move(variants_),
      .where_clause = std::move(where_clause_)
  };
}

inline Enum_Def make_enum_def(std::string&& name_, std::vector<Enum_Variant>&& variants_) {
  return make_enum_def(std::move(name_), {}, std::move(variants_), std::nullopt);
}

// Impl block helpers
inline Impl_Block make_impl_block(
    Type_Name&& type_name_,
    std::vector<Type_Param>&& type_params_,
    std::vector<Func_Def>&& methods_,
    std::optional<Where_Clause>&& where_clause_ = std::nullopt
) {
  return Impl_Block{
      .type_name = std::move(type_name_),
      .type_params = std::move(type_params_),
      .methods = std::move(methods_),
      .where_clause = std::move(where_clause_)
  };
}

inline Impl_Block make_impl_block(Type_Name&& type_name_, std::vector<Func_Def>&& methods_) {
  return make_impl_block(std::move(type_name_), {}, std::move(methods_), std::nullopt);
}

// Associated type helpers
inline Assoc_Type_Decl make_assoc_type_decl(std::string&& name_, std::vector<Trait_Bound>&& bounds_ = {}) {
  return Assoc_Type_Decl{.name = std::move(name_), .bounds = std::move(bounds_)};
}

inline Assoc_Type_Impl make_assoc_type_impl(std::string&& name_, Type_Name&& type_value_) {
  return Assoc_Type_Impl{.name = std::move(name_), .type_value = std::move(type_value_)};
}

// Trait helpers
inline Trait_Def make_trait_def(
    std::string&& name_,
    std::vector<Type_Param>&& type_params_,
    std::vector<Assoc_Type_Decl>&& assoc_types_,
    std::vector<Func_Decl>&& methods_,
    std::optional<Where_Clause>&& where_clause_ = std::nullopt
) {
  return Trait_Def{
      .name = std::move(name_),
      .type_params = std::move(type_params_),
      .assoc_types = std::move(assoc_types_),
      .methods = std::move(methods_),
      .where_clause = std::move(where_clause_)
  };
}

inline Trait_Def make_trait_def(std::string&& name_, std::vector<Func_Decl>&& methods_) {
  return make_trait_def(std::move(name_), {}, {}, std::move(methods_), std::nullopt);
}

inline Trait_Impl make_trait_impl(
    Type_Name&& trait_name_,
    Type_Name&& type_name_,
    std::vector<Type_Param>&& type_params_,
    std::vector<Assoc_Type_Impl>&& assoc_type_impls_,
    std::vector<Func_Def>&& methods_,
    std::optional<Where_Clause>&& where_clause_ = std::nullopt
) {
  return Trait_Impl{
      .trait_name = std::move(trait_name_),
      .type_name = std::move(type_name_),
      .type_params = std::move(type_params_),
      .assoc_type_impls = std::move(assoc_type_impls_),
      .methods = std::move(methods_),
      .where_clause = std::move(where_clause_)
  };
}

inline Trait_Impl make_trait_impl(Type_Name&& trait_name_, Type_Name&& type_name_, std::vector<Func_Def>&& methods_) {
  return make_trait_impl(std::move(trait_name_), std::move(type_name_), {}, {}, std::move(methods_), std::nullopt);
}

// Type_Alias helpers
inline Type_Alias
make_type_alias(std::string&& name_, std::vector<Type_Param>&& type_params_, Type_Name&& aliased_type_) {
  return Type_Alias{
      .name = std::move(name_),
      .type_params = std::move(type_params_),
      .aliased_type = std::move(aliased_type_)
  };
}

inline Type_Alias make_type_alias(std::string&& name_, Type_Name&& aliased_type_) {
  return Type_Alias{.name = std::move(name_), .type_params = {}, .aliased_type = std::move(aliased_type_)};
}

// Module helpers
inline Import_Item make_import_item(std::string&& name_, std::optional<std::string>&& alias_ = std::nullopt) {
  return Import_Item{.name = std::move(name_), .alias = std::move(alias_)};
}

inline Import_Statement
make_import_statement(std::vector<std::string>&& module_path_, std::vector<Import_Item>&& items_) {
  return Import_Statement{.module_path = std::move(module_path_), .items = std::move(items_)};
}

inline Item make_item(bool is_pub_, Statement&& item_) {
  return Item{.is_pub = is_pub_, .item = std::move(item_)};
}

inline Module make_module(std::vector<Import_Statement>&& imports_, std::vector<Item>&& items_) {
  return Module{.imports = std::move(imports_), .items = std::move(items_)};
}

inline Module make_module(std::vector<Item>&& items_) {
  return Module{.imports = {}, .items = std::move(items_)};
}

}  // namespace life_lang::ast

#endif  // LIFE_LANG_AST_HPP
