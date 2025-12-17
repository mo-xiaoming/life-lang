# S-Expression Grammar for life-lang AST

## Overview

S-expressions (symbolic expressions) are the serialization format for life-lang AST nodes. They provide a human-readable, Lisp-style representation for debugging, testing, and inspection.

## Format

S-expressions use parentheses for nesting and support both compact and indented formatting:

```lisp
; Compact (indent=0)
(func_def (func_decl "main" () () (path ((type_segment "I32")))) (block ((return (integer "42")))))

; Indented (indent=2, default)
(func_def
  (func_decl "main" () ()
    (path
      ((type_segment "I32"))))
  (block
    ((return
      (integer "42")))))
```

##Grammar Rules

### Basic Syntax

```ebnf
sexp          = atom | list ;
atom          = string | symbol | number | boolean | nil ;
list          = "(" tag elements ")" ;
tag           = symbol ;
elements      = sexp* ;

string        = '"' (char | escape_sequence)* '"' ;
symbol        = [a-z_] [a-z0-9_]* ;
boolean       = "true" | "false" ;
nil           = "nil" ;
```

### Escape Sequences in Strings

- `\"` - double quote
- `\\` - backslash  
- `\n` - newline
- `\r` - carriage return
- `\t` - tab

### AST Node Representations

#### Literals

```lisp
(integer "value")                    ; Integer without suffix
(integer "value" "suffix")           ; Integer with type suffix (I32, U64, etc.)
(float "value")                      ; Float without suffix  
(float "value" "suffix")             ; Float with type suffix (F32, F64)
(string "content")                   ; String literal
(char "c")                           ; Character literal
(bool true)                          ; Boolean literal
(bool false)
```

**Examples:**
```lisp
(integer "42")
(integer "1000" "I64")
(float "3.14")
(float "2.5" "F32")
(string "hello world")
(char "x")
(bool true)
```

#### Types

```lisp
(path (segments))                    ; Type name path
(type_segment "name")                ; Simple type segment
(type_segment "name" (type_params))  ; Generic type segment
(tuple (elements))                   ; Tuple type
(func_type (params) return_type)     ; Function type
(unit)                               ; Unit type ()
```

**Examples:**
```lisp
(path ((type_segment "I32")))
(path ((type_segment "Vec" ((path ((type_segment "String")))))) )
(tuple ((path ((type_segment "I32"))) (path ((type_segment "String")))))
(func_type ((path ((type_segment "I32")))) (path ((type_segment "String"))))
(unit)
```

#### Variables and Paths

```lisp
(var (segments))                     ; Variable reference
(var_segment "name")                 ; Simple variable segment
(var_segment "name" (type_params))   ; Generic variable segment
```

**Examples:**
```lisp
(var ((var_segment "x")))
(var ((var_segment "HashMap" ((path ((type_segment "String"))) (path ((type_segment "I32")))))))
```

#### Expressions

```lisp
(binary op lhs rhs)                  ; Binary operation (+, -, *, /, ==, etc.)
(unary op operand)                   ; Unary operation (-, !, not)
(assign lhs rhs)                     ; Assignment
(field_access expr "field")          ; Field access (expr.field)
(func_call name (args))              ; Function call
(method_call receiver "method" (args)) ; Method call
(struct_lit type_name (fields))      ; Struct literal
(if cond then_branch else_branch)    ; If expression
(match scrutinee (arms))             ; Match expression
(block (statements))                 ; Block expression
(for pattern iterator body)          ; For loop
(while cond body)                    ; While loop
(range start end inclusive)          ; Range expression
```

**Examples:**
```lisp
(binary + (integer "1") (integer "2"))
(unary - (integer "42"))
(assign (var ((var_segment "x"))) (integer "10"))
(field_access (var ((var_segment "point"))) "x")
(func_call (var ((var_segment "println"))) ((string "hello")))
(method_call (var ((var_segment "vec"))) "push" ((integer "1")))

(struct_lit (path ((type_segment "Point")))
  ((field_init "x" (integer "1"))
   (field_init "y" (integer "2"))))

(if (binary == (var ((var_segment "x"))) (integer "0"))
  (integer "1")
  (integer "2"))

(match (var ((var_segment "opt")))
  ((match_arm (variant_pattern "Some" ((pattern "x"))) nil (var ((var_segment "x"))))
   (match_arm (variant_pattern "None" ()) nil (integer "0"))))

(block
  ((let false (pattern "x") nil (integer "42"))
   (return (var ((var_segment "x"))))))

(for (pattern "i") (range (integer "0") (integer "10") false)
  (block
    ((func_call (var ((var_segment "println"))) ((var ((var_segment "i"))))))))

(while (binary < (var ((var_segment "x"))) (integer "10"))
  (block
    ((assign (var ((var_segment "x"))) (binary + (var ((var_segment "x"))) (integer "1"))))))

(range (integer "0") (integer "10") false)
```

#### Patterns

```lisp
(pattern "name")                     ; Variable binding pattern
(variant_pattern "variant" (patterns)) ; Enum variant pattern
(struct_pattern type_name (fields))  ; Struct pattern
(tuple_pattern (patterns))           ; Tuple pattern
(field_pattern "name" pattern)       ; Field pattern in struct pattern
```

