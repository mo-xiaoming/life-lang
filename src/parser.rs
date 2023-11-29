#![allow(dead_code)]

use crate::lexer;

#[derive(Debug)]
pub struct Ast {}

#[derive(Debug)]
enum AstNode {
    Number(i64),
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

    #[derive(Debug)]
    pub enum ParseErrorKind<'cu> {
        UnexpectedEndOfInput,
        LexError(lexer::LexError<'cu>),
        IntegerOverflow { token: lexer::Token },
        MismatchedParentheses { another_paren: lexer::Token },
        UnexpectedToken(lexer::Token),
    }

    #[derive(Debug)]
    pub struct ParseError<'cu> {
        kind: ParseErrorKind<'cu>,
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
            if let Some(token_trait) = self.token_traits.get(&token.kind) {
                Some((token_trait.precedence, token_trait.associativity))
            } else {
                None
            }
        }

        fn parse_expression<'cu>(
            &self,
            cu: &'cu lexer::CompilationUnit,
            iter: &'cu mut lexer::CompilationUnitTokenIter,
            min_precedence: u8,
        ) -> Result<AstNode, ParseError<'cu>> {
            let mut lhs = self.parse_primary(cu, iter)?;

            while let Some(op) = iter.next() {
                let token = op.map_err(|e| ParseError {
                    kind: ParseErrorKind::LexError(e),
                })?;
                let (precedence, associativity) =
                    self.get_precedence(&token).ok_or(ParseError {
                        kind: ParseErrorKind::UnexpectedToken(token.clone()),
                    })?;
                if precedence < min_precedence {
                    break;
                }
                let min_precedence = if associativity == Associativity::Left {
                    precedence + 1
                } else {
                    precedence
                };
                let rhs = self.parse_expression(cu, iter, min_precedence)?;
                lhs = AstNode::BinaryOp {
                    op: token,
                    lhs: Box::new(lhs),
                    rhs: Box::new(rhs),
                };
            }

            Ok(lhs)
        }

        fn parse_primary<'cu>(
            &self,
            cu: &'cu lexer::CompilationUnit,
            iter: &'cu mut lexer::CompilationUnitTokenIter,
        ) -> Result<AstNode, ParseError<'cu>> {
            let token = iter
                .next()
                .ok_or(ParseError {
                    kind: ParseErrorKind::UnexpectedEndOfInput,
                })?
                .map_err(|e| ParseError {
                    kind: ParseErrorKind::LexError(e),
                })?;

            match token.kind {
                lexer::TokenKind::Int64 => token
                    .to_str(cu)
                    .parse::<i64>()
                    .map(AstNode::Number)
                    .map_err(|_| ParseError {
                        kind: ParseErrorKind::IntegerOverflow { token },
                    }),
                lexer::TokenKind::Dash => {
                    let rhs = self.parse_primary(cu, iter)?;
                    Ok(AstNode::UnaryOp {
                        op: token,
                        rhs: Box::new(rhs),
                    })
                }
                lexer::TokenKind::LParen => {
                    let expr = self.parse_expression(cu, iter, 0)?;
                    match iter.next() {
                        Some(Ok(token)) if token.kind == lexer::TokenKind::RParen => Ok(expr),
                        _ => Err(ParseError {
                            kind: ParseErrorKind::MismatchedParentheses {
                                another_paren: token,
                            },
                        }),
                    }
                }
                _ => Err(ParseError {
                    kind: ParseErrorKind::UnexpectedToken(token),
                }),
            }
        }

        pub fn parse(&self, cu: &lexer::CompilationUnit) -> Ast {
            let mut iter = cu.token_iter();
            self.parse_expression(cu, &mut iter, 0u8);
            Ast {}
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
            )
            .unwrap();
            let parser = Parser::new();
            let _ast = parser.parse(&cu);
        }
    }
}

pub struct Parser {}

impl Parser {
    pub fn parse(cu: &lexer::CompilationUnit) -> Ast {
        for token in cu.token_iter() {
            match token {
                Ok(token) => println!("{}", token.serialize(cu)),
                Err(err) => println!("{}", err.to_string(cu)),
            }
        }
        Ast {}
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lexer() {
        let cu = lexer::CompilationUnit::from_string(
            "stdin",
            " 7^2 + 3 * (12 / (+15 / ( 3+1) - - 1) ) - 2^3^4",
        )
        .unwrap();
        let _ast = Parser::parse(&cu);
    }
}
