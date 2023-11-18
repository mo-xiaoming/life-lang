use smol_str::SmolStr;
use unicode_segmentation::UnicodeSegmentation;

pub struct UCChar {
    start_byte_offset: usize,
    raw_str: SmolStr,
}

pub enum CompilationUnitKind {
    FromFile { path: std::path::PathBuf },
    FromString { mark: String },
}

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

    fn get_origin(&self) -> String {
        match &self.kind {
            CompilationUnitKind::FromFile { path } => format!("{}", path.display()),
            CompilationUnitKind::FromString { mark } => mark.to_owned(),
        }
    }

    fn str_to_ucs(s: &str) -> Vec<UCChar> {
        s.grapheme_indices(true)
            .map(|(idx, s)| UCChar {
                start_byte_offset: idx,
                raw_str: SmolStr::from(s),
            })
            .collect()
    }

    fn iter(&self) -> <&Self as IntoIterator>::IntoIter {
        self.into_iter()
    }
}

pub struct CompilationUnitIntoIter<'cu> {
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
            iter: self.uc_content.iter(),
        }
    }
}

enum TokenKind {
    Int64,

    Plus,
    Dash,
    Asterisk,
    Slash,
    Caret,

    LParen,
    RParen,
}

impl std::fmt::Display for TokenKind {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            TokenKind::Int64 => write!(f, "Int64"),
            TokenKind::Plus => write!(f, "Plus"),
            TokenKind::Dash => write!(f, "Dash"),
            TokenKind::Asterisk => write!(f, "Asterisk"),
            TokenKind::Slash => write!(f, "Slash"),
            TokenKind::Caret => write!(f, "Caret"),
            TokenKind::LParen => write!(f, "LParen"),
            TokenKind::RParen => write!(f, "RParen"),
        }
    }
}

struct CompilationUnitRawIndex(usize);

struct TokenSpan {
    start: CompilationUnitRawIndex,
    end: CompilationUnitRawIndex,
}

struct Token {
    kind: TokenKind,
    span: TokenSpan,
}

impl Token {
    fn from(kind: TokenKind, uc: &UCChar) -> Self {
        Self {
            kind,
            span: TokenSpan {
                start: CompilationUnitRawIndex(uc.start_byte_offset),
                end: CompilationUnitRawIndex(uc.start_byte_offset + uc.raw_str.len()),
            },
        }
    }

    fn from_range<'cu>(
        kind: TokenKind,
        uc_start: &'cu UCChar,
        uc_inclusive_end: &'cu UCChar,
    ) -> Self {
        Self {
            kind,
            span: TokenSpan {
                start: CompilationUnitRawIndex(uc_start.start_byte_offset),
                end: CompilationUnitRawIndex(
                    uc_inclusive_end.start_byte_offset + uc_inclusive_end.raw_str.len(),
                ),
            },
        }
    }

    fn to_string(&self, uc: &CompilationUnit) -> String {
        uc.raw_content[self.span.start.0..self.span.end.0].to_string()
    }

    fn serialize(&self, uc: &CompilationUnit) -> String {
        format!("{} {}", self.kind, self.to_string(uc))
    }
}

struct LexError<'cu>(&'cu UCChar);

impl<'cu> LexError<'cu> {
    fn to_string(&'cu self, uc: &'cu CompilationUnit) -> String {
        format!(
            "{}: unexpected charactor `{}` at {}",
            uc.get_origin(),
            self.0.raw_str,
            self.0.start_byte_offset
        )
    }
}

struct Lexer<'cu> {
    iter: std::iter::Peekable<<&'cu CompilationUnit as IntoIterator>::IntoIter>,
}

impl<'cu> Lexer<'cu> {
    fn new(cu: &'cu CompilationUnit) -> Self {
        Self {
            iter: cu.iter().peekable(),
        }
    }

    fn uc_is_ascii_digit(uc: &'cu UCChar) -> bool {
        let s = uc.raw_str.as_str();
        if s.len() != 1 {
            return false;
        }
        let c = s.chars().next().unwrap();
        c.is_ascii_digit()
    }

    fn next_token(&mut self) -> Option<Result<Token, LexError>> {
        loop {
            match self.iter.next() {
                Some(uc) if uc.raw_str.len() == 1 => {
                    match uc.raw_str.as_str().chars().next().unwrap() {
                        '+' => return Some(Ok(Token::from(TokenKind::Plus, uc))),
                        '-' => return Some(Ok(Token::from(TokenKind::Dash, uc))),
                        '*' => return Some(Ok(Token::from(TokenKind::Asterisk, uc))),
                        '/' => return Some(Ok(Token::from(TokenKind::Slash, uc))),
                        '^' => return Some(Ok(Token::from(TokenKind::Caret, uc))),
                        '(' => return Some(Ok(Token::from(TokenKind::LParen, uc))),
                        ')' => return Some(Ok(Token::from(TokenKind::RParen, uc))),
                        '1'..='9' => match std::iter::from_fn(|| {
                            self.iter
                                .by_ref()
                                .next_if(|&uc| Self::uc_is_ascii_digit(uc))
                        })
                        .last()
                        {
                            Some(uc_end) => {
                                return Some(Ok(Token::from_range(TokenKind::Int64, uc, uc_end)))
                            }
                            None => return Some(Ok(Token::from(TokenKind::Int64, uc))),
                        },
                        c if c.is_whitespace() => continue,
                        _ => return Some(Err(LexError(uc))),
                    }
                }
                Some(uc) => return Some(Err(LexError(uc))),
                None => return None,
            }
        }
    }
}

pub struct Ast {}

pub struct Parser {}

impl Parser {
    pub fn parse(cu: &CompilationUnit) -> Ast {
        let mut lexer = Lexer::new(cu);
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
        let cu = CompilationUnit::from_string(
            "stdin",
            " 7^2 + 3 * (12 / (+15 / ( 3+1) - - 1) ) - 2^3^4",
        )
        .unwrap();
        let _ast = Parser::parse(&cu);
    }
}
