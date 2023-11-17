#![allow(dead_code)]

use smol_str::SmolStr;
use unicode_segmentation::UnicodeSegmentation;

#[derive(Debug)]
pub struct UCChar {
    start_idx_from_raw_content: usize,
    uc: SmolStr,
}

#[derive(Debug)]
pub enum CompilationUnitKind {
    FromFile { path: std::path::PathBuf },
    FromString { mark: String },
}

#[derive(Debug)]
pub struct CompilationUnit {
    kind: CompilationUnitKind,
    raw_content: String,
    uc_content: Vec<UCChar>,
}

impl CompilationUnit {
    pub fn from_file<P>(filename: P) -> Result<Self, String>
    where
        P: AsRef<std::path::Path>,
    {
        let raw_content = std::fs::read_to_string(&filename).map_err(|e| e.to_string())?;
        Ok(CompilationUnit {
            kind: CompilationUnitKind::FromFile {
                path: filename.as_ref().to_path_buf(),
            },
            raw_content: raw_content.clone(),
            uc_content: Self::str_to_ucs(&raw_content),
        })
    }

    pub fn from_string(mark: &str, input: &str) -> Result<Self, String> {
        Ok(CompilationUnit {
            kind: CompilationUnitKind::FromString {
                mark: mark.to_owned(),
            },
            raw_content: input.to_owned(),
            uc_content: Self::str_to_ucs(input),
        })
    }

    fn str_to_ucs(s: &str) -> Vec<UCChar> {
        s.grapheme_indices(true)
            .map(|(idx, s)| UCChar {
                start_idx_from_raw_content: idx,
                uc: SmolStr::from(s),
            })
            .collect()
    }

    fn iter(&self) -> <&Self as IntoIterator>::IntoIter {
        self.into_iter()
    }
}

#[derive(Debug)]
pub struct CompilationUnitIntoIter<'cu> {
    cu: &'cu CompilationUnit,
    iter: std::slice::Iter<'cu, UCChar>,
}

impl<'cu> Iterator for CompilationUnitIntoIter<'cu> {
    type Item = &'cu UCChar;

    fn next(&mut self) -> Option<Self::Item> {
        self.iter.next()
    }
}

impl<'cu> IntoIterator for &'cu CompilationUnit {
    type Item = <CompilationUnitIntoIter<'cu> as Iterator>::Item;

    type IntoIter = CompilationUnitIntoIter<'cu>;

    fn into_iter(self) -> Self::IntoIter {
        Self::IntoIter {
            cu: self,
            iter: self.uc_content.iter(),
        }
    }
}

#[derive(Debug, PartialEq, Eq)]
enum TokenKind {
    Eof,

    Int64,

    Plus,
    Minus,
    Asterisk,
    Slash,

    LParen,
    RParen,
}

#[derive(Debug)]
struct CompilationUnitIndex(usize);

#[derive(Debug)]
struct TokenSpan {
    start: CompilationUnitIndex,
    end: CompilationUnitIndex,
}

#[derive(Debug)]
struct Token {
    kind: TokenKind,
    span: TokenSpan,
}

#[derive(Debug)]
struct Lexer<'cu> {
    cu: &'cu CompilationUnit,
    iter: <&'cu CompilationUnit as IntoIterator>::IntoIter,
}

impl<'cu> Lexer<'cu> {
    fn new(cu: &'cu CompilationUnit) -> Self {
        Self {
            cu,
            iter: cu.iter(),
        }
    }

    fn next_token(&mut self) -> Result<Token, TokenSpan> {
        unimplemented!()
    }
}

#[derive(Debug)]
pub struct Ast {}

#[derive(Debug)]
pub struct Parser {}

impl Parser {
    pub fn parse(cu: &CompilationUnit) -> Ast {
        let mut lexer = Lexer::new(cu);
        loop {
            let token = lexer.next_token().unwrap();
            println!("{:?}", token);
            if token.kind == TokenKind::Eof {
                break;
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
        let _cu =
            CompilationUnit::from_string("stdin", "7^2 + 3 * (12 / (+15 / ( 3+1) - - 1) ) - 2^3^4")
                .unwrap();
        //let _ast = Parser::parse(&cu);
    }
}
