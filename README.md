# life-lang compiler

[![CI Build and Test](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/mo-xiaoming/life-lang/actions/workflows/build-and-test.yml)
[![codecov](https://codecov.io/gh/mo-xiaoming/life-lang/branch/master/graph/badge.svg)](https://codecov.io/gh/mo-xiaoming/life-lang)

## How to build

Make sure you have vcpkg installed. You can follow the official setup guide [here](https://github.com/microsoft/vcpkg#quick-start).

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

./build/lifec
```

## The language

```
fn main(args: std::Array<std::String>): i32 {
    std::print("Hello world!");
    return 0;
}
```

```cpp
x3::symbols<int> symtab;

auto mkkw = [](std::string kw) {
    symtab.add(kw);
    return lexeme[x3::lit(kw) >> !alnum];
}

auto const kw_fn = mkkw("fn")
auto const kw_let = mkkw("let")
auto const reserved = lexeme[symtab >> !(alnum | char_('_'))];
auto const ident = lexeme[*char_('_') >> alpha >> *(alnum | char_('_'))] - reserved
auto const fn_sign = kw_fn > ident > lit('(') > *(ident > type) > lit(')') > lit(':') > type
auto const fn_decl = fn_sign > ';'
auto const fn_body = lit('{') > stat > lit('}')
auto const fn_def = fn_sign > fn_body
auto const stmt = fn_decl | fn_def
auto const tu = +stmt;
```