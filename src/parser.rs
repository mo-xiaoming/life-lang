#![allow(dead_code)]

use super::lexer;

#[derive(Debug)]
pub struct Ast {}

#[derive(Debug)]
enum AstNode {
    Number(i64),
    BinaryOp {
        op: lexer::TokenKind,
        lhs: Box<AstNode>,
        rhs: Box<AstNode>,
    },
    UnaryOp {
        op: lexer::TokenKind,
        rhs: Box<AstNode>,
    },
}

#[derive(Debug)]
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

        //        fn parse_expr(&mut self) -> AstNode {
        //            let mut lhs = self.parse_term();
        //
        //            while let Some(token) = self.peek_token() {
        //                match token.kind {
        //                    TokenKind::Plus | TokenKind::Minus => {
        //                        self.consume_operator();
        //                        let rhs = self.parse_term();
        //                        lhs = AstNode::BinaryOp {
        //                            operator: token,
        //                            lhs: Box::new(lhs),
        //                            rhs: Box::new(rhs),
        //                        };
        //                    }
        //                    _ => break,
        //                }
        //            }
        //
        //            lhs
        //        }
        //
        //        fn parse_term(&mut self) -> AstNode {
        //            let mut lhs = self.parse_factor();
        //
        //            while let Some(token) = self.peek_token() {
        //                match token.kind {
        //                    TokenKind::Star | TokenKind::Slash => {
        //                        self.consume_operator();
        //                        let rhs = self.parse_factor();
        //                        lhs = AstNode::BinaryOp {
        //                            operator: token,
        //                            lhs: Box::new(lhs),
        //                            rhs: Box::new(rhs),
        //                        };
        //                    }
        //                    _ => break,
        //                }
        //            }
        //
        //            lhs
        //        }
        //
        //        // Continue with parse_factor, parse_unary, etc.

        pub fn parse(&self, cu: &lexer::CompilationUnit) -> Ast {
            for _token in cu.token_iter() {}
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
