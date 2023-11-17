# fusion-lang

[![CI](https://github.com/mo-xiaoming/life-lang/actions/workflows/CI.yaml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/CI.yaml)

Following Julian Hartl's youtube series [_Building a compiler in rust_](https://github.com/julian-hartl/fusion-lang)

```
7^2 + 3 * (12 / (+15 / ( 3+1) - - 1) ) - 2^3^4

1-1+1


number: [1-9][0-9]* | 0
atom: number | - number | + number | ( expression )
expression: atom bin_op atom | atom
```