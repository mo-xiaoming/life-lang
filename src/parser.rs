#![allow(dead_code)]

use super::lexer;

pub struct Ast {}

enum Associativity {
    Left,
    Right,
}

struct TokenTrait {
    kind: lexer::TokenKind,
    precedence: u8,
    associativity: Associativity,
}

mod precedence_climbing {}

pub struct Parser {}

impl Parser {
    pub fn parse(cu: &lexer::CompilationUnit) -> Ast {
        let mut lexer = lexer::Lexer::new(cu);
        while let Some(token) = lexer.next_token() {
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
