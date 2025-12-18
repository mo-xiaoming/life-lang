# life-lang Grammar Specification

This document provides the formal EBNF grammar for the life-lang programming language.

**⚠️ IMPORTANT**: This grammar is the authoritative source of truth. The parser implementation in `src/parser.cpp` MUST match these rules exactly. When modifying this grammar, update the corresponding `parse_*` methods in the parser.

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
integer = decimal_int | hex_int | binary_int ;
decimal_int = digit { digit | "_" } ;
hex_int = "0x" hex_digit { hex_digit | "_" } ;
binary_int = "0b" binary_digit { binary_digit | "_" } ;
hex_digit = digit | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C" | "D" | "E" | "F" ;
binary_digit = "0" | "1" ;
float = digit { digit } "." digit { digit } ;
bool_literal = "true" | "false" ;
string = '"' { any_char - '"' | escape_sequence } '"' ;
char = "'" ( any_char - "'" | escape_sequence ) "'" ;
unit_literal = "(" ")" ;
```

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

**Operator Precedence** (highest to lowest):
1. Field access, method calls, indexing: `.`, `()`, `[]`
2. **Type cast**: `as`
3. Unary: `-`, `!`, `&`, `*`
4. Multiplicative: `*`, `/`, `%`
5. Additive: `+`, `-`
6. Shift: `<<`, `>>`
7. Bitwise AND: `&`
8. Bitwise XOR: `^`
9. Bitwise OR: `|`
10. Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
11. Logical AND: `&&`
12. Logical OR: `||`
13. Range: `..`, `..=`

**Type Cast Semantics:**
- `expr as Type` - Explicit type conversion
- Allowed conversions (enforced in semantic analysis):
  - Numeric primitives: `I32 as I64`, `I32 as F64`, etc.
  - Pointer to integer: `&x as U64`
  - Integer to pointer: `0 as *T` (unsafe context only)
  - Enum variant to discriminant: `Color.Red as I32`
- Examples: `x + y as I64 * z` parses as `x + ((y as I64) * z)`

### Unary Operations

```ebnf
unary_expr = unary_op expr ;
unary_op = "-" | "!" | "&" | "*" ;
```

### Range Expressions

```ebnf
range_expr = expr ".." expr
           | expr ".." "=" expr
           | ".." expr
           | ".." "=" expr ;
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
pattern = literal_pattern
        | var_pattern
        | wildcard_pattern
        | tuple_pattern
        | struct_pattern
        | enum_pattern ;

literal_pattern = literal ;

var_pattern = var_name ;

wildcard_pattern = "_" ;

tuple_pattern = "(" pattern { "," pattern } ")" ;

struct_pattern = type_name "{" [ pattern_field { "," pattern_field } ] "}" ;
pattern_field = var_name [ ":" pattern ] ;

enum_pattern = type_name [ "(" pattern { "," pattern } ")" ] ;
```

## Notes

- **No parentheses** around conditions in `if`, `while`, `for` (braces provide boundaries)
- **Block trailing expressions**: Last expression without semicolon is the block's value
- **Immutability**: Values immutable by default (use `mut` keyword for mutation)
- **Type inference**: Type annotations optional in `let` statements when type can be inferred

## Implementation

The grammar is implemented as a hand-written recursive descent parser in `src/parser.cpp`.
See `src/parser.hpp` for the `Parser` class interface.
