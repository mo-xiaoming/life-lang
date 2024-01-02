mod dp;

use crate::{ast, lexer};
use colored::*;

#[derive(Debug)]
enum IntermediateResult {
    Finished,
    Node {
        node: ast::AstNode,
        next_token_idx: lexer::TokenIdx,
    },
}

type ParseResult = Result<IntermediateResult, ParseError>;

trait ParseResultExt {
    fn is_finished(&self) -> bool;
    fn new_node(node: ast::AstNode, next_token_idx: lexer::TokenIdx) -> Self;
    fn new_finished() -> Self;
    fn new_single_error(error: SingleParseError) -> Self;
    fn new_error_lex_error(errors: Vec<(lexer::TokenIdx, String)>) -> Self;
    fn new_error_unexpected_token(
        msg: impl Into<String>,
        start_token_idx: lexer::TokenIdx,
        inclusive_end_token_idx: lexer::TokenIdx,
    ) -> Self;
    fn new_error_unexpected_eof(msg: impl Into<String>, start_token_idx: lexer::TokenIdx) -> Self;
    fn new_error_mismatched_paren(
        lparen: lexer::TokenIdx,
        start_token_idx: lexer::TokenIdx,
    ) -> Self;
    fn add_upper_context_to_error(ctx_msg: impl Into<String>, error: ParseError) -> Self;
}

impl ParseResultExt for ParseResult {
    fn is_finished(&self) -> bool {
        matches!(self, Ok(IntermediateResult::Finished))
    }
    fn new_node(node: ast::AstNode, next_token_idx: lexer::TokenIdx) -> Self {
        Ok(IntermediateResult::Node {
            node,
            next_token_idx,
        })
    }
    fn new_finished() -> Self {
        Ok(IntermediateResult::Finished)
    }
    fn new_single_error(error: SingleParseError) -> Self {
        Err(ParseError::new_single_error(error))
    }
    fn new_error_lex_error(errors: Vec<(lexer::TokenIdx, String)>) -> Self {
        Err(ParseError::new_single_error(SingleParseError::LexErrors(
            errors,
        )))
    }
    fn new_error_unexpected_token(
        msg: impl Into<String>,
        start_token_idx: lexer::TokenIdx,
        error_token_idx: lexer::TokenIdx,
    ) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::UnexpectedToken {
                msg: msg.into(),
                ctx_start_token_idx: start_token_idx,
                error_token_idx,
            },
        ))
    }
    fn new_error_unexpected_eof(msg: impl Into<String>, start_token_idx: lexer::TokenIdx) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::UnexpectedEof {
                msg: msg.into(),
                start_token_idx,
            },
        ))
    }
    fn new_error_mismatched_paren(
        lparen: lexer::TokenIdx,
        start_token_idx: lexer::TokenIdx,
    ) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::MismatchedParentheses {
                lparen,
                start_token_idx,
            },
        ))
    }
    fn add_upper_context_to_error(ctx_msg: impl Into<String>, error: ParseError) -> Self {
        Err(error.add_error_context(ctx_msg))
    }
}

#[derive(Debug, Clone)]
pub enum ParseError {
    SingleParseError(SingleParseError),
    ErrorWithContext(Vec<ParseError>),
    AlternativeError {
        error: SingleParseError,
        children: Vec<ParseError>,
    },
}

impl ParseError {
    fn new_single_error(error: SingleParseError) -> Self {
        Self::SingleParseError(error)
    }
    fn add_error_context(self, msg: impl Into<String>) -> Self {
        let context = ParseError::new_single_error(SingleParseError::Context { msg: msg.into() });
        match self {
            Self::ErrorWithContext(mut errors) => {
                errors.push(context);
                Self::ErrorWithContext(errors)
            }
            e => Self::ErrorWithContext(vec![e, context]),
        }
    }

    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseErrors>) -> String {
        const LEN_PER_ERROR: usize = 100;
        match self {
            Self::SingleParseError(error) => error.get_string(ast),
            Self::ErrorWithContext(errors) => {
                let mut s = String::with_capacity(errors.len() * LEN_PER_ERROR);
                for e in errors {
                    s.push_str(&e.get_string(ast));
                }
                s
            }
            Self::AlternativeError { error, children } => {
                let mut s = String::with_capacity((children.len() + 1) * LEN_PER_ERROR);
                s.push_str(&error.get_string(ast));
                for e in children {
                    s.push_str(&e.get_string(ast));
                }
                s
            }
        }
    }
}

