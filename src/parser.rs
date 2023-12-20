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
    fn new_error(error: ParseError) -> Self;
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
    fn new_error(error: ParseError) -> Self {
        Err(error)
    }
}

struct ParseErrors {
    error: ParseError,
    children: Vec<ParseErrors>,
}

#[derive(Debug, Clone)]
pub enum ParseError {
    UnexpectedEndOfInput {
        msg: String,
        start_token_idx: lexer::TokenIndex,
    },
    IntegerOverflow {
        token: lexer::TokenIndex,
    },
    MismatchedParentheses {
        lparen: lexer::TokenIndex,
    },
    UnexpectedToken {
        msg: String,
        start_token_idx: lexer::TokenIndex,
        inclusive_end_token_idx: lexer::TokenIndex,
    },
    LexError(Vec<(lexer::TokenIndex, String)>),
}

impl ParseError {
    fn get_string(&self, ast: &ast::Ast) -> String {
        match self {
            Self::UnexpectedEndOfInput { msg, .. } => format!("unexpected end of input, {}", msg),
            Self::IntegerOverflow { token } => {
                format!("integer overflow: `{}`, {:?}", ast.to_string(*token), self)
            }
            Self::MismatchedParentheses { lparen } => format!(
                "mismatched parentheses: `{}`, {:?}",
                ast.to_string(*lparen),
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
                    ast.to_string(*inclusive_end_token_idx),
                    self
                )
            }
            Self::LexError(errors) => format!(
                "lex error: {}",
                errors
                    .iter()
                    .map(|(token_idx, msg)| {
                        format!("`{}`: {}\n", ast.to_string(*token_idx), msg)
                    })
                    .collect::<Vec<_>>()
                    .join(", ")
            ),
        }
    }
}

#[derive(Debug)]
struct PackedToken {
    token_idx: lexer::TokenIndex,
    token: lexer::Token,
}

#[derive(Debug, Default)]
struct PackedTokens(Vec<PackedToken>);

impl Extend<(lexer::TokenIndex, lexer::Token)> for PackedTokens {
    fn extend<T: IntoIterator<Item = (lexer::TokenIndex, lexer::Token)>>(&mut self, iter: T) {
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
pub fn parse(cu: &lexer::CompilationUnit) -> Result<ast::Ast, ParseError> {
    let mut ast = ast::Ast::new(cu);

    // return ALL lexing errors
    let (tokens, lex_errors) = dp::partition_packed_tokens_and_lex_errors(ast.get_tokens().clone());
    if !lex_errors.is_empty() {
        return Err(ParseError::LexError(lex_errors));
    }

    // create AST tree
    let module_node = dp::parse_module(&mut ast, &tokens, PackedTokenIndex::new(0))?;
    ast.set_module(module_node);

    Ok(ast)
}

#[cfg(test)]
mod test_parser {
    use super::*;

    #[test]
    fn test_empty_ast() {
        for s in ["", " ", ";", "\r\n\n;", "  ;", "\r\n\n", ";;"] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu);
            assert!(
                matches!(ast, Ok(ast::Ast { .. })),
                "input: `{}`, ast: {:?}",
                s,
                ast
            );
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
                let ast =
                    parse(&cu).unwrap_or_else(|e| panic!("failed to parse: \"{}\", {:?}", s, e));
                let result = ast.accept(&mut ast::AstPrinter::new(&ast));
                assert_eq!(result, expected, "input: {}, ast: {}", s, ast);
            }
        }
    }

    #[test]
    fn test_eval() {
        for (s, expected) in [("1;", 1i64), ("1+1;", 2), ("1-1;", 0)] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu).unwrap_or_else(|e| panic!("failed to parse: \"{}\", {:?}", s, e));
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
        let ast = parse(&cu).unwrap_or_else(|e| panic!("failed to parse: {:?}", e));
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
        let ast = parse(&cu).unwrap();
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(ast.accept(printer), " A x你xy{}好a🌖;\n");
    }

    #[test]
    fn test_definitions() {
        let cu = lexer::CompilationUnit::from_string("stdin", "let x = 3; var y = x - 42;");
        let ast = parse(&cu).unwrap();
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(
            ast.accept(printer),
            "let x = 3;\nvar y = (x - 42);\n",
            "ast: {}",
            ast
        );
    }
}
