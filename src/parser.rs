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
struct AstNodeIndex(usize);

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

trait AstNodesVisitor<R> {
    fn visit(&self, ast: &Ast, idx: AstNodeIndex) -> R;
}

#[derive(Debug)]
struct PrintAstNodesVisitor;

impl AstNodesVisitor<String> for PrintAstNodesVisitor {
    fn visit(&self, ast: &Ast, idx: AstNodeIndex) -> String {
        ast.get_str(idx)
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
enum Associativity {
    Left,
    Right,
}

#[derive(Debug)]
struct TokenTrait {
    precedence: u8,
    associativity: Associativity,
}

mod precedence_climbing {
    use super::*;
    use crate::lexer;

    #[derive(Debug, Clone)]
    pub enum ParseError {
        UnexpectedEndOfInput,
        IntegerOverflow { token: lexer::TokenIndex },
        MismatchedParentheses { another_paren: lexer::TokenIndex },
        UnexpectedToken(lexer::TokenIndex),
        LexError(Vec<lexer::TokenIndex>),
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
                            precedence: 1,
                            associativity: Associativity::Left,
                        },
                    ),
                    (
                        lexer::TokenKind::Dash,
                        TokenTrait {
                            precedence: 1,
                            associativity: Associativity::Left,
                        },
                    ),
                    (
                        lexer::TokenKind::Star,
                        TokenTrait {
                            precedence: 2,
                            associativity: Associativity::Left,
                        },
                    ),
                    (
                        lexer::TokenKind::Slash,
                        TokenTrait {
                            precedence: 2,
                            associativity: Associativity::Left,
                        },
                    ),
                    (
                        lexer::TokenKind::Caret,
                        TokenTrait {
                            precedence: 3,
                            associativity: Associativity::Right,
                        },
                    ),
                ]
                .into_iter()
                .collect(),
            }
        }

        fn get_precedence(&self, token: &lexer::Token) -> Option<(u8, Associativity)> {
            self.token_traits
                .get(&token.get_kind())
                .map(|token_trait| (token_trait.precedence, token_trait.associativity))
        }

        fn parse_expression(
            &self,
            ast: &mut Ast,
            tokens: &[(lexer::TokenIndex, lexer::Token)],
            cur_packed_token_idx: usize,
            min_precedence: u8,
        ) -> Result<(Option<AstNode>, usize), ParseError> {
            let (mut lhs, mut cur_packed_token_idx) =
                self.parse_primary(ast, tokens, cur_packed_token_idx)?;
            if lhs.is_none() || matches!(lhs, Some(AstNode::EndOfLine { .. })) {
                return Ok((None, cur_packed_token_idx));
            }

            while cur_packed_token_idx < tokens.len() {
                let Some(op) = tokens.get(cur_packed_token_idx) else {
                    break;
                };
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

                let (rhs, new_cur_packed_token_idx) =
                    self.parse_expression(ast, tokens, cur_packed_token_idx + 1, min_precedence)?;
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
            tokens: &[(lexer::TokenIndex, lexer::Token)],
            mut cur_packed_token_idx: usize,
        ) -> Result<(Option<AstNode>, usize), ParseError> {
            let Some(token) = tokens.get(cur_packed_token_idx) else {
                return Ok((None, cur_packed_token_idx));
            };

            match token.1.get_kind() {
                lexer::TokenKind::Int64 => {
                    Ok((Some(AstNode::Number(token.0)), cur_packed_token_idx + 1))
                }
                lexer::TokenKind::Dash => {
                    if let (Some(rhs), new_cur_packed_token_idx) =
                        self.parse_primary(ast, tokens, cur_packed_token_idx + 1)?
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
                lexer::TokenKind::Plus => tokens.get(cur_packed_token_idx + 1).map_or(
                    Err(ParseError::UnexpectedEndOfInput),
                    |(token_idx, token)| match token.get_kind() {
                        lexer::TokenKind::Int64 => {
                            Ok((Some(AstNode::Number(*token_idx)), cur_packed_token_idx + 2))
                        }
                        _ => Err(ParseError::UnexpectedToken(*token_idx)),
                    },
                ),
                lexer::TokenKind::LParen => {
                    let (expr, new_cur_packed_token_idx) =
                        self.parse_expression(ast, tokens, cur_packed_token_idx, 0)?;
                    cur_packed_token_idx = new_cur_packed_token_idx;
                    return match tokens.get(cur_packed_token_idx) {
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
            let tokens = cu.get_tokens();
            let mut ast = Ast {
                cu,
                root: None,
                nodes: AstNodes::new(tokens.len()),
                tokens,
            };

            let (tokens, lex_errors): (Vec<_>, Vec<_>) = ast
                .tokens
                .clone()
                .into_iter()
                .enumerate()
                .filter(|(_, token)| {
                    token.get_kind() != lexer::TokenKind::Spaces
                        && token.get_kind() != lexer::TokenKind::NewLine
                })
                .map(|(token_idx, token)| (lexer::TokenIndex::new(token_idx), token))
                .partition(|(_, token)| token.get_kind() != lexer::TokenKind::Invalid);
            if !lex_errors.is_empty() {
                return Err(ParseError::LexError(
                    lex_errors
                        .into_iter()
                        .map(|(token_idx, _)| token_idx)
                        .collect(),
                ));
            }

            let (expr, _cur_packed_token_idx) =
                self.parse_expression(&mut ast, &tokens, 0, 0).unwrap();
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
}
