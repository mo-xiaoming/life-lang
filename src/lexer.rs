use smol_str::SmolStr;
use unicode_segmentation::UnicodeSegmentation;

#[derive(Debug)]
pub struct UCChar {
    start_byte_offset: usize,
    raw_str: SmolStr,
}

#[derive(Debug)]
pub(crate) enum CompilationUnitKind {
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
                mark: String::from(mark),
            },
            raw_content: String::from(input),
            uc_content: Self::str_to_ucs(input),
        })
    }

    fn get_origin(&self) -> String {
        match &self.kind {
            CompilationUnitKind::FromFile { path } => format!("{}", path.display()),
            CompilationUnitKind::FromString { mark } => String::from(mark),
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

#[derive(Debug)]
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

#[derive(Debug, PartialEq)]
pub(crate) enum TokenKind {
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

#[derive(Debug)]
struct CompilationUnitRawIndex(usize);

#[derive(Debug)]
struct TokenSpan {
    start: CompilationUnitRawIndex,
    end: CompilationUnitRawIndex,
}

#[derive(Debug)]
pub(crate) struct Token {
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

    pub(crate) fn serialize(&self, uc: &CompilationUnit) -> String {
        format!("{} {}", self.kind, self.to_string(uc))
    }
}

#[derive(Debug)]
pub(crate) struct LexError<'cu>(&'cu UCChar);

impl<'cu> LexError<'cu> {
    pub(crate) fn to_string(&'cu self, uc: &'cu CompilationUnit) -> String {
        format!(
            "{}: unexpected charactor `{}` at {}",
            uc.get_origin(),
            self.0.raw_str,
            self.0.start_byte_offset
        )
    }
}

#[derive(Debug)]
pub(crate) struct Lexer<'cu> {
    iter: std::iter::Peekable<<&'cu CompilationUnit as IntoIterator>::IntoIter>,
}

impl<'cu> Lexer<'cu> {
    pub(crate) fn new(cu: &'cu CompilationUnit) -> Self {
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

    pub(crate) fn next_token(&mut self) -> Option<Result<Token, LexError>> {
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
                        '1'..='9' => {
                            return Some(Ok(std::iter::from_fn(|| {
                                self.iter
                                    .by_ref()
                                    .next_if(|&uc| Self::uc_is_ascii_digit(uc))
                            })
                            .last()
                            .map_or(Token::from(TokenKind::Int64, uc), |uc_end| {
                                Token::from_range(TokenKind::Int64, uc, uc_end)
                            })))
                        }
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

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    #[derive(Debug)]
    struct TempFile {
        file: std::fs::File,
        path: std::path::PathBuf,
    }

    impl TempFile {
        fn new() -> Self {
            use uuid::Uuid;

            let mut temp_path = std::env::temp_dir();
            temp_path.push(format!("life_lang_test_{}", Uuid::new_v4()));
            Self {
                file: std::fs::File::create(&temp_path)
                    .map_err(|e| {
                        format!("failed to create temp file {}: {}", temp_path.display(), e)
                    })
                    .unwrap(),
                path: temp_path,
            }
        }

        fn write(&mut self, s: &str) {
            write!(self.file, "{}", s)
                .map_err(|e| {
                    format!(
                        "failed to write to temp file {}: {}",
                        self.path.display(),
                        e
                    )
                })
                .unwrap();
        }
    }

    impl Drop for TempFile {
        fn drop(&mut self) {
            std::fs::remove_file(&self.path)
                .map_err(|e| format!("failed to remove temp file {}: {}", self.path.display(), e))
                .unwrap();
        }
    }

    fn with_temp_file(f: impl Fn(&mut TempFile)) {
        let mut temp_file = TempFile::new();
        f(&mut temp_file);
    }

    fn test_from_file_worker(tf: &mut TempFile, content: &str, expected_length: usize) {
        tf.write(content);
        let cu = CompilationUnit::from_file(&tf.path).unwrap();
        assert_eq!(cu.raw_content, content,);
        assert_eq!(cu.uc_content.len(), expected_length, "{:?}", cu.uc_content);
    }

    #[test]
    fn test_from_file_ascii() {
        with_temp_file(|tf| {
            const S: &str = "a de\n";
            test_from_file_worker(tf, S, S.len());
        });
    }

    #[test]
    fn test_from_file_unicode() {
        with_temp_file(|tf| {
            const S: &str = "hi, 你好\n";
            test_from_file_worker(tf, S, 7);
        });
    }

    #[test]
    fn test_from_file_not_exists() {
        assert!(CompilationUnit::from_file("non-exist").is_err());
    }

    fn test_from_string_worker(content: &str, expected_length: usize) {
        const MARK: &str = "mark";
        let cu = CompilationUnit::from_string(MARK, content).unwrap();
        assert_eq!(cu.raw_content, content);
        assert_eq!(cu.uc_content.len(), expected_length, "{:?}", cu.uc_content);
    }

    #[test]
    fn test_from_string_ascii() {
        const S: &str = "a bc\n";
        test_from_string_worker(S, S.len());
    }

    #[test]
    fn test_from_string_unicode() {
        const S: &str = "hi, 你好\n";
        test_from_string_worker(S, 7);
    }

    #[test]
    fn test_get_origin_from_file() {
        with_temp_file(|tf| {
            tf.write("abcefgh");
            let cu = CompilationUnit::from_file(&tf.path).unwrap();
            assert_eq!(cu.get_origin(), tf.path.to_str().unwrap());
        });
    }

    #[test]
    fn test_get_origin_from_string() {
        const MARK: &str = "mark";
        let cu = CompilationUnit::from_string(MARK, "abc").unwrap();
        assert_eq!(cu.get_origin(), MARK);
    }

    fn test_token_from_worker(s: &str) {
        let uc_char = UCChar {
            raw_str: SmolStr::from(s),
            start_byte_offset: 42,
        };
        let token = Token::from(TokenKind::Int64, &uc_char);
        assert_eq!(token.kind, TokenKind::Int64);
        assert_eq!(token.span.start.0, uc_char.start_byte_offset);
        assert_eq!(
            token.span.end.0,
            uc_char.start_byte_offset + uc_char.raw_str.len()
        );
    }

    #[test]
    fn test_token_from_ascii() {
        test_token_from_worker("5")
    }

    #[test]
    fn test_token_from_unicode() {
        test_token_from_worker("您")
    }

    fn test_token_from_range_worker(s: &str) {
        let cu = CompilationUnit::from_string("mark", s).unwrap();
        let ucs: Vec<&UCChar> = cu.iter().collect();
        assert_eq!(ucs.len(), 2);
        let token = Token::from_range(TokenKind::Int64, ucs[0], ucs[1]);
        assert_eq!(token.kind, TokenKind::Int64);
        assert_eq!(token.span.start.0, ucs[0].start_byte_offset);
        assert_eq!(
            token.span.end.0,
            ucs[1].start_byte_offset + ucs[1].raw_str.len()
        );
    }

    #[test]
    fn test_token_from_range_ascii() {
        test_token_from_range_worker("12")
    }

    #[test]
    fn test_token_from_range_unicode() {
        test_token_from_range_worker("您好")
    }

    fn test_token_from_string_worker(s: &str) {
        let cu = CompilationUnit::from_string("mark", s).unwrap();
        let uc_char = UCChar {
            raw_str: SmolStr::from(s),
            start_byte_offset: 0,
        };
        let token = Token::from(TokenKind::Int64, &uc_char);
        assert_eq!(token.to_string(&cu), s);
    }

    #[test]
    fn test_token_to_string() {
        test_token_from_string_worker("5")
    }

    #[test]
    fn test_token_to_string_unicode() {
        test_token_from_string_worker("您")
    }

    fn test_token_serialize_worker(s: &str) {
        let cu = CompilationUnit::from_string("mark", s).unwrap();
        let uc_char = UCChar {
            raw_str: SmolStr::from(s),
            start_byte_offset: 0,
        };
        let token = Token::from(TokenKind::Int64, &uc_char);
        assert_eq!(token.serialize(&cu), format!("Int64 {}", s));
    }

    #[test]
    fn test_token_serialize_ascii() {
        test_token_serialize_worker("5")
    }

    #[test]
    fn test_token_serialize_unicode() {
        test_token_serialize_worker("您")
    }
}
