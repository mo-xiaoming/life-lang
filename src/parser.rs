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
        inclusive_end_token_idx: lexer::TokenIdx,
    ) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::UnexpectedToken {
                msg: msg.into(),
                start_token_idx,
                error_token_idx: inclusive_end_token_idx,
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
    // B calls A
    // before [A, ...]
    // after [A, B, ...]
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

    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseError>) -> String {
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

impl ast::AstError for ParseError {
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, Self>) -> String {
        self.get_string(ast)
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
        start_token_idx: lexer::TokenIdx,
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
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseError>) -> String {
        match self {
            Self::Context { msg } => {
                format!("{}: {}\n", Self::context_str(), msg.blue(),)
            }
            Self::UnexpectedEof { msg, .. } => {
                format!("{}: unexpected end of input, {}", Self::error_str(), msg)
            }
            Self::IntegerOverflow { token: token_idx } => {
                format!(
                    "{}: integer overflow `{}`",
                    Self::error_str(),
                    ast.get_diagnostics(*token_idx)
                )
            }
            Self::MismatchedParentheses {
                lparen,
                start_token_idx,
            } => format!(
                "{}: mismatched parentheses `{}`, at `{}`",
                Self::error_str(),
                ast.get_diagnostics(*lparen),
                ast.get_diagnostics(*start_token_idx),
            ),
            Self::UnexpectedToken {
                msg,
                start_token_idx,
                error_token_idx,
            } => {
                format!(
                    "{}: {}\n{}",
                    Self::error_str(),
                    msg.red(),
                    ast.get_diagnostics_with_error_token(*start_token_idx, *error_token_idx),
                )
            }
            Self::LexErrors(errors) => format!(
                "{}: {}",
                Self::error_str(),
                errors
                    .iter()
                    .map(|(token_idx, msg)| {
                        format!("`{}`: {}\n", ast.get_diagnostics(*token_idx), msg)
                    })
                    .collect::<Vec<_>>()
                    .join(", ")
            ),
        }
    }
}

//impl Extend<(lexer::TokenIdx, lexer::Token)> for Tokens {
//    fn extend<T: IntoIterator<Item = (lexer::TokenIdx, lexer::Token)>>(&mut self, iter: T) {
//        let iter = iter.into_iter();
//        let (lower_bound, _) = iter.size_hint();
//        self.0.reserve(lower_bound);
//        for (index, token) in iter {
//            self.0.push(PackedToken {
//                token_idx: index,
//                token,
//            });
//        }
//    }
//}

pub fn parse(cu: &lexer::CompilationUnit) -> ast::Ast<ParseError> {
    let mut ast = ast::Ast::new(cu);

    let tokens = ast.get_tokens();
    match dp::get_lex_errors(tokens) {
        Some(lex_errors) => {
            ast.set_error(ParseError::SingleParseError(lex_errors));
        }
        None => dp::parse_module(&mut ast, lexer::TokenIdx::new(0)),
    }
    ast
}

#[cfg(test)]
mod test_parser {
    use super::*;

    struct ColorOff;
    impl ColorOff {
        fn new() -> Self {
            colored::control::set_override(false);
            Self
        }
    }
    impl Drop for ColorOff {
        fn drop(&mut self) {
            colored::control::unset_override();
        }
    }

    #[test]
    fn test_empty_ast() {
        for s in ["", " ", ";", "\r\n\n;", "  ;", "\r\n\n", ";;"] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu);
            // TODO: how to make error non-ignoreable?
            assert!(ast.get_error().is_none(), "ast: {}", ast);
        }
    }

    #[test]
    fn test_negative_numbers() {
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
    fn test_negative_number_error() {
        let _color_off = ColorOff::new();

        let input = r#"
# following line should not have two `-`
# it is not supported

let x = - - 4;
"#;
        let cu = lexer::CompilationUnit::from_string("stdin", input);
        let ast = parse(&cu);
        assert!(ast.get_error().is_some(), "ast: {}", ast);
        let got = ast.get_error().unwrap().get_string(&ast);
        println!("{}", got);
        let expected = r#"error: `-` cannot be chained
    5|let x = - - 4;
     |        ~~^
context: an expression must start with an expression
context: expect an expression after `=` for a definition
"#;
        assert_eq!(got, expected);
    }

    #[test]
    fn test_eval() {
        for (s, expected) in [("1;", 1i64), ("1+1;", 2), ("1-1;", 0)] {
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
    fn test_parser() {
        let cu = lexer::CompilationUnit::from_string(
            "stdin",
            "7%2 + 3 * (12 / ( 15 / - 3+1 - - 1) ) - 2 - 1 + 1;",
        );
        let ast = parse(&cu);
        assert!(ast.get_error().is_none(), "ast: {}", ast);
        let result = ast.accept(&mut ast::AstEvaluator::new(&ast));
        assert!(
            matches!(result, Ok(Some(got)) if got == -13),
            "expected: -13, got: {:?}",
            result
        );
    }

    #[test]
    fn test_parsing_string() {
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
    fn test_definitions() {
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
}
