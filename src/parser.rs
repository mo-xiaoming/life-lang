#![allow(dead_code)]

use crate::lexer;

#[derive(Debug)]
pub struct Ast<'cu> {
    cu: &'cu lexer::CompilationUnit,
    root: Option<AstNodeIndex>,
    nodes: AstNodes,
    tokens: lexer::Tokens,
}

impl<'cu> Ast<'cu> {
    fn get_str(&self, idx: AstNodeIndex) -> String {
        self.nodes.get_str(self, idx)
    }
    fn nodes_len(&self) -> usize {
        self.nodes.len()
    }
    pub fn accept<V: AstNodesVisitor>(&self, visitor: &mut V) -> Option<V::Output> {
        self.root.map(|root| visitor.visit(self, root))
    }
}

#[derive(Debug)]
enum AstNode {
    EndOfLine(lexer::TokenIndex),
    Number(lexer::TokenIndex),
    BinaryOp {
        op: lexer::TokenIndex,
        lhs: AstNodeIndex,
        rhs: AstNodeIndex,
    },
    UnaryOp {
        op: lexer::TokenIndex,
        rhs: AstNodeIndex,
    },
}

impl AstNode {
    fn get_str(&self, ast: &Ast) -> String {
        match self {
            AstNode::EndOfLine(token_idx) | AstNode::Number(token_idx) => {
                ast.tokens.get(*token_idx).get_str(ast.cu).to_owned()
            }
            AstNode::BinaryOp { op, lhs, rhs } => {
                let op_str = ast.tokens.get(*op).get_str(ast.cu);
                let lhs_str = ast.get_str(*lhs);
                let rhs_str = ast.get_str(*rhs);
                format!("({} {} {})", lhs_str, op_str, rhs_str)
            }
            AstNode::UnaryOp { op, rhs } => {
                let op_str = ast.tokens.get(*op).get_str(ast.cu);
                let rhs_str = ast.get_str(*rhs);
                format!("({} {})", op_str, rhs_str)
            }
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AstNodeIndex(usize);

impl AstNodeIndex {
    fn new(idx: usize) -> Self {
        Self(idx)
    }

    fn get(&self) -> usize {
        self.0
    }
}

#[derive(Debug)]
struct AstNodes(Vec<AstNode>);

impl AstNodes {
    fn new(n: usize) -> Self {
        Self(Vec::with_capacity(n))
    }

    fn push(&mut self, node: AstNode) -> AstNodeIndex {
        self.0.push(node);
        AstNodeIndex::new(self.0.len() - 1)
    }

    fn get(&self, idx: AstNodeIndex) -> &AstNode {
        self.0.get(idx.get()).unwrap()
    }

    fn get_str(&self, ast: &Ast, idx: AstNodeIndex) -> String {
        self.get(idx).get_str(ast)
    }

    fn len(&self) -> usize {
        self.0.len()
    }
}

pub trait AstNodesVisitor {
    type Output;

    fn visit(&self, ast: &Ast, idx: AstNodeIndex) -> Self::Output;
}

#[derive(Debug)]
pub struct PrintAstNodesVisitor;

impl AstNodesVisitor for PrintAstNodesVisitor {
    type Output = String;

    fn visit(&self, ast: &Ast, idx: AstNodeIndex) -> Self::Output {
        ast.get_str(idx)
    }
}

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
    MismatchedParentheses { another_paren: lexer::TokenIndex },
    UnexpectedToken(lexer::TokenIndex),
    LexError(Vec<lexer::TokenIndex>),
}

#[derive(Debug, Default)]
struct PackedTokens(Vec<(lexer::TokenIndex, lexer::Token)>);

impl Extend<(lexer::TokenIndex, lexer::Token)> for PackedTokens {
    fn extend<T: IntoIterator<Item = (lexer::TokenIndex, lexer::Token)>>(&mut self, iter: T) {
        let iter = iter.into_iter();
        let (lower_bound, _) = iter.size_hint();
        self.0.reserve(lower_bound);
        for (index, token) in iter {
            self.0.push((index, token));
        }
    }
}

impl PackedTokens {
    fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
    fn get_all_indices(&self) -> Vec<lexer::TokenIndex> {
        self.0.iter().map(|(token_idx, _)| *token_idx).collect()
    }
    fn get(&self, idx: PackedTokenIndex) -> Option<&(lexer::TokenIndex, lexer::Token)> {
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
                    TokenTrait {
                        precedence: Precedence::new(1),
                        associativity: Associativity::Left,
                    },
                ),
                (
                    lexer::TokenKind::Dash,
                    TokenTrait {
                        precedence: Precedence::new(1),
                        associativity: Associativity::Left,
                    },
                ),
                (
                    lexer::TokenKind::Star,
                    TokenTrait {
                        precedence: Precedence::new(2),
                        associativity: Associativity::Left,
                    },
                ),
                (
                    lexer::TokenKind::Slash,
                    TokenTrait {
                        precedence: Precedence::new(2),
                        associativity: Associativity::Left,
                    },
                ),
                (
                    lexer::TokenKind::Caret,
                    TokenTrait {
                        precedence: Precedence::new(3),
                        associativity: Associativity::Right,
                    },
                ),
            ]
            .into_iter()
            .collect(),
        }
    }

    fn get_precedence(&self, token: &lexer::Token) -> Option<(Precedence, Associativity)> {
        self.token_traits
            .get(&token.get_kind())
            .map(|token_trait| (token_trait.precedence, token_trait.associativity))
    }

    fn parse_expression(
        &self,
        ast: &mut Ast,
        packed_tokens: &PackedTokens,
        cur_packed_token_idx: PackedTokenIndex,
        min_precedence: Precedence,
    ) -> Result<(Option<AstNode>, PackedTokenIndex), ParseError> {
        let (mut lhs, mut cur_packed_token_idx) =
            self.parse_primary(ast, packed_tokens, cur_packed_token_idx)?;
        if lhs.is_none() || matches!(lhs, Some(AstNode::EndOfLine { .. })) {
            return Ok((None, cur_packed_token_idx));
        }

        while let Some(op) = packed_tokens.get(cur_packed_token_idx) {
            if op.1.get_kind() == lexer::TokenKind::SemiColon {
                return Ok((lhs, cur_packed_token_idx));
            }

            let (precedence, associativity) = self
                .get_precedence(&op.1)
                .ok_or(ParseError::UnexpectedToken(op.0))?;
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
            let lhs_node_idx = ast.nodes.push(lhs.unwrap());
            let rhs_node_idx = ast.nodes.push(rhs.unwrap());
            lhs = Some(AstNode::BinaryOp {
                op: op.0,
                lhs: lhs_node_idx,
                rhs: rhs_node_idx,
            });
        }

        Ok((lhs, cur_packed_token_idx))
    }

    fn parse_primary(
        &self,
        ast: &mut Ast,
        packed_tokens: &PackedTokens,
        mut cur_packed_token_idx: PackedTokenIndex,
    ) -> Result<(Option<AstNode>, PackedTokenIndex), ParseError> {
        let Some(token) = packed_tokens.get(cur_packed_token_idx) else {
            return Ok((None, cur_packed_token_idx));
        };

        match token.1.get_kind() {
            lexer::TokenKind::Int64 => {
                Ok((Some(AstNode::Number(token.0)), cur_packed_token_idx + 1))
            }
            lexer::TokenKind::Dash => {
                if let (Some(rhs), new_cur_packed_token_idx) =
                    self.parse_primary(ast, packed_tokens, cur_packed_token_idx + 1)?
                {
                    let rhs_node_idx = ast.nodes.push(rhs);
                    Ok((
                        Some(AstNode::UnaryOp {
                            op: token.0,
                            rhs: rhs_node_idx,
                        }),
                        new_cur_packed_token_idx,
                    ))
                } else {
                    Err(ParseError::UnexpectedEndOfInput)
                }
            }
            lexer::TokenKind::Plus => packed_tokens.get(cur_packed_token_idx + 1).map_or(
                Err(ParseError::UnexpectedEndOfInput),
                |(token_idx, token)| match token.get_kind() {
                    lexer::TokenKind::Int64 => {
                        Ok((Some(AstNode::Number(*token_idx)), cur_packed_token_idx + 2))
                    }
                    _ => Err(ParseError::UnexpectedToken(*token_idx)),
                },
            ),
            lexer::TokenKind::LParen => {
                let (expr, new_cur_packed_token_idx) = self.parse_expression(
                    ast,
                    packed_tokens,
                    cur_packed_token_idx,
                    Precedence::new(0),
                )?;
                cur_packed_token_idx = new_cur_packed_token_idx;
                return match packed_tokens.get(cur_packed_token_idx) {
                    Some(token) if token.1.get_kind() == lexer::TokenKind::RParen => {
                        Ok((expr, cur_packed_token_idx + 1))
                    }
                    _ => Err(ParseError::MismatchedParentheses {
                        another_paren: token.0,
                    }),
                };
            }
            lexer::TokenKind::SemiColon => Ok((None, cur_packed_token_idx + 1)),
            _ => Err(ParseError::UnexpectedToken(token.0)),
        }
    }

    pub fn parse<'cu>(&self, cu: &'cu lexer::CompilationUnit) -> Result<Ast<'cu>, ParseError> {
        fn partition_packed_tokens_and_lex_errors(
            tokens: lexer::Tokens,
        ) -> (PackedTokens, PackedTokens) {
            tokens
                .into_iter()
                .enumerate()
                .filter(|(_, token)| {
                    token.get_kind() != lexer::TokenKind::Spaces
                        && token.get_kind() != lexer::TokenKind::NewLine
                })
                .map(|(token_idx, token)| (lexer::TokenIndex::new(token_idx), token))
                .partition(|(_, token)| token.get_kind() != lexer::TokenKind::Invalid)
        }

        let tokens = cu.get_tokens();
        let mut ast = Ast {
            cu,
            root: None,
            nodes: AstNodes::new(tokens.len()),
            tokens,
        };

        let (tokens, lex_errors) = partition_packed_tokens_and_lex_errors(ast.tokens.clone());
        if !lex_errors.is_empty() {
            return Err(ParseError::LexError(lex_errors.get_all_indices()));
        }

        let (expr, _cur_packed_token_idx) = self
            .parse_expression(
                &mut ast,
                &tokens,
                PackedTokenIndex::new(0),
                Precedence::new(0),
            )
            .unwrap();
        if let Some(expr) = expr {
            ast.root = Some(ast.nodes.push(expr));
        }
        // TODO: only read one expression for now

        //if cur_packed_token_idx < tokens.len() {
        //    return Err(ParseError::UnexpectedToken(tokens[cur_packed_token_idx].0));
        //}

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
            let ast = parser.parse(&cu);
            assert!(ast.is_ok());
            assert!(
                matches!(ast, Ok(Ast { root: None, .. })),
                "input: `{}`, ast: {:?}",
                s,
                ast
            );
        }
    }

    #[test]
    fn test_simple_math() {
        for (expected, test_data) in [
            ("0", vec!["0;", " 0;", "0 ;", " 0 ;"]),
            ("1", vec!["1;", " 1;", "1 ;", " 1 ;"]),
            (
                "(0 + 0)",
                vec![
                    "0+0;", " 0+0;", "0 +0;", "0+ 0;", " 0 + 0;", "0 + 0 ;", " 0 + 0 ;",
                ],
            ),
            (
                "(1 + 1)",
                vec![
                    "1+1;", " 1+1;", "1 +1;", "1+ 1;", " 1 + 1;", "1 + 1 ;", " 1 + 1 ;",
                ],
            ),
            (
                "(0 - 0)",
                vec![
                    "0-0;", " 0-0;", "0 -0;", "0- 0;", " 0 - 0;", "0 - 0 ;", " 0 - 0 ;",
                ],
            ),
            (
                "(1 - 1)",
                vec![
                    "1-1;", " 1-1;", "1 -1;", "1- 1;", " 1 - 1;", "1 - 1 ;", " 1 - 1 ;",
                ],
            ),
            (
                "(0 * 0)",
                vec![
                    "0*0;", " 0*0;", "0 *0;", "0* 0;", " 0 * 0;", "0 * 0 ;", " 0 * 0 ;",
                ],
            ),
            (
                "(1 * 1)",
                vec![
                    "1*1;", " 1*1;", "1 *1;", "1* 1;", " 1 * 1;", "1 * 1 ;", " 1 * 1 ;",
                ],
            ),
            (
                "(0 / 0)",
                vec![
                    "0/0;", " 0/0;", "0 /0;", "0/ 0;", " 0 / 0;", "0 / 0 ;", " 0 / 0 ;",
                ],
            ),
            (
                "(1 / 1)",
                vec![
                    "1/1;", " 1/1;", "1 /1;", "1/ 1;", " 1 / 1;", "1 / 1 ;", " 1 / 1 ;",
                ],
            ),
            (
                "(0 ^ 0)",
                vec![
                    "0^0;", " 0^0;", "0 ^0;", "0^ 0;", " 0 ^ 0;", "0 ^ 0 ;", " 0 ^ 0 ;",
                ],
            ),
            (
                "(1 ^ 1)",
                vec![
                    "1^1;", " 1^1;", "1 ^1;", "1^ 1;", " 1 ^ 1;", "1 ^ 1 ;", " 1 ^ 1 ;",
                ],
            ),
        ] {
            for s in test_data {
                let cu = lexer::CompilationUnit::from_string("stdin", s);
                let parser = Parser::new();
                let ast = parser.parse(&cu);
                assert!(
                    matches!(ast, Ok(Ast { root: Some(_), .. })),
                    "failed on `{}`",
                    s
                );
                let ast = ast.unwrap();
                assert!(
                    matches!(ast, Ast { root: Some(ast_node_idx), .. } if ast.get_str(ast_node_idx) == expected),
                    "ast: {:?}, expected: `{}`, got: `{}`",
                    ast,
                    expected,
                    ast.get_str(ast.root.unwrap())
                );

                let printer = PrintAstNodesVisitor;
                assert_eq!(printer.visit(&ast, ast.root.unwrap()), expected);
            }
        }
    }

    #[test]
    fn test_negative_numbers() {
        for (expected, node_cnt, test_data) in [
            ("(- 42)", 2, vec!["-42;", "- 42;", " - 42 ;", " -42 ;"]),
            (
                "(- (- 42))",
                3,
                vec!["--42;", "- -42;", " -- 42 ;", "- -42 ;", " - - 42 ;"],
            ),
            (
                "(3 - (- (- 2)))",
                5,
                vec![
                    "3---2;",
                    "3-- -2;",
                    " 3 - - - 2 ;",
                    "3 - --2 ;",
                    " 3 ---2 ;",
                ],
            ),
            (
                "((- 3) - (- 2))",
                5,
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
                let ast = parser.parse(&cu);
                assert!(
                    matches!(ast, Ok(Ast { root: Some(_), .. })),
                    "failed on `{}`",
                    s
                );
                let ast = ast.unwrap();

                let printer = PrintAstNodesVisitor;
                assert_eq!(
                    ast.nodes_len(),
                    node_cnt,
                    "{}",
                    printer.visit(&ast, ast.root.unwrap())
                );
                assert_eq!(printer.visit(&ast, ast.root.unwrap()), expected);
            }
        }
    }

    #[test]
    fn test_number_with_preceding_positive_sign_should_return_error() {}

    #[test]
    fn test_parser() {
        //let cu = lexer::CompilationUnit::from_string(
        //    "stdin",
        //    " 7^2 + 3 * (12 / (+15 / -( 3+1) - - 1) ) - 2^3^4",
        //);
        //let parser = Parser::new();
        //let _ast = parser.parse(&cu);
    }
}
