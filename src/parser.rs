#![allow(dead_code)]

use crate::ast;
use crate::lexer;
use crate::lexer::TokenIndex;

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
enum Associativity {
    Left,
    Right,
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Clone, Copy)]
struct Precedence(i64);

impl Precedence {
    fn new(v: i64) -> Self {
        Self(v)
    }

    fn get(&self) -> i64 {
        self.0
    }
}

impl std::ops::Add<i64> for Precedence {
    type Output = Self;

    fn add(self, rhs: i64) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

#[derive(Debug)]
struct TokenTrait {
    precedence: Precedence,
    associativity: Associativity,
}

#[derive(Debug, Clone)]
pub enum ParseError {
    UnexpectedEndOfInput,
    IntegerOverflow { token: lexer::TokenIndex },
    MismatchedParentheses { lparen: lexer::TokenIndex },
    UnexpectedToken(lexer::TokenIndex),
    LexError(Vec<(lexer::TokenIndex, String)>),
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
    fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
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

#[derive(Debug)]
pub struct Parser {
    token_traits: std::collections::HashMap<lexer::TokenKind, TokenTrait>,
}

impl Parser {
    pub fn new() -> Self {
        Self {
            token_traits: [
                (
                    lexer::TokenKind::Plus,
                    Precedence::new(1),
                    Associativity::Left,
                ),
                (
                    lexer::TokenKind::Dash,
                    Precedence::new(1),
                    Associativity::Left,
                ),
                (
                    lexer::TokenKind::Star,
                    Precedence::new(2),
                    Associativity::Left,
                ),
                (
                    lexer::TokenKind::Slash,
                    Precedence::new(2),
                    Associativity::Left,
                ),
                (
                    lexer::TokenKind::Percentage,
                    Precedence::new(2),
                    Associativity::Left,
                ),
            ]
            .into_iter()
            .map(|(token_kind, precedence, associativity)| {
                (
                    token_kind,
                    TokenTrait {
                        precedence,
                        associativity,
                    },
                )
            })
            .collect(),
        }
    }

    fn get_precedence(&self, token: &lexer::Token) -> Option<(Precedence, Associativity)> {
        self.token_traits
            .get(token.get_kind())
            .map(|token_trait| (token_trait.precedence, token_trait.associativity))
    }

    fn parse_module(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        mut cur_packed_token_idx: PackedTokenIndex,
    ) -> Result<(ast::AstNode, PackedTokenIndex), ParseError> {
        let mut module_node = ast::AstNode::new_module(50);
        while let Some(token) = packed_tokens.get(cur_packed_token_idx) {
            if token.token.get_kind() == &lexer::TokenKind::SemiColon {
                cur_packed_token_idx += 1;
                continue;
            }
            let (statement_node, new_cur_packed_token_idx) =
                self.parse_statement(ast, packed_tokens, cur_packed_token_idx)?;
            let Some(statement) = statement_node else {
                break;
            };
            let statement_node_idx = ast.push(statement);
            module_node.add_statement_to_module(ast, statement_node_idx);
            cur_packed_token_idx = new_cur_packed_token_idx;
        }
        Ok((module_node, cur_packed_token_idx))
    }

    fn parse_statement(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        cur_packed_token_idx: PackedTokenIndex,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let (expr, cur_packed_token_idx) =
            self.parse_expression(ast, packed_tokens, cur_packed_token_idx, Precedence::new(0))?;
        match expr {
            Some(expr) => match expr {
                ast::AstNode::Expression(_) => Ok((
                    Some(ast::AstNode::Statement {
                        expression_node_idx: ast.push(expr),
                    }),
                    cur_packed_token_idx,
                )),
                _ => panic!("BUG: expected expression, got {:?}", expr),
            },
            None => Ok((None, cur_packed_token_idx)),
        }
    }

    fn parse_expression(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        cur_packed_token_idx: PackedTokenIndex,
        min_precedence: Precedence,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let (mut lhs, mut cur_packed_token_idx) =
            self.parse_primary(ast, packed_tokens, cur_packed_token_idx)?;
        if lhs.is_none() {
            return Ok((None, cur_packed_token_idx));
        }

        while let Some(op) = packed_tokens.get(cur_packed_token_idx) {
            if op.token.get_kind() == &lexer::TokenKind::SemiColon {
                return Ok((lhs, cur_packed_token_idx + 1));
            }
            if op.token.get_kind() == &lexer::TokenKind::RParen {
                return Ok((lhs, cur_packed_token_idx));
            }
            let (precedence, associativity) = self
                .get_precedence(&op.token)
                .ok_or(ParseError::UnexpectedToken(op.token_idx))?;
            if precedence < min_precedence {
                break;
            }
            let min_precedence = if associativity == Associativity::Left {
                precedence + 1
            } else {
                precedence
            };

            let (rhs, new_cur_packed_token_idx) = self.parse_expression(
                ast,
                packed_tokens,
                cur_packed_token_idx + 1,
                min_precedence,
            )?;
            if rhs.is_none() {
                cur_packed_token_idx = new_cur_packed_token_idx;
                break;
            }
            cur_packed_token_idx = new_cur_packed_token_idx;
            let lhs_node_idx = ast.push(lhs.unwrap());
            let rhs_node_idx = ast.push(rhs.unwrap());
            lhs = Some(ast::AstNode::Expression(ast::Expr::BinaryOp {
                operator: op.token_idx,
                lhs: lhs_node_idx,
                rhs: rhs_node_idx,
            }));
        }

        Ok((lhs, cur_packed_token_idx))
    }