impl ast::AstErrors for ParseErrors {
    type Error = ParseError;

    fn with_capacity(capacity: usize) -> Self {
        Self::with_capacity(capacity)
    }
    fn push(&mut self, error: Self::Error) {
        self.0.push(error);
    }
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, Self>) -> String {
        self.get_string(ast)
    }
}

#[derive(Debug)]
pub struct ParseErrors(Vec<ParseError>);

impl ParseErrors {
    fn with_capacity(capacity: usize) -> Self {
        Self(Vec::with_capacity(capacity))
    }
    fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
    fn add(&mut self, error: ParseError) {
        self.0.push(error);
    }
    fn get_string(&self, ast: &ast::Ast<ParseErrors>) -> String {
        let mut s = String::with_capacity(self.0.len() * 100);
        for e in &self.0 {
            s.push_str(&e.get_string(ast));
        }
        s
    }
}

#[derive(Debug, Clone)]
pub enum SingleParseError {
    Context {
        msg: String,
    },
    UnexpectedEof {
        msg: String,
        start_token_idx: lexer::TokenIdx,
    },
    IntegerOverflow {
        token: lexer::TokenIdx,
    },
    MismatchedParentheses {
        lparen: lexer::TokenIdx,
        start_token_idx: lexer::TokenIdx,
    },
    UnexpectedToken {
        msg: String,
        ctx_start_token_idx: lexer::TokenIdx,
        error_token_idx: lexer::TokenIdx,
    },
    LexErrors(Vec<(lexer::TokenIdx, String)>),
}

impl SingleParseError {
    fn context_str() -> ColoredString {
        "context".blue()
    }
    fn error_str() -> ColoredString {
        "error".red()
    }
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseErrors>) -> String {
        match self {
            Self::Context { msg } => {
                format!("{}: {}\n", Self::context_str(), msg.blue(),)
            }
            Self::UnexpectedEof {
                msg,
                start_token_idx,
            } => {
                format!(
                    "{}: unexpected end of file, {}\n{}",
                    Self::error_str(),
                    msg.red(),
                    ast.get_diag_with_ctx_token(*start_token_idx)
                )
            }
            Self::IntegerOverflow { token: token_idx } => {
                format!(
                    "{}: integer overflow `{}`",
                    Self::error_str(),
                    ast.get_diag_with_ctx_token(*token_idx)
                )
            }
            Self::MismatchedParentheses {
                lparen,
                start_token_idx,
            } => format!(
                "{}: mismatched parentheses `{}`, at `{}`",
                Self::error_str(),
                ast.get_diag_with_ctx_token(*lparen),
                ast.get_diag_with_ctx_token(*start_token_idx),
            ),
            Self::UnexpectedToken {
                msg,
                ctx_start_token_idx: start_token_idx,
                error_token_idx,
            } => {
                format!(
                    "{}: {}\n{}",
                    Self::error_str(),
                    msg.red(),
                    ast.get_diag_with_ctx_and_error_tokens(*start_token_idx, *error_token_idx),
                )
            }
            Self::LexErrors(errors) => {
                let mut result = String::with_capacity(errors.len() * 80);
                for (error_token_idx, msg) in errors {
                    let formatted = format!(
                        "{}: {}\n{}",
                        Self::error_str(),
                        msg.red(),
                        ast.get_diag_with_error_token(*error_token_idx)
                    );
                    result.push_str(&formatted);
                }
                result
            }
        }
    }
}

pub fn parse(cu: &lexer::CompilationUnit) -> ast::Ast<ParseErrors> {
    let mut ast = ast::Ast::new(cu);

    let tokens = ast.get_tokens();
    match dp::get_lex_errors(tokens) {
        Some(lex_errors) => {
            let mut errors = ParseErrors::with_capacity(1);
            errors.add(ParseError::SingleParseError(lex_errors));
            ast.set_errors(errors);
        }
        None => dp::parse_module(&mut ast, lexer::TokenIdx::new(0)),
    }
    ast
}

#[cfg(test)]
mod test_parser {
    use super::*;

    fn no_color() {
        colored::control::set_override(false);
    }

