#![allow(unused)]

mod dp;

use crate::{ast, lexer};

#[derive(Debug)]
enum IntermediateResult {
    Finished,
    Node {
        node: ast::AstNode,
        next_packed_token_idx: PackedTokenIndex,
    },
}

type ParseResult = Result<IntermediateResult, ParseError>;

trait ParseResultExt {
    fn is_finished(&self) -> bool;
    fn new_node(node: ast::AstNode, next_token_idx: PackedTokenIndex) -> Self;
    fn new_finished() -> Self;
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
    fn add_upper_context_to_error(ctx: ParseError, error: ParseError) -> Self;
}

impl ParseResultExt for ParseResult {
    fn is_finished(&self) -> bool {
        matches!(self, Ok(IntermediateResult::Finished))
    }
    fn new_node(node: ast::AstNode, next_token_idx: PackedTokenIndex) -> Self {
        Ok(IntermediateResult::Node {
            node,
            next_packed_token_idx: next_token_idx,
        })
    }
    fn new_finished() -> Self {
        Ok(IntermediateResult::Finished)
    }
    fn new_error_lex_error(errors: Vec<(lexer::TokenIdx, String)>) -> Self {
        Err(ParseError::new_single_error(SingleParseError::LexError(
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
                inclusive_end_token_idx,
            },
        ))
    }
    fn new_error_unexpected_eof(msg: impl Into<String>, start_token_idx: lexer::TokenIdx) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::UnexpectedEndOfInput {
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
    fn add_upper_context_to_error(ctx: ParseError, error: ParseError) -> Self {
        Err(error.add_error_context(ctx))
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
    fn add_error_context(self, context: ParseError) -> Self {
        assert!(matches!(
            context,
            Self::SingleParseError(SingleParseError::Context { .. })
        ));
        match self {
            Self::ErrorWithContext(mut errors) => {
                errors.push(context);
                Self::ErrorWithContext(errors)
            }
            e => Self::ErrorWithContext(vec![e, context]),
        }
    }
    // B tries X and Y
    // X returns [X]
    // Y returns [Y]
    // after [B, [X, Y]]
    fn merge_alternative_errors(context: ParseError, children: Vec<ParseError>) -> Self {
        match context {
            Self::SingleParseError(error) => Self::AlternativeError { error, children },
            _ => panic!("`merge_alternative_error`s should only be called on a `SingleParseError`"),
        }
    }

    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseError>) -> String {
        match self {
            Self::SingleParseError(error) => error.get_string(ast),
            Self::ErrorWithContext(errors) => format!(
                "multiple errors: {}",
                errors
                    .iter()
                    .map(|error| error.get_string(ast))
                    .collect::<Vec<_>>()
                    .join(", ")
            ),
            Self::AlternativeError { error, children } => format!(
                "alternative errors: {}, children: {}",
                error.get_string(ast),
                children
                    .iter()
                    .map(|error| error.get_string(ast))
                    .collect::<Vec<_>>()
                    .join(", ")
            ),
        }
    }
}

impl ast::AstError for ParseError {
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, Self>) -> String {
        self.get_string(ast)
    }
}

#[derive(Debug)]
struct MultiParseErrors {
    top_error: SingleParseError,
    children: Vec<MultiParseErrors>,
}

#[derive(Debug, Clone)]
pub enum SingleParseError {
    Context {
        msg: String,
        start_token_idx: lexer::TokenIdx,
    },
    UnexpectedEndOfInput {
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
        inclusive_end_token_idx: lexer::TokenIdx,
    },
    LexError(Vec<(lexer::TokenIdx, String)>),
}

impl SingleParseError {
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseError>) -> String {
        match self {
            Self::Context {
                msg,
                start_token_idx,
            } => {
                format!(
                    "context: `{}`, from `{}`",
                    msg,
                    ast.get_string_unchecked(*start_token_idx),
                )
            }
            Self::UnexpectedEndOfInput { msg, .. } => format!("unexpected end of input, {}", msg),
            Self::IntegerOverflow { token: token_idx } => {
                format!(
                    "integer overflow: `{}`, {:?}",
                    ast.get_string_unchecked(*token_idx),
                    self
                )
            }
            Self::MismatchedParentheses {
                lparen,
                start_token_idx,
            } => format!(
                "mismatched parentheses: `{}`, at `{}`, {:?}",
                ast.get_string_unchecked(*lparen),
                ast.get_string_unchecked(*start_token_idx),
                self
            ),
            Self::UnexpectedToken {
                msg,
                start_token_idx: _start_token_idx,
                inclusive_end_token_idx,
            } => {
                format!(
                    "unexpected token: {}, but got `{}` instead, {:?}",
                    msg,
                    ast.get_string_unchecked(*inclusive_end_token_idx),
                    self
                )
            }
            Self::LexError(errors) => format!(
                "lex error: {}",
                errors
                    .iter()
                    .map(|(token_idx, msg)| {
                        format!("`{}`: {}\n", ast.get_string_unchecked(*token_idx), msg)
                    })
                    .collect::<Vec<_>>()
                    .join(", ")
            ),
        }
    }
}

#[derive(Debug)]
struct PackedToken {
    token_idx: lexer::TokenIdx,
    token: lexer::Token,
}

#[derive(Debug, Default)]
struct PackedTokens(Vec<PackedToken>);

impl Extend<(lexer::TokenIdx, lexer::Token)> for PackedTokens {
    fn extend<T: IntoIterator<Item = (lexer::TokenIdx, lexer::Token)>>(&mut self, iter: T) {
        let iter = iter.into_iter();
        let (lower_bound, _) = iter.size_hint();
        self.0.reserve(lower_bound);
        for (index, token) in iter {
            self.0.push(PackedToken {
                token_idx: index,
                token,
            });
        }
    }
}

impl PackedTokens {
    fn into_iter(self) -> std::vec::IntoIter<PackedToken> {
        self.0.into_iter()
    }
    fn get(&self, idx: PackedTokenIndex) -> Option<&PackedToken> {
        self.0.get(idx.get())
    }
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Clone, Copy)]
struct PackedTokenIndex(usize);

impl PackedTokenIndex {
    fn new(idx: usize) -> Self {
        Self(idx)
    }

    fn get(&self) -> usize {
        self.0
    }
}

impl std::ops::Add<usize> for PackedTokenIndex {
    type Output = Self;

    fn add(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

impl std::ops::AddAssign<usize> for PackedTokenIndex {
    fn add_assign(&mut self, rhs: usize) {
        *self = *self + rhs;
    }
}

impl std::ops::Sub<usize> for PackedTokenIndex {
    type Output = Self;

    fn sub(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_sub(rhs).unwrap())
    }
}

// recursive decendent parser
pub fn parse(cu: &lexer::CompilationUnit) -> ast::Ast<ParseError> {
    let mut ast = ast::Ast::new(cu);

    let (tokens, lex_errors) = dp::partition_packed_tokens_and_lex_errors(ast.get_tokens().clone());
    if !lex_errors.is_empty() {
        ast.set_error(ParseError::SingleParseError(SingleParseError::LexError(
            lex_errors,
        )));
    } else {
        let result = dp::parse_module(&mut ast, &tokens, PackedTokenIndex::new(0));
        match result {
            Ok(module_node) => {
                ast.set_module(module_node);
            }
            Err(e) => {
                ast.set_error(e);
            }
        }
    }
    ast
}

#[cfg(test)]
mod test_parser {
    use super::*;

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
        let cu = lexer::CompilationUnit::from_string("stdin", "- - 42;");
        let ast = parse(&cu);
        assert!(ast.get_error().is_some(), "ast: {}", ast);
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