    fn parse_primary(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        mut cur_packed_token_idx: PackedTokenIndex,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let Some(packed_token) = packed_tokens.get(cur_packed_token_idx) else {
            return Ok((None, cur_packed_token_idx));
        };

        match packed_token.token.get_kind() {
            lexer::TokenKind::I64 => Ok((
                Some(ast::AstNode::Expression(ast::Expr::I64(
                    packed_token.token_idx,
                ))),
                cur_packed_token_idx + 1,
            )),
            // don't allow chained unary operators
            lexer::TokenKind::Dash => packed_tokens.get(cur_packed_token_idx + 1).map_or(
                Err(ParseError::UnexpectedEndOfInput),
                |PackedToken {
                     token_idx: operand_token_idx,
                     token: operand_token,
                 }| match operand_token.get_kind() {
                    lexer::TokenKind::I64 => Ok((
                        Some(ast::AstNode::Expression(ast::Expr::UnaryOp {
                            operator: packed_token.token_idx,
                            operand: ast
                                .push(ast::AstNode::Expression(ast::Expr::I64(*operand_token_idx))),
                        })),
                        cur_packed_token_idx + 2,
                    )),
                    _ => Err(ParseError::UnexpectedToken(*operand_token_idx)),
                },
            ),
            lexer::TokenKind::LParen => {
                let (expr, new_cur_packed_token_idx) = self.parse_expression(
                    ast,
                    packed_tokens,
                    cur_packed_token_idx + 1,
                    Precedence::new(0),
                )?;
                cur_packed_token_idx = new_cur_packed_token_idx;
                match packed_tokens.get(cur_packed_token_idx) {
                    Some(token) if token.token.get_kind() == &lexer::TokenKind::RParen => {
                        Ok((expr, cur_packed_token_idx + 1))
                    }
                    _ => Err(ParseError::MismatchedParentheses {
                        lparen: packed_token.token_idx,
                    }),
                }
            }
            lexer::TokenKind::SemiColon => Ok((None, cur_packed_token_idx + 1)),
            lexer::TokenKind::StringLiteral { content } => Ok((
                Some(ast::AstNode::Expression(ast::Expr::StringLiteral {
                    token_idx: packed_token.token_idx,
                    content: content.clone(),
                })),
                cur_packed_token_idx + 1,
            )),
            _ => Err(ParseError::UnexpectedToken(packed_token.token_idx)),
        }
    }

    pub fn parse<'cu>(&self, cu: &'cu lexer::CompilationUnit) -> Result<ast::Ast<'cu>, ParseError> {
        fn partition_packed_tokens_and_lex_errors(
            tokens: lexer::Tokens,
        ) -> (PackedTokens, Vec<(TokenIndex, String)>) {
            let (tokens, invalid_tokens) = tokens
                .into_iter()
                .enumerate()
                .filter(|(_, token)| {
                    token.get_kind() != &lexer::TokenKind::Spaces
                        && token.get_kind() != &lexer::TokenKind::NewLine
                })
                .map(|(i, token)| (TokenIndex::new(i), token))
                .partition(|(_, token)| {
                    !matches!(token.get_kind(), lexer::TokenKind::Invalid { .. })
                });
            (
                tokens,
                invalid_tokens
                    .into_iter()
                    .map(|PackedToken { token_idx, token }| match token.get_kind() {
                        lexer::TokenKind::Invalid { msg } => (token_idx, msg.clone()),
                        _ => panic!("BUG: expected invalid token, got {:?}", token),
                    })
                    .collect(),
            )
        }

        let mut ast = ast::Ast::new(cu);

        let (tokens, lex_errors) = partition_packed_tokens_and_lex_errors(ast.get_tokens().clone());
        if !lex_errors.is_empty() {
            return Err(ParseError::LexError(lex_errors));
        }

        let (module, _cur_packed_token_idx) = self
            .parse_module(&mut ast, &tokens, PackedTokenIndex::new(0))
            .unwrap_or_else(|e| panic!("failed to parse: {:?}", e));
        ast.set_module(module);
        Ok(ast)
    }
}

impl Default for Parser {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod test_parser {
    use super::*;

    #[test]
    fn test_empty_ast() {
        for s in ["", " ", ";", "\r\n\n;", "  ;", "\r\n\n"] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let parser = Parser::new();
            let ast = parser
                .parse(&cu)
                .unwrap_or_else(|e| panic!("failed to parse: \"{}\", {:?}", s, e));
            assert!(ast.is_empty(), "ast: {}", ast);
        }
    }

    #[test]
    fn test_negative_numbers() {
        for (expected, test_data) in [
            ("-42", vec!["-42;", "- 42;", " - 42 ;", " -42 ;"]),
            (
                "(3 - -2)",
                vec!["3--2;", "3- -2;", " 3  - - 2 ;", "3 - -2 ;", " 3 --2 ;"],
            ),
            (
                "(-3 - -2)",
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
                let parser = Parser::new();
                let ast = parser
                    .parse(&cu)
                    .unwrap_or_else(|e| panic!("failed to parse: \"{}\", {:?}", s, e));
                let result = ast.accept(&mut ast::AstPrinter::new(&ast));
                assert_eq!(result, expected, "ast: {}", ast);
            }
        }
    }

    #[test]
    fn test_eval() {
        for (s, expected) in [("1;", 1i64), ("1+1;", 2), ("1-1;", 0)] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let parser = Parser::new();
            let ast = parser
                .parse(&cu)
                .unwrap_or_else(|e| panic!("failed to parse: \"{}\", {:?}", s, e));
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
        let parser = Parser::new();
        let ast = parser
            .parse(&cu)
            .unwrap_or_else(|e| panic!("failed to parse: {:?}", e));
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
            r#"" \u{41} x\u{4f60}xy{}\u{597d}a\u{1f316}""#,
        );
        let parser = Parser::new();
        let ast = parser.parse(&cu).unwrap();
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(ast.accept(printer), " A x你xy{}好a🌖");
    }
}
