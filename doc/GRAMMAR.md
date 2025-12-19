# life-lang Grammar Specification

This document provides the formal EBNF grammar for the life-lang programming language.

**⚠️ IMPORTANT**: This grammar is the authoritative source of truth. The parser implementation in `src/parser.cpp` MUST match these rules exactly. When modifying this grammar, update the corresponding `parse_*` methods in the parser.

## Design Philosophy

life-lang is guided by three core principles:

1. **Consistent behavior** across all build modes (debug, release, optimized)
   - Same code always produces same results
   - No hidden surprises between development and production
   - Test once, deploy with confidence

2. **Fail fast** on programmer errors
   - Overflow, underflow, division by zero → immediate panic
   - Detect bugs early rather than propagate incorrect values
   - Explicit alternatives available when needed (`checked_*`, `wrapping_*`, `saturating_*`)

3. **Predictable results**
   - No undefined behavior
   - No silent data corruption
   - Clear, documented semantics for all operations

These principles prioritize **correctness and user-friendliness over raw performance**. When maximum performance is needed, explicit opt-in mechanisms are provided.

## Lexical Elements

### Keywords
```ebnf
keyword = "fn" | "struct" | "enum" | "impl" | "trait" | "let" | "mut" 
        | "if" | "else" | "match" | "for" | "in" | "while" | "break" | "continue"
        | "return" | "type" | "where" | "self" | "Self" | "pub" | "import" | "as" ;
```

### Identifiers
```ebnf
var_name = letter { letter | digit | "_" } ;
type_name = upper { letter | digit | "_" } ;
```

### Literals
```ebnf
integer = ( decimal_int | hex_int | octal_int | binary_int ) [ int_suffix ] ;
decimal_int = digit { digit | "_" } ;
hex_int = "0x" hex_digit { hex_digit | "_" } ;
octal_int = "0o" octal_digit { octal_digit | "_" } ;
binary_int = "0b" binary_digit { binary_digit | "_" } ;
hex_digit = digit | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C" | "D" | "E" | "F" ;
octal_digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" ;
binary_digit = "0" | "1" ;
int_suffix = "I8" | "I16" | "I32" | "I64" | "U8" | "U16" | "U32" | "U64" ;
float = digit { digit | "_" } "." digit { digit | "_" } [ exponent ] [ float_suffix ]
      | digit { digit | "_" } exponent [ float_suffix ]
      | nan_literal [ float_suffix ]
      | inf_literal [ float_suffix ] ;
nan_literal = "nan" | "NaN" | "NAN" | "Nan" ;
inf_literal = "inf" | "Inf" | "INF" ;
exponent = ( "e" | "E" ) [ "+" | "-" ] digit { digit | "_" } ;
float_suffix = "F32" | "F64" ;
bool_literal = "true" | "false" ;
string = '"' { any_char - '"' | escape_sequence } '"' ;
string_interpolation = '"' { string_part | "{" expr "}" } '"' ;
string_part = { any_char - '"' - "{" | escape_sequence } ;
raw_string = "r" { "#" } '"' { any_char } '"' { "#" } ;
char = "'" ( any_char - "'" | escape_sequence ) "'" ;
unit_literal = "(" ")" ;
```

**Notes:**
- Integers support decimal, hexadecimal (0x prefix), octal (0o prefix), and binary (0b prefix) formats
- Underscores can be used for readability: `1_000_000`, `0xFF_FF`, `0o755`, `0b1111_0000`
- Type suffixes specify exact numeric type: `42I32`, `255U8`, `3.14F32`
- Leading zeros not allowed in decimal integers (except standalone `0`)
- **Negative integer literals**: Parsed as unary minus applied to positive literal (e.g., `-128I8` → `(unary - (integer "128" "I8"))`)
  - Minimum values (e.g., `-128I8`, `-32768I16`, `-2147483648I32`) validated during semantic analysis
  - The literal `128` would overflow `I8`, but `-128I8` is valid as it represents the minimum value
  - Semantic analyzer recognizes the pattern and treats it as the minimum value constant