**Examples:**
```lisp
(pattern "x")
(variant_pattern "Some" ((pattern "value")))
(variant_pattern "None" ())
(struct_pattern (path ((type_segment "Point")))
  ((field_pattern "x" (pattern "px"))
   (field_pattern "y" (pattern "py"))))
(tuple_pattern ((pattern "a") (pattern "b")))
```

#### Statements

```lisp
(let mut pattern type_annotation initializer) ; Let statement
(return expr)                        ; Return statement  
(break)                              ; Break statement
(continue)                           ; Continue statement
(expr_stmt expr)                     ; Expression statement
```

**Examples:**
```lisp
(let false (pattern "x") nil (integer "42"))
(let true (pattern "count") (path ((type_segment "I32"))) (integer "0"))
(return (var ((var_segment "x"))))
(break)
(continue)
```

#### Declarations

```lisp
(func_def func_decl body)            ; Function definition
(func_decl "name" (type_params) (params) return_type) ; Function declaration
(param mut "name" type)              ; Function parameter
(struct_def "name" (type_params) (fields)) ; Struct definition
(field "name" type)                  ; Struct field
(enum_def "name" (type_params) (variants)) ; Enum definition
(variant "name" (fields))            ; Enum variant
(trait_def "name" (type_params) (items) where_clause) ; Trait definition
(impl_block type_name (type_params) (items) where_clause) ; Impl block
(trait_impl trait_name impl_type (type_params) (items) where_clause) ; Trait impl
(type_alias "name" (type_params) type where_clause) ; Type alias
```

**Examples:**
```lisp
(func_def
  (func_decl "add" ()
    ((param false "a" (path ((type_segment "I32"))))
     (param false "b" (path ((type_segment "I32")))))
    (path ((type_segment "I32"))))
  (block
    ((return (binary + (var ((var_segment "a"))) (var ((var_segment "b"))))))))

(struct_def "Point" ()
  ((field "x" (path ((type_segment "I32"))))
   (field "y" (path ((type_segment "I32"))))))

(enum_def "Option" ((type_param (path ((type_segment "T"))) ()))
  ((variant "Some" ((path ((type_segment "T")))))
   (variant "None" ())))

(trait_def "Display" () () nil)

(impl_block (path ((type_segment "Point"))) ()
  ((func_def
    (func_decl "new" ()
      ((param false "x" (path ((type_segment "I32"))))
       (param false "y" (path ((type_segment "I32")))))
      (path ((type_segment "Point"))))
    (block
      ((return (struct_lit (path ((type_segment "Point")))
        ((field_init "x" (var ((var_segment "x"))))
         (field_init "y" (var ((var_segment "y")))))))))))
  nil)

(type_alias "Result" ((type_param (path ((type_segment "T"))) ())
                      (type_param (path ((type_segment "E"))) ()))
  (path ((type_segment "Result")
         ((path ((type_segment "T")))
          (path ((type_segment "E"))))))
  nil)
```

#### Trait Bounds and Where Clauses

```lisp
(type_param name (bounds))           ; Type parameter with bounds
(trait_bound trait_name)             ; Trait bound
(where (predicates))                 ; Where clause
(where_pred type_name (bounds))      ; Where predicate
```

**Examples:**
```lisp
(type_param (path ((type_segment "T")))
  ((trait_bound (path ((type_segment "Display"))))))

(where
  ((where_pred (path ((type_segment "T")))
     ((trait_bound (path ((type_segment "Clone"))))
      (trait_bound (path ((type_segment "Debug"))))))))
```

#### Module

```lisp
(module (items))                     ; Top-level module
```

**Example:**
```lisp
(module
  ((struct_def "Point" ()
     ((field "x" (path ((type_segment "I32"))))
      (field "y" (path ((type_segment "I32"))))))
   (func_def
     (func_decl "main" () () (path ((type_segment "I32"))))
     (block
       ((return (integer "42")))))))
```

## Special Values

- `nil` - Represents absence of value (used for optional fields)
- `()` - Empty list (used for empty vectors/sequences)
- `true` / `false` - Boolean values

## Formatting Guidelines

### Compact Mode (indent=0)
- No newlines between elements
- Minimal whitespace
- Suitable for programmatic processing

### Indented Mode (indent=2, default)
- Each nested list on a new line
- 2 spaces per indentation level
- Easier for human reading

### When to Use Each Mode

- **Tests**: Use compact mode for easier string matching
- **Debugging**: Use indented mode for readability
- **CLI output**: Use indented mode by default
- **Logging**: Use compact mode to save space

## Comparison with JSON

| Aspect | S-expression | JSON |
|--------|-------------|------|
| Syntax | `(tag ...)` | `{"tag": {...}}` |
| Nesting | Natural via parentheses | Nested objects |
| Arrays | `(item1 item2)` | `[item1, item2]` |
| Optional | `nil` | `null` |
| Size | More compact | More verbose |
| Readability | Lisp-style | JavaScript-style |
| Dependencies | None (built-in) | nlohmann-json library |

## Implementation Notes

- S-expressions are generated by `life_lang::ast::to_sexp_string(node, indent)`
- Default indent is 2 spaces
- Use indent=0 for compact, single-line output
- All strings are properly escaped
- Numbers and symbols are written without quotes
- Lists are recursively formatted with proper indentation
