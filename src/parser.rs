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
        mut next_token_idx: PackedTokenIndex,
    ) -> Result<(ast::AstNode, PackedTokenIndex), ParseError> {
        let mut module_node = ast::AstNode::new_module(50);
        while let Some(token) = packed_tokens.get(next_token_idx) {
            if token.token.get_kind() == &lexer::TokenKind::SemiColon {
                next_token_idx += 1;
                continue;
            }
            let (statement_node, new_next_token_idx) =
                self.parse_statement(ast, packed_tokens, next_token_idx)?;
            let Some(statement) = statement_node else {
                break;
            };
            let statement_node_idx = ast.push(statement);
            module_node.add_statement_to_module(ast, statement_node_idx);
            next_token_idx = new_next_token_idx;
        }
        Ok((module_node, next_token_idx))
    }

    fn try_parse_definition(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        start_packed_token_idx: PackedTokenIndex,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        // has something
        let Some(packed_token) = packed_tokens.get(start_packed_token_idx) else {
            return Ok((None, start_packed_token_idx));
        };
        let origin_start_token_idx = packed_token.token_idx;

        // must be either `let` or `var`
        let kind = packed_token.token.get_kind();
        if kind != &lexer::TokenKind::KwLet && kind != &lexer::TokenKind::KwVar {
            return Err(ParseError::UnexpectedToken {
                msg: "expression should start with `let` or `var`, but got".to_owned(),
                start_token_idx: packed_token.token_idx,
                inclusive_end_token_idx: packed_token.token_idx,
            });
        }
        let kw_token_idx = packed_token.token_idx;
        let after_kw_packed_token_idx = start_packed_token_idx + 1;

        // lhs is an expression. TODO: must be something can be assigned to
        let (lhs_expr, after_lhs_packed_token_idx) = self.parse_expression(
            ast,
            packed_tokens,
            after_kw_packed_token_idx,
            Precedence::new(0),
        )?;

        // must be `=`
        let Some(packed_token) = packed_tokens.get(after_lhs_packed_token_idx) else {
            return Err(ParseError::UnexpectedEndOfInput {
                msg: "expected definition but could not find `=`".to_owned(),
                start_token_idx: origin_start_token_idx,
            });
        };
        if !matches!(packed_token.token.get_kind(), &lexer::TokenKind::Equal) {
            return Err(ParseError::UnexpectedToken {
                msg: "definition expects `=`".to_owned(),
                start_token_idx: origin_start_token_idx,
                inclusive_end_token_idx: packed_token.token_idx,
            });
        }
        let eq_token_idx = packed_token.token_idx;
        let after_eq_packed_token_idx = after_lhs_packed_token_idx + 1;

        // rhs is an expression
        let (rhs_expr, after_rhs_packed_token_idx) = self.parse_expression(
            ast,
            packed_tokens,
            after_eq_packed_token_idx,
            Precedence::new(0),
        )?;

        // ends with `;`
        let Some(packed_token) = packed_tokens.get(after_rhs_packed_token_idx) else {
            return Err(ParseError::UnexpectedEndOfInput {
                msg: "expected definition but could not find `;`".to_owned(),
                start_token_idx: origin_start_token_idx,
            });
        };
        // `;` should be checked in parse_statement
        if !matches!(packed_token.token.get_kind(), lexer::TokenKind::SemiColon) {
            return Err(ParseError::UnexpectedToken {
                msg: "definition is a statement, it must ends with `;`".to_owned(),
                start_token_idx: origin_start_token_idx,
                inclusive_end_token_idx: packed_token.token_idx,
            });
        }

        // return
        Ok((
            Some(ast::AstNode::Statement(ast::Stat::Definition {
                kw: kw_token_idx,
                lhs_expression_node_idx: ast.push(lhs_expr.unwrap()),
                eq: eq_token_idx,
                rhs_expression_node_idx: ast.push(rhs_expr.unwrap()),
            })),
            after_rhs_packed_token_idx + 1,
        ))
    }

    fn try_parse_expression(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        next_token_idx: PackedTokenIndex,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let (expr, next_token_idx) =
            self.parse_expression(ast, packed_tokens, next_token_idx, Precedence::new(0))?;
        match expr {
            Some(expr) => match expr {
                ast::AstNode::Expression(_) => Ok((
                    Some(ast::AstNode::Statement(ast::Stat::Expression(
                        ast.push(expr),
                    ))),
                    next_token_idx,
                )),
                _ => panic!("BUG: expected expression, got {:?}", expr),
            },
            None => Ok((None, next_token_idx)),
        }
    }

    fn parse_statement(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        next_token_idx: PackedTokenIndex,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let mut errors = Vec::with_capacity(5);
        match self.try_parse_definition(ast, packed_tokens, next_token_idx) {
            Err(e) => errors.push(e),
            a => return a,
        }

        match self.try_parse_expression(ast, packed_tokens, next_token_idx) {
            Err(e) => errors.push(e),
            a => return a,
        }

        assert!(!errors.is_empty());
        if errors.len() > 1 {
            for e in errors.iter() {
                dbg!(format!("{}, {:?}", e.get_string(ast), e));
            }
        }
        Err(errors[0].clone())
    }

    fn parse_expression(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        start_packed_token_idx: PackedTokenIndex,
        min_precedence: Precedence,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let (mut lhs, after_lhs_packed_token_idx) =
            self.parse_primary(ast, packed_tokens, start_packed_token_idx)?;
        if lhs.is_none() {
            return Ok((None, start_packed_token_idx));
        };

        let mut next_packed_token_idx = after_lhs_packed_token_idx;
        while let Some(op) = packed_tokens.get(next_packed_token_idx) {
            if [
                lexer::TokenKind::Equal,
                lexer::TokenKind::SemiColon,
                lexer::TokenKind::RParen,
            ]
            .contains(op.token.get_kind())
            {
                return Ok((lhs, next_packed_token_idx));
            }
            let (precedence, associativity) =
                self.get_precedence(&op.token)
                    .ok_or_else(|| ParseError::UnexpectedToken {
                        msg: "expect an operator makes sense here".to_owned(),
                        start_token_idx: packed_tokens
                            .get(start_packed_token_idx)
                            .unwrap()
                            .token_idx,
                        inclusive_end_token_idx: op.token_idx,
                    })?;
            if precedence < min_precedence {
                break;
            }
            let min_precedence = if associativity == Associativity::Left {
                precedence + 1
            } else {
                precedence
            };

            let (rhs, after_rhs_packed_token_idx) = self.parse_expression(
                ast,
                packed_tokens,
                next_packed_token_idx + 1,
                min_precedence,
            )?;
            if rhs.is_none() {
                return Err(ParseError::UnexpectedEndOfInput {
                    msg: "expected an expression after operator".to_owned(),
                    start_token_idx: op.token_idx,
                });
            };
            let lhs_node_idx = ast.push(lhs.unwrap());
            let rhs_node_idx = ast.push(rhs.unwrap());
            lhs = Some(ast::AstNode::Expression(ast::Expr::BinaryOp {
                operator: op.token_idx,
                lhs: lhs_node_idx,
                rhs: rhs_node_idx,
            }));
            next_packed_token_idx = after_rhs_packed_token_idx;
        }

        Ok((lhs, next_packed_token_idx))
    }

    fn parse_primary(
        &self,
        ast: &mut ast::Ast,
        packed_tokens: &PackedTokens,
        start_packed_token_idx: PackedTokenIndex,
    ) -> Result<(Option<ast::AstNode>, PackedTokenIndex), ParseError> {
        let Some(packed_token) = packed_tokens.get(start_packed_token_idx) else {
            return Ok((None, start_packed_token_idx));
        };

        match packed_token.token.get_kind() {
            lexer::TokenKind::I64 => Ok((
                Some(ast::AstNode::Expression(ast::Expr::I64(
                    packed_token.token_idx,
                ))),
                start_packed_token_idx + 1,
            )),
            lexer::TokenKind::Identifier => Ok((
                Some(ast::AstNode::Expression(ast::Expr::Identifier(
                    packed_token.token_idx,
                ))),
                start_packed_token_idx + 1,
            )),
            // don't allow chained unary operators
            lexer::TokenKind::Dash => packed_tokens.get(start_packed_token_idx + 1).map_or(
                Err(ParseError::UnexpectedEndOfInput {
                    msg: "negative number expects digits after `-`".to_owned(),
                    start_token_idx: packed_token.token_idx,
                }),
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
                        start_packed_token_idx + 2,
                    )),
                    _ => Err(ParseError::UnexpectedToken {
                        msg: "unary `-` only accepts integer".to_owned(),
                        start_token_idx: packed_token.token_idx,
                        inclusive_end_token_idx: *operand_token_idx,
                    }),
                },
            ),
            lexer::TokenKind::LParen => {
                let (expr, after_expr_packed_token_idx) = self.parse_expression(
                    ast,
                    packed_tokens,
                    start_packed_token_idx + 1,
                    Precedence::new(0),
                )?;
                match packed_tokens.get(after_expr_packed_token_idx) {
                    Some(token) if token.token.get_kind() == &lexer::TokenKind::RParen => {
                        Ok((expr, after_expr_packed_token_idx + 1))
                    }
                    _ => Err(ParseError::MismatchedParentheses {
                        lparen: packed_token.token_idx,
                    }),
                }
            }
            lexer::TokenKind::SemiColon => Ok((None, start_packed_token_idx + 1)),
            lexer::TokenKind::StringLiteral { content } => Ok((
                Some(ast::AstNode::Expression(ast::Expr::StringLiteral {
                    token_idx: packed_token.token_idx,
                    content: content.clone(),
                })),
                start_packed_token_idx + 1,
            )),
            _ => Err(ParseError::UnexpectedToken {
                msg: "expected an expression, but got".to_owned(),
                start_token_idx: packed_token.token_idx,
                inclusive_end_token_idx: packed_token.token_idx,
            }),
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

        let (module, _next_token_idx) = self
            .parse_module(&mut ast, &tokens, PackedTokenIndex::new(0))
            .unwrap_or_else(|e| panic!("failed to parse: {}", e.get_string(&ast)));
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

    #[test]
    fn test_definitions() {
        let cu = lexer::CompilationUnit::from_string("stdin", "let x = 3; var y = x - 42;");
        let parser = Parser::new();
        let ast = parser.parse(&cu).unwrap();
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(
            ast.accept(printer),
            "let x = 3;\nvar y = (x - 42);\n",
            "ast: {}",
            ast
        );
    }
}