- Floats support scientific notation: `1.5e10`, `3.14E-5`
- Floats require either a decimal point or exponent (or both)
- **IEEE 754 special values** as literals:
  - NaN: `nan`, `NaN`, `NAN`, or `Nan` (with optional suffix: `nanF32`, `NaNF64`)
  - Positive infinity: `inf`, `Inf`, or `INF` (with optional suffix: `infF32`, `InfF64`)
  - Negative infinity: `-inf`, `-Inf`, `-INF` (unary minus applied to infinity literal)
  - Rationale: More user-friendly than requiring `F32::NAN` constants; aligns with Python, JavaScript, C99
- **String types - three distinct forms with different purposes**:
  
  1. **Regular strings** (`"..."`): Escape sequences processed
     - Example: `"Hello\nWorld"` → newline between words
     - Use for: User-facing text with escape sequences
  
  2. **String interpolation** (`"...{expr}..."`): Escape sequences + expression substitution
     - Example: `"Hello, {name}! You are {age} years old."`
     - Full expression support: `"result is {x + y * 2}"`
     - Method calls: `"Name: {user.name.to_upper()}"`
     - Empty `{}` not treated as interpolation (literal braces in format strings)
     - Use for: Dynamic user-facing messages, logging with values
     - Rationale: More ergonomic than format functions; aligns with Python f-strings, JavaScript, Kotlin, Swift
  
  3. **Raw strings** (`r"..."`, `r#"..."#`): No processing at all - everything is literal
     - Basic syntax: `r"C:\path\to\file"` - backslashes are literal
     - With embedded quotes: `r#"He said "hello""#` - delimiter allows quotes in content
     - Multiple delimiters: `r##"Contains "# and "#" patterns"##` - match delimiter count
     - Multi-line: Raw strings can span multiple lines naturally
     - No escape processing: `r"\n"` is literally backslash-n, not a newline character
     - **No interpolation**: `r"value: {x}"` contains literal braces, not an expression
     - Use for: Regex patterns, Windows file paths, JSON/XML templates, SQL queries
     - Rationale: Avoid escaping hell for regex, file paths, embedded languages; aligns with Rust, Python, C++11
  
  **Design principle**: Raw strings and interpolation serve opposite purposes:
  - Interpolation: Dynamic content with escape processing (`"user: {name}\n"`)
  - Raw strings: Static literal content, no processing (`r"regex: \d+\.\w+"`)
  - Combining them would be confusing: which braces are literal vs interpolation?
  - If you need both, use interpolation with escaped backslashes: `"path: {user}\\Documents"`

- **String concatenation**: No special operator (`+` or `++`)
  
  **Idiomatic approach - use string interpolation**:
  ```rust
  // Instead of: a + b + c
  let result = "{a}{b}{c}";           // Clean, one allocation
  
  // Path building
  let path = "{base_dir}/{user}/{file}.txt";
  
  // Message construction
  let msg = "{prefix} {content} {suffix}";
  ```
  
  **Rationale for no `+` operator**:
  - **Preserves value semantics**: `+` has consistent meaning (numeric addition only)
  - **Avoids type confusion**: `1 + 2` is always arithmetic, never string concatenation
  - **Better performance**: `"{a}{b}{c}"` allocates once; `a + b + c` creates intermediate copies
  - **More explicit**: String building is intentional, not hidden behind overloaded operators
  - **Interpolation covers 90% of use cases**: Most concatenation is for messages, paths, formatting
  
  **For explicit array joining** (future standard library):
  ```rust
  String::concat([a, b, c])           // Join array of strings
  String::join([a, b, c], ", ")       // Join with separator
  ```
  
  **Comparison with other languages**:
  - Python/JavaScript: `+` operator creates performance issues (`a + b + c + d` = multiple copies)
  - Rust: `.to_owned() + &b` is verbose and awkward
  - Go: Library-only (`strings.Join`) - verbose but principled
  - **Life-lang**: Interpolation (ergonomic) + library functions (explicit edge cases) = best of both

### Comments
```ebnf
comment = "//" { any_char - newline } newline
        | "/*" { any_char } "*/" ;
```

## Module Structure

```ebnf
module = { import_statement | item } ;

import_statement = "import" module_path "." "{" import_item_list "}" ";" ;

module_path = type_name { "." type_name } ;

import_item_list = import_item { "," import_item } ;
import_item = identifier [ "as" identifier ] ;
identifier = var_name | type_name ;

item = [ "pub" ] ( func_def
                 | struct_def  
                 | enum_def
                 | impl_block
                 | trait_def
                 | trait_impl
                 | type_alias ) ;
```

