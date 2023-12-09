# life-lang

[![CI](https://github.com/mo-xiaoming/life-lang/actions/workflows/CI.yaml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/CI.yaml)
[![codecov](https://codecov.io/gh/mo-xiaoming/life-lang/graph/badge.svg?token=L3QFA1WWQF)](https://codecov.io/gh/mo-xiaoming/life-lang)

Just fiddling around, this is like my 4th or 5th repo diving into

- Rust (finally decided to ditch c++)
- compiler design (probably light-years beyond my imaginary intelligence)

<!-- Following Julian Hartl's youtube series [_Building a compiler in rust_](https://github.com/julian-hartl/fusion-lang) -->

[x] step 1

```rust
use life_lang::{lexer, parser, ast};

let cu = lexer::CompilationUnit::from_string(
    "stdin",
    "7%2 + 3 * (12 / ( 15 / - 3+1 - - 1) ) - 2 - 1 + 1",
);
let parser = parser::Parser::new();
let ast = parser.parse(&cu).unwrap();
let printer = &mut ast::AstEvaluator::new(&ast);
assert_eq!(ast.accept(printer), Ok(Some(-13)));
```
