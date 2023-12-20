# life-lang

[![CI](https://github.com/mo-xiaoming/life-lang/actions/workflows/CI.yaml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/CI.yaml)
[![codecov](https://codecov.io/gh/mo-xiaoming/life-lang/graph/badge.svg?token=L3QFA1WWQF)](https://codecov.io/gh/mo-xiaoming/life-lang)

Just fiddling around, this is like my 4th or 5th repo diving into

- Rust (finally decided to ditch c++)
- compiler design (probably light-years beyond my imaginary intelligence)

[x] parse simple math expressions

```rust
use life_lang::{lexer, parser, ast};

let cu = lexer::CompilationUnit::from_string(
    "stdin",
    "7%2 + 3 * (12 / ( 15 / - 3+1 - - 1) ) - 2 - 1 + 1;",
);
let ast = parser::parse(&cu).unwrap();
let evaluator = &mut ast::AstEvaluator::new(&ast);
assert_eq!(ast.accept(evaluator), Ok(Some(-13)));
```

[x] support string

- special characters: `\\`, `\"`, `\n`, `\r`, `\t`, `\0`, `\u{...}` (1-6 hex number within `{}`)
- double quoted strings like `"abc"`

```rust
use life_lang::{lexer, parser, ast};

let cu = lexer::CompilationUnit::from_string(
    "stdin",
    r#""\u{4f60}\u{597d}\u{1f316}";"#,
);
let ast = parser::parse(&cu).unwrap();
let printer = &mut ast::AstPrinter::new(&ast);
assert_eq!(ast.accept(printer), "你好🌖;\n");
```

[x] `let` and `var`

```rust
use life_lang::{lexer, parser, ast};

let cu = lexer::CompilationUnit::from_string(
    "stdin",
    "let x = 3; var y = x - 42;"
);
let ast = parser::parse(&cu).unwrap();
let printer = &mut ast::AstPrinter::new(&ast);
assert_eq!(ast.accept(printer), "let x = 3;\nvar y = (x - 42);\n");
```

[ ] better error messages
