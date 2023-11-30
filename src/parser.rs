#![allow(dead_code)]
use crate::lexer;

#[derive(Debug)]
pub struct Ast {
    root: Option<AstNode>,
}

#[derive(Debug)]
enum AstNode {
    Number(lexer::Token),
    BinaryOp {
        op: lexer::Token,
        lhs: Box<AstNode>,
        rhs: Box<AstNode>,
    },
    UnaryOp {
        op: lexer::Token,
        rhs: Box<AstNode>,
    },
}

#[derive(Debug, PartialEq, Clone, Copy)]
enum Associativity {
    Left,
    Right,
}

#[derive(Debug)]
struct TokenTrait {
    kind: lexer::TokenKind,
    precedence: u8,
    associativity: Associativity,
}

mod precedence_climbing {
    use super::*;
    use crate::lexer;

    #[derive(Debug, Clone)]
    pub enum ParseError {
        UnexpectedEndOfInput,
        IntegerOverflow { token: lexer::Token },
        MismatchedParentheses { another_paren: lexer::Token },
        UnexpectedToken(lexer::Token),
        LexError(Vec<lexer::Token>),
    }

    #[derive(Debug)]
    pub struct Parser {
        token_traits: std::collections::HashMap<lexer::TokenKind, TokenTrait>,
    }

    impl Parser {
        pub fn new() -> Self {
            let mut token_traits = std::collections::HashMap::with_capacity(10);
            let traits = vec![
                (lexer::TokenKind::Plus, 1, Associativity::Left),
                (lexer::TokenKind::Dash, 1, Associativity::Left),
                (lexer::TokenKind::Star, 2, Associativity::Left),
                (lexer::TokenKind::Slash, 2, Associativity::Left),
                (lexer::TokenKind::Caret, 3, Associativity::Right),
            ];
            for (kind, precedence, associativity) in traits {
                token_traits.insert(
                    kind,
                    TokenTrait {
                        kind,
                        precedence,
                        associativity,
                    },
                );
            }

            Self { token_traits }
        }

        fn get_precedence(&self, token: &lexer::Token) -> Option<(u8, Associativity)> {
            self.token_traits
                .get(&token.kind)
                .map(|token_trait| (token_trait.precedence, token_trait.associativity))
        }

        fn skip_blanks(&self, tokens: &[lexer::Token], mut cur_token_idx: usize) -> usize {
            while cur_token_idx < tokens.len() {
                let token = tokens.get(cur_token_idx).unwrap();
                if token.kind != lexer::TokenKind::Spaces || token.kind != lexer::TokenKind::NewLine
                {
                    break;
                }
                cur_token_idx += 1;
            }
            cur_token_idx
        }

        fn parse_expression(
            &self,
            tokens: &[lexer::Token],
            cur_token_idx: usize,
            min_precedence: u8,
        ) -> Result<(AstNode, usize), ParseError> {
            let (mut lhs, mut cur_token_idx) = self.parse_primary(tokens, cur_token_idx)?;

            while cur_token_idx < tokens.len() {
                cur_token_idx = self.skip_blanks(tokens, cur_token_idx);
                let op = tokens.get(cur_token_idx);
                if op.is_none() {
                    break;
                }
                let op = op.unwrap();

                let (precedence, associativity) = self
                    .get_precedence(op)
                    .ok_or(ParseError::UnexpectedToken(op.clone()))?;
                if precedence < min_precedence {
                    break;
                }
                let min_precedence = if associativity == Associativity::Left {
                    precedence + 1
                } else {
                    precedence
                };

                cur_token_idx = self.skip_blanks(tokens, cur_token_idx + 1);
                let (rhs, new_cur_token_idx) =
                    self.parse_expression(tokens, cur_token_idx + 1, min_precedence)?;
                cur_token_idx = new_cur_token_idx;
                lhs = AstNode::BinaryOp {
                    op: op.clone(),
                    lhs: Box::new(lhs),
                    rhs: Box::new(rhs),
                };
            }

            Ok((lhs, cur_token_idx))
        }

        fn parse_primary(
            &self,
            tokens: &[lexer::Token],
            mut cur_token_idx: usize,
        ) -> Result<(AstNode, usize), ParseError> {
            let token = tokens
                .get(cur_token_idx)
                .ok_or(ParseError::UnexpectedEndOfInput)?;

            cur_token_idx = self.skip_blanks(tokens, cur_token_idx + 1);

            match token.kind {
                lexer::TokenKind::Int64 => Ok((AstNode::Number(token.clone()), cur_token_idx + 1)),
                lexer::TokenKind::Dash | lexer::TokenKind::Plus => {
                    let (rhs, new_cur_token_idx) = self.parse_primary(tokens, cur_token_idx)?;
                    Ok((
                        AstNode::UnaryOp {
                            op: token.clone(),
                            rhs: Box::new(rhs),
                        },
                        new_cur_token_idx,
                    ))
                }
                lexer::TokenKind::LParen => {
                    let (expr, new_cur_token_idx) =
                        self.parse_expression(tokens, cur_token_idx, 0)?;
                    cur_token_idx = new_cur_token_idx;
                    return match tokens.get(cur_token_idx) {
                        Some(token) if token.kind == lexer::TokenKind::RParen => {
                            Ok((expr, cur_token_idx + 1))
                        }
                        _ => Err(ParseError::MismatchedParentheses {
                            another_paren: token.clone(),
                        }),
                    };
                }
                _ => Err(ParseError::UnexpectedToken(token.clone())),
            }
        }

        pub fn parse(&self, cu: &lexer::CompilationUnit) -> Result<Ast, ParseError> {
            let (tokens, lex_errors): (Vec<_>, Vec<_>) = cu
                .get_tokens()
                .into_iter()
                .partition(|token| token.kind != lexer::TokenKind::Invalid);
            if !lex_errors.is_empty() {
                return Err(ParseError::LexError(lex_errors));
            }

            let cur_token_idx = self.skip_blanks(&tokens, 0);
            if cur_token_idx >= tokens.len() {
                return Ok(Ast { root: None });
            }
            let (expr, new_cur_token_idx) =
                self.parse_expression(&tokens, cur_token_idx, 0).unwrap();
            let cur_token_idx = self.skip_blanks(&tokens, new_cur_token_idx);
            if cur_token_idx < tokens.len() {
                return Err(ParseError::UnexpectedToken(tokens[cur_token_idx].clone()));
            }
            Ok(Ast { root: Some(expr) })
        }
    }

    #[cfg(test)]
    mod test {
        use super::*;

        #[test]
        fn test_parser() {
            let cu = lexer::CompilationUnit::from_string(
                "stdin",
                " 7^2 + 3 * (12 / (+15 / -( 3+1) - - 1) ) - 2^3^4",
            );
            let parser = Parser::new();
            let _ast = parser.parse(&cu);
        }
    }
}