**Notes:**
- Import statements must appear before items (conventional ordering)
- The `pub` modifier makes items visible outside the module
- Module path uses `.` separator (e.g., `Geometry.Shapes`)
- Dot before `{` indicates "from this module, import these items" (similar to Rust's `::`)
- Identifier list allows importing both types and functions
- Name conflicts: Importing the same name from multiple modules is an error (semantic analysis phase)

## Declarations

### Function Definitions
```ebnf
func_def = "fn" var_name [ template_params ] "(" [ param_list ] ")" ":" type_name [ where_clause ] block ;

param_list = func_param { "," func_param } ;
func_param = [ "mut" ] var_name ":" type_name ;
```

**Note:** The optional `pub` modifier is specified at the item level (see Module Structure).

### Struct Definitions
```ebnf
struct_def = "struct" type_name [ template_params ] [ where_clause ] "{" [ field_list ] "}" ;

field_list = struct_field { "," struct_field } ;
struct_field = [ "pub" ] var_name ":" type_name ;
```

### Enum Definitions
```ebnf
enum_def = "enum" type_name [ template_params ] [ where_clause ] "{" variant_list "}" ;

variant_list = enum_variant { "," enum_variant } ;
enum_variant = type_name [ "(" type_name { "," type_name } ")" ] ;
```

### Impl Blocks
```ebnf
impl_block = "impl" [ template_params ] type_name [ where_clause ] "{" { impl_method } "}" ;
impl_method = [ "pub" ] func_def ;
```

### Trait Definitions
```ebnf
trait_def = "trait" type_name [ template_params ] [ ":" trait_bound_list ] [ where_clause ] "{" { trait_member } "}" ;
```

### Trait Implementations
```ebnf
trait_impl = "impl" [ template_params ] type_name "for" type_name [ where_clause ] "{" { func_def } "}" ;
```

### Type Aliases
```ebnf
type_alias = "type" type_name [ template_params ] "=" type_name [ where_clause ] ";" ;
```

## Generic Parameters

### Template Parameters
```ebnf
template_params = "<" type_param { "," type_param } ">" ;
type_param = type_name [ ":" trait_bound_list ] ;
```

### Trait Bounds
```ebnf
trait_bound_list = trait_bound { "+" trait_bound } ;
trait_bound = type_name [ "<" type_name { "," type_name } ">" ] ;
```

### Where Clauses
```ebnf
where_clause = "where" where_predicate { "," where_predicate } ;
where_predicate = type_name ":" trait_bound_list ;
```

## Types

```ebnf
type_name = path_type
          | function_type
          | tuple_type
          | array_type ;

path_type = type_name_segment { "." type_name_segment } ;
type_name_segment = type_name [ "<" type_name { "," type_name } ">" ] ;

function_type = "fn" "(" [ type_name { "," type_name } ] ")" [ ":" type_name ] ;

tuple_type = "(" type_name { "," type_name } ")" ;

array_type = "[" type_name ";" integer "]" ;
```

## Statements

```ebnf
statement = let_statement
          | expr_statement
          | return_statement
          | break_statement
          | continue_statement
          | assignment_statement ;

let_statement = "let" [ "mut" ] pattern [ ":" type_name ] "=" expr ";" ;

expr_statement = expr ";" ;

return_statement = "return" [ expr ] ";" ;

break_statement = "break" ";" ;

continue_statement = "continue" ";" ;

assignment_statement = expr "=" expr ";" ;
```

## Expressions

```ebnf
expr = if_expr
     | match_expr
     | for_expr
     | while_expr
     | block
     | binary_expr
     | unary_expr
     | range_expr
     | primary_expr ;
```

### Control Flow

```ebnf
if_expr = "if" expr block { "else" "if" expr block } [ "else" block ] ;

match_expr = "match" expr "{" match_arm { "," match_arm } "}" ;
match_arm = pattern [ "if" expr ] "=>" expr ;

for_expr = "for" pattern "in" expr block ;

while_expr = "while" expr block ;

block = "{" { statement } [ expr ] "}" ;
```

### Binary Operations

```ebnf
binary_expr = expr binary_op expr ;
binary_op = "+" | "-" | "*" | "/" | "%" | "==" | "!=" | "<" | ">" | "<=" | ">=" 
          | "&&" | "||" | "&" | "|" | "^" | "<<" | ">>" | "as" ;
```

**Arithmetic Overflow and Error Behavior:**

**Design Philosophy:**
- **Consistent behavior** across all build modes (debug, release, optimized)
- **Fail fast** on programmer errors (division by zero, overflow)
- **Predictable results** - same code always produces same behavior
- Prioritizes correctness and user-friendliness over raw performance

**Integer Arithmetic (`+`, `-`, `*`):**
- **Always panics on overflow/underflow** (all build modes)
- Examples:
  - `I32::MAX + 1` → panic
  - `I32::MIN - 1` → panic
  - `I32::MAX * 2` → panic
- **Rationale**: Overflow is a logic error; wrapping silently can cause security bugs and incorrect results
- **Special case - Minimum value literals**: `-128I8`, `-32768I16`, `-2147483648I32`, `-9223372036854775808I64`
  - Parsed as unary minus applied to positive literal: `-128I8` → `(unary - (integer "128" "I8"))`
  - Even though `128` exceeds `I8::MAX` (127), the pattern is recognized during semantic analysis
  - Validated to represent the exact minimum value (`I8::MIN`, etc.)
  - No overflow panic for these specific literals, as they represent valid constants

**Division and Modulo (`/`, `%`):**
- **Division by zero**: Always panics
  - Integer: `x / 0` → panic, `x % 0` → panic
  - Float: `x / 0.0` → `Infinity` or `-Infinity` (IEEE 754 standard, no panic)
- **Integer overflow cases**: Always panic
  - `I32::MIN / -1` → panic (would overflow to `I32::MAX + 1`)
  - `I32::MIN % -1` → panic (for consistency)
- **Rationale**: Division by zero is always a logic error

**Bitwise Shift (`<<`, `>>`):**
- **Shift amount ≥ bit width**: Always panics
  - `1u32 << 32` → panic
  - `1i64 >> 64` → panic
- **Negative shift amount**: Always panics
  - `x << -1` → panic
- **Rationale**: Shift overflow often indicates a bug; results are architecture-dependent

**Explicit Overflow Handling (Future):**
When overflow is intentional or needs special handling:
- `wrapping_add()`, `wrapping_mul()` → explicit wrap-around (two's complement)
- `saturating_add()`, `saturating_mul()` → clamp to min/max
- `checked_add()`, `checked_mul()` → `Option<T>` (None on overflow, avoids panic)
- `overflowing_add()` → `(T, Bool)` (result and overflow flag)

**Float Arithmetic:**
- Follows IEEE 754: `NaN`, `Infinity`, `-Infinity`, denormals
- No panics: `1.0 / 0.0` → `Infinity`, `0.0 / 0.0` → `NaN`
- Use `is_nan()`, `is_infinite()` to check for special values

**Comparison with Other Languages:**
- **Rust**: Debug=panic, Release=wrap (inconsistent, requires testing both)
- **Swift**: Always traps on overflow (consistent, like life-lang)
- **Python/Java**: No overflow on arbitrary-precision/BigInt types
- **C/C++**: Undefined behavior (dangerous)
- **life-lang**: Always panic (consistent, safe, predictable)

**Operator Precedence** (highest to lowest):
1. Field access, method calls, indexing: `.`, `()`, `[]`
2. **Type cast**: `as`
3. Unary: `-`, `!`, `~`
4. Multiplicative: `*`, `/`, `%`
5. Additive: `+`, `-`
6. Shift: `<<`, `>>`
7. Comparison: `<`, `>`, `<=`, `>=`
8. Equality: `==`, `!=`
9. Bitwise AND: `&`
10. Bitwise XOR: `^`
11. Bitwise OR: `|`
12. Logical AND: `&&`
13. Logical OR: `||`
14. Range: `..`, `..=`

**Notes:**
- Precedence follows standard C/Rust conventions
- Logical operators have lower precedence than bitwise operators
- Within bitwise operators: AND binds tighter than XOR, XOR tighter than OR
- Higher precedence = tighter binding (evaluates first)

**Type Cast Semantics:**
- `expr as Type` - Explicit type conversion
- **Never panics** - truncates, wraps, or saturates on overflow
- Allowed conversions (enforced in semantic analysis):
  - **Integer to integer**:
    - Same size (e.g., `I32 as U32`): no-op, reinterprets bits (2's complement)
    - Larger to smaller (e.g., `I64 as I32`): truncates to lower bits, wraps around
    - Smaller to larger (e.g., `I32 as I64`): zero-extends if unsigned, sign-extends if signed
  - **Float to integer**:
    - Rounds toward zero
    - `NaN` becomes `0`
    - Values exceeding range saturate to `min`/`max` (no overflow panic)
  - **Integer to float**: produces closest representable value
  - **Float to float**: `F32 as F64` is lossless, `F64 as F32` rounds to nearest
  - Pointer to integer: `&x as U64` (implementation-defined size)
  - Integer to pointer: `0 as *T` (unsafe context only)
  - Enum variant to discriminant: `Color.Red as I32`
- Examples: `x + y as I64 * z` parses as `x + ((y as I64) * z)`
- **Design rationale**: Following Rust's semantics - explicit casts never panic, programmer takes responsibility for correctness

### Unary Operations

```ebnf
unary_expr = unary_op expr ;
unary_op = "-" | "+" | "!" | "~" ;
```

**Notes:**
- `-`: Arithmetic negation (e.g., `-5`, `-x`)
- `+`: Arithmetic positive/identity (e.g., `+5`, `+x`)
- `!`: Logical NOT (e.g., `!flag`, `!condition`)
- `~`: Bitwise NOT (e.g., `~bits`, `~0xFF`)

### Range Expressions

```ebnf
range_expr = expr ".." [ "=" ] expr      (* bounded range: start to end *)
           | expr ".." [ "=" ]          (* unbounded end: start to infinity *)
           | ".." [ "=" ] expr          (* unbounded start: -infinity to end *)
           | ".."                        (* fully unbounded: all values *)
           ;

(* Range Expression Semantics:
   - '..' (exclusive): half-open interval, excludes endpoint
   - '..=' (inclusive): closed interval, includes endpoint
   
   Conceptual model:
   - 'a..b'   ≡ [a, b)   = {x | a ≤ x < b}
   - 'a..=b'  ≡ [a, b]   = {x | a ≤ x ≤ b}
   - 'a..'    ≡ [a, ∞)   = {x | a ≤ x ≤ type_max}  (includes MAX)
   - '..b'    ≡ (-∞, b)  = {x | type_min ≤ x < b}  (excludes b)
   - '..=b'   ≡ (-∞, b]  = {x | type_min ≤ x ≤ b}  (includes b)
   - '..'     ≡ (-∞, ∞)  = {x | type_min ≤ x ≤ type_max}  (all values)
   
   Key insight: Unbounded ranges are infinite intervals clipped to type bounds.
   - 'a..' includes type_max because [a, ∞) has no "next value" beyond max
   - 'a..b' excludes b per half-open interval definition
   
   Examples:
   - 0..10    = 0,1,2,3,4,5,6,7,8,9          (excludes 10)
   - 0..=10   = 0,1,2,3,4,5,6,7,8,9,10       (includes 10)
   - 18..     = 18,19,...,I32_MAX            (includes MAX for I32)
   - ..5      = I32_MIN,...,2,3,4            (excludes 5)
   - ..=5     = I32_MIN,...,3,4,5            (includes 5)
   - ..       = I32_MIN,...,I32_MAX          (all I32 values)
*)
```

### Primary Expressions

```ebnf
primary_expr = literal
             | var_name
             | func_call
             | struct_literal
             | array_literal
             | index_expr
             | field_access
             | tuple_literal
             | "(" expr ")" ;

func_call = expr "(" [ expr { "," expr } ] ")" ;

struct_literal = type_name "{" [ field_init { "," field_init } ] "}" ;
field_init = var_name ":" expr ;

array_literal = "[" [ expr { "," expr } ] "]" ;

index_expr = expr "[" expr "]" ;

field_access = expr "." var_name ;

tuple_literal = "(" expr { "," expr } ")" ;

literal = integer | float | bool_literal | string | char | unit_literal ;
```

## Patterns

```ebnf
pattern = or_pattern ;

or_pattern = single_pattern { "|" single_pattern } ;

single_pattern = literal_pattern
               | var_pattern
               | wildcard_pattern
               | tuple_pattern
               | struct_pattern
               | enum_pattern ;

literal_pattern = literal ;

var_pattern = var_name ;

wildcard_pattern = "_" ;

tuple_pattern = "(" pattern { "," pattern } ")" ;

struct_pattern = type_name "{" [ pattern_field { "," pattern_field } [ "," ".." ] ] "}" 
               | type_name "{" ".." "}" ;
pattern_field = var_name [ ":" pattern ] ;

enum_pattern = type_name [ "(" pattern { "," pattern } ")" ] ;
```

**Rest Pattern (`..`) Notes:**
- **Purpose**: Ignore remaining/private fields in struct patterns
- **Type-only matching**: `Point { .. }` - matches any Point, ignores all fields
- **Partial matching**: `Config { debug, timeout, .. }` - match specific fields, ignore rest
- **Required for private fields**: When matching structs from other modules with private fields
- **Position**: Must come after named fields (if any)
- **Examples**:
  ```rust
  // Type-only matching
  match shape {
    Point { .. } => "it's a point",
    Circle { .. } => "it's a circle",
  }
  
  // Partial field matching
  match config {
    Config { debug: true, .. } => enable_logging(),
    Config { host, port, .. } => connect(host, port),
  }
  
  // Cross-module matching (User has private fields)
  match user {
    User { name: "admin", .. } => grant_access(),  // Ignores private fields
    User { name, .. } => regular_user(name),
  }
  ```

**Visibility Rules for Pattern Matching:**
- **Same module**: Can match all fields (pub and private)
- **Other modules**: Can ONLY match `pub` fields
  - Must use `..` to ignore private fields
  - Error if attempting to name a private field from another module
- **Rationale**: Preserves encapsulation while allowing practical pattern matching
- **Examples**:
  ```rust
  // Module A
  pub struct User {
    pub name: String,
    age: I32,  // private
  }
  
  // Module B (importing User)
  match user {
    User { name, .. } => {}              // ✅ OK: ignores private age
    User { name: "Alice", .. } => {}     // ✅ OK: matches pub field
    User { name, age } => {}             // ❌ ERROR: age is private
  }
  
  // Module A (same module as User)
  match user {
    User { name, age } => {}             // ✅ OK: all fields visible
  }
  ```

**Or-Pattern Notes:**
- **Top-level or-patterns**: `Some(1) | Some(2) | None` - matches any of the alternatives
- **Nested or-patterns**: `Some(1 | 2 | 3)` - same as `Some(1) | Some(2) | Some(3)` for this case
- **Mixed variants**: `Ok(()) | Err(SpecificError)` - can mix different enum variants
- **Semantic constraint**: All alternatives must bind the same set of variables with the same types
  - Valid: `Point { x: 0, y } | Point { x: 1, y }` - both bind `y`
  - Invalid: `Point { x: 0, y } | Point { x, y: 1 }` - different variables
- **Examples**:
  ```rust
  match char {
    'a' | 'e' | 'i' | 'o' | 'u' => "vowel",  // Simple alternatives
    _ => "consonant",
  }
  
  match value {
    Some(1 | 2 | 3) => "small",              // Nested or-pattern
    Some(x) => "other: {x}",
    None => "nothing",
  }
  
  match result {
    Ok(()) | Err(RecoverableError) => continue,  // Mixed variants
    Err(e) => panic("Fatal: {e}"),
  }
  ```

## Notes

- **No parentheses** around conditions in `if`, `while`, `for` (braces provide boundaries)
- **Block trailing expressions**: Last expression without semicolon is the block's value
- **Immutability**: Values immutable by default (use `mut` keyword for mutation)
- **Type inference**: Type annotations optional in `let` statements when type can be inferred

## Implementation

The grammar is implemented as a hand-written recursive descent parser in `src/parser.cpp`.
See `src/parser.hpp` for the `Parser` class interface.