    #[test]
    fn test_empty_ast() {
        no_color();
        for s in ["", " ", ";", "\r\n\n;", "  ;", "\r\n\n", ";;"] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu);
            // TODO: how to make error non-ignoreable?
            assert!(ast.get_error().is_none(), "ast: {}", ast);
        }
    }

    #[test]
    fn test_negative_numbers() {
        no_color();
        for (expected, test_data) in [
            ("-42;\n", vec!["-42;", "- 42;", " - 42 ;", " -42 ;"]),
            (
                "(3 - -2);\n",
                vec!["3--2;", "3- -2;", " 3  - - 2 ;", "3 - -2 ;", " 3 --2 ;"],
            ),
            (
                "(-3 - -2);\n",
                vec![
                    "-3--2;",
                    "-3- -2;",
                    "- 3 - - 2 ;",
                    " - 3  --2 ;",
                    " -3 -- 2 ;",
                ],
            ),
        ] {
            for s in test_data {
                let cu = lexer::CompilationUnit::from_string("stdin", s);
                let ast = parse(&cu);
                assert!(ast.get_error().is_none(), "ast: {}", ast);
                let result = ast.accept(&mut ast::AstPrinter::new(&ast));
                assert_eq!(result, expected, "input: {}, ast: {}", s, ast);
            }
        }
    }

    #[test]
    fn test_definitions() {
        no_color();
        let cu = lexer::CompilationUnit::from_string("stdin", "let x = 3; var y = x - 42;");
        let ast = parse(&cu);
        assert!(ast.get_error().is_none(), "ast: {}", ast);
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(
            ast.accept(printer),
            "let x = 3;\nvar y = (x - 42);\n",
            "ast: {}",
            ast
        );
    }

    #[test]
    fn test_eval() {
        no_color();
        for (s, expected) in [
            ("1;", 1i64),
            ("1+1;", 2),
            ("1-1;", 0),
            ("7%2 + 3 * (12 / ( 15 / - 3+1 - - 1) ) - 2 - 1 + 1;", -13),
        ] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu);
            assert!(ast.get_error().is_none(), "ast: {}", ast);
            let result = ast.accept(&mut ast::AstEvaluator::new(&ast));
            assert!(
                matches!(result, Ok(Some(got)) if expected == got),
                "expected: {:?}, got: {:?}",
                expected,
                result
            );
        }
    }

    #[test]
    fn test_string() {
        no_color();
        let cu = lexer::CompilationUnit::from_string(
            "stdin",
            r#"" \u{41} x\u{4f60}xy{}\u{597d}a\u{1f316}";"#,
        );
        let ast = parse(&cu);
        assert!(ast.get_error().is_none(), "ast: {}", ast);
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(ast.accept(printer), " A x你xy{}好a🌖;\n");
    }

    #[test]
    fn test_lex_errors() {
        no_color();
        let input = r#"
# what's the meaning of using unicode as identifier?

let 常量 = 42;

# `042` is not supported

let x = 042;

2^4;

# \z is not a valid escape char

let s = "abc\zdef";

let s = "abc\udef";

let s = "abc\u";

let s = "abc\u{}";

let s = "abc\u{";

let s = "abc\u{deadxy}def";

let s = "abc\u{deadbeef}def";

let s = "abc\u{ffffffffff}def";

let end = "abc;

# no \" after this line
"#;
        let cu = lexer::CompilationUnit::from_string("stdin", input);
        let ast = parse(&cu);
        assert!(ast.get_error().is_some(), "ast: {}", ast);
        let got = ast.get_error().unwrap().get_string(&ast);
        let expected = r#"error: multi-char unicode like `常` only supported in strings and comments
    4|let 常量 = 42;
     |    ^^^^
error: leading zero is not allowed
    8|let x = 042;
     |        ^^^
error: unsupported `^`
   10|2^4;
     | ^
error: invalid escape char `z`
   14|let s = "abc\zdef";
     |             ^
error: unicode should be in the format of \u{...}
   16|let s = "abc\udef";
     |              ^
error: unicode should be in the format of \u{...}
   18|let s = "abc\u";
     |              ^
error: unicode should be in the format of \u{...}, cannot be empty between `{}`
   20|let s = "abc\u{}";
     |               ^
error: only hex numbers are allowed in unicode sequence, `"` is not allowed
   22|let s = "abc\u{";
     |               ^
error: only hex numbers are allowed in unicode sequence, `x` is not allowed
   24|let s = "abc\u{deadxy}def";
     |                   ^
error: `deadbeef` is not a valid unicode code point
   26|let s = "abc\u{deadbeef}def";
     |               ^
error: `ffffffffff` is not a valid unicode code point
   28|let s = "abc\u{ffffffffff}def";
     |               ^
error: unterminated string literal
   30|let end = "abc;
     |          ^
"#;
        use pretty_assertions::assert_eq;
        assert_eq!(got, expected);
    }

    #[test]
    fn test_lex_errors_across_multi_lines() {
        no_color();
        let input = r#"
let s = "abc
def
(\z)
xyz";
"#;
        let cu = lexer::CompilationUnit::from_string("stdin", input);
        let ast = parse(&cu);
        assert!(ast.get_error().is_some(), "ast: {}", ast);
        let got = ast.get_error().unwrap().get_string(&ast);
        let expected = r#"error: invalid escape char `z`
    4|(\z)
     |  ^
"#;
        use pretty_assertions::assert_eq;
        assert_eq!(got, expected);
    }

    #[test]
    fn test_parse_error() {
        no_color();
        for (input, expected) in [
            (
                r#"
# following line should not have two `-`
# it is not supported

let x = - - 4;

let x = -a;

let x = x + ;

let +;

var;

let a xyz;

let a = ;

let a = (2 + );

let a = 2 + 3

let a = 3;
"#,
                r#"error: `-` cannot be chained
    5|let x = - - 4;
     |        ~~^
context: expect an expression after `=` for a definition
error: expected a number after `-`
    7|let x = -a;
     |        ~^
context: expect an expression after `=` for a definition
error: expected an expression
    9|let x = x + ;
     |            ^
context: operator `+` must be followed by an expression
context: expect an expression after `=` for a definition
error: expected an expression
   11|let +;
     |    ^
context: expect an expression after `let` for a definition
error: expected an expression
   13|var;
     |   ^
context: expect an expression after `var` for a definition
error: expected definition format `let ... = ...`, but could not find `=`
   15|let a xyz;
     |~~~~~~^^^
error: expected an expression
   17|let a = ;
     |        ^
context: expect an expression after `=` for a definition
error: expected an expression
   19|let a = (2 + );
     |             ^
context: operator `+` must be followed by an expression
context: not a valid expression between `()`
context: expect an expression after `=` for a definition
error: statement must end with `;`
   21|let a = 2 + 3
     |~~~~~~~~~~~~~
   22|
   23|let a = 3;
     |^^^
"#,
            ),
            (
                r#"let x = - "#,
                r#"error: unexpected end of file, expected a number after `-`
    1|let x = - 
     |        ~~
context: expect an expression after `=` for a definition
"#,
            ),
            (
                r#"let"#,
                r#"error: unexpected end of file, expect an expression after `let` for a definition
    1|let
     |~~~
"#,
            ),
            (
                r"let a ",
                r#"error: unexpected end of file, expected definition format `let ... = ...`, but could not find `=`
    1|let a 
     |~~~~~~
"#,
            ),
            (
                r#"let a = "#,
                r#"error: unexpected end of file, expect an expression after `=` for a definition
    1|let a = 
     |~~~~~~~~
"#,
            ),
            (
                r#"let a = 2 + "#,
                r#"error: unexpected end of file, operator `+` must be followed by an expression
    1|let a = 2 + 
     |          ~~
context: expect an expression after `=` for a definition
"#,
            ),
            (
                r#"let a = 2 + 3"#,
                r#"error: unexpected end of file, statement must end with `;`
    1|let a = 2 + 3
     |~~~~~~~~~~~~~
"#,
            ),
            (
                r#"let a = (2 + "#,
                r#"error: unexpected end of file, operator `+` must be followed by an expression
    1|let a = (2 + 
     |           ~~
context: not a valid expression between `()`
context: expect an expression after `=` for a definition
"#,
            ),
            (
                r#"let a = ("#,
                r#"error: unexpected end of file, nothing after `(`
    1|let a = (
     |        ~
context: expect an expression after `=` for a definition
"#,
            ),
            (
                r#"let a = (2 + 3"#,
                r#"error: unexpected end of file, no matching `)`
    1|let a = (2 + 3
     |        ~~~~~~
context: expect an expression after `=` for a definition
"#,
            ),
        ] {
            let cu = lexer::CompilationUnit::from_string("stdin", input);
            let ast = parse(&cu);
            assert!(ast.get_error().is_some(), "ast: {}", ast);
            let got = ast.get_error().unwrap().get_string(&ast);
            use pretty_assertions::assert_eq;
            assert_eq!(got, expected);
        }
    }
}
