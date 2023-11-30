#![allow(dead_code)]

use unicode_segmentation::UnicodeSegmentation;

#[derive(Debug, Clone, Copy)]
struct ByteIndex(usize);

impl ByteIndex {
    fn new(i: usize) -> Self {
        Self(i)
    }
}

#[derive(Debug, Clone, Copy)]
struct ByteIndexSpan {
    start: ByteIndex,
    inclusive_end: ByteIndex,
}

impl ByteIndexSpan {
    fn raw_content_start(&self) -> usize {
        self.start.0
    }
    fn raw_content_inclusive_end(&self) -> usize {
        self.inclusive_end.0
    }
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        cu.raw_content
            .get(self.raw_content_start()..=self.raw_content_inclusive_end())
            .unwrap()
    }
    fn merge(&self, other: &Self) -> Self {
        Self {
            start: ByteIndex::new(self.raw_content_start().min(other.raw_content_start())),
            inclusive_end: ByteIndex::new(
                self.raw_content_inclusive_end()
                    .max(other.raw_content_inclusive_end()),
            ),
        }
    }
}

#[derive(Debug, Clone, Copy)]
struct UcContentIndex(usize);

impl UcContentIndex {
    fn new(i: usize) -> Self {
        Self(i)
    }
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        cu.ucs.get(self.0).unwrap().get_str(cu)
    }
}

#[derive(Debug, Clone, Copy)]
struct UcContentIndexSpan {
    start: UcContentIndex,
    inclusive_end: UcContentIndex,
}

impl UcContentIndexSpan {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        self.get_raw_content_index_span(cu).get_str(cu)
    }

    fn get_raw_content_index_span(&self, cu: &CompilationUnit) -> ByteIndexSpan {
        cu.ucs
            .get(self.start.0)
            .unwrap()
            .merge(cu.ucs.get(self.inclusive_end.0).unwrap())
    }
}

#[derive(Debug, Clone)]
pub struct Token {
    pub(crate) kind: TokenKind,
    span: UcContentIndexSpan,
}

impl Token {
    fn from(kind: TokenKind, span: UcContentIndexSpan) -> Self {
        Self { kind, span }
    }

    pub(crate) fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        self.span.get_str(cu)
    }

    pub(crate) fn serialize(&self, cu: &CompilationUnit) -> String {
        format!("{} {}", self.kind, self.get_str(cu))
    }
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
    ucs: Vec<ByteIndexSpan>,
}

fn str_to_ucs(s: &str) -> Vec<ByteIndexSpan> {
    s.grapheme_indices(true)
        .map(|(idx, s)| ByteIndexSpan {
            start: ByteIndex::new(idx),
            inclusive_end: ByteIndex::new(idx + s.len() - 1),
        })
        .collect()
}

impl CompilationUnit {
    pub fn from_file<P>(filename: P) -> Result<Self, String>
    where
        P: AsRef<std::path::Path>,
    {
        let raw_content = std::fs::read_to_string(&filename).map_err(|e| e.to_string())?;
        let ucs = str_to_ucs(&raw_content);
        Ok(CompilationUnit {
            kind: CompilationUnitKind::FromFile {
                path: filename.as_ref().to_path_buf(),
            },
            raw_content,
            ucs,
        })
    }

    pub fn from_string(mark: &str, input: &str) -> Self {
        CompilationUnit {
            kind: CompilationUnitKind::FromString {
                mark: String::from(mark),
            },
            raw_content: String::from(input),
            ucs: str_to_ucs(input),
        }
    }

    pub fn get_origin(&self) -> String {
        match &self.kind {
            CompilationUnitKind::FromFile { path } => format!("{}", path.display()),
            CompilationUnitKind::FromString { mark } => String::from(mark),
        }
    }

    pub fn get_tokens(&self) -> Vec<Token> {
        fn uc_is_ascii_digit(s: &str) -> bool {
            s.len() == 1 && s.chars().next().unwrap().is_ascii_digit()
        }

        fn take_while(
            cu: &CompilationUnit,
            start: UcContentIndex,
            f: impl Fn(&str) -> bool,
        ) -> UcContentIndex {
            let mut i = start.0;
            while let Some(byte_idx_span) = cu.ucs.get(i) {
                let s = byte_idx_span.get_str(cu);
                if !f(s) {
                    break;
                }
                i += 1;
            }
            UcContentIndex::new(i)
        }

        fn get_single_char_token_kind(c: char) -> Option<TokenKind> {
            match c {
                '+' => Some(TokenKind::Plus),
                '-' => Some(TokenKind::Dash),
                '*' => Some(TokenKind::Star),
                '/' => Some(TokenKind::Slash),
                '^' => Some(TokenKind::Caret),
                '(' => Some(TokenKind::LParen),
                ')' => Some(TokenKind::RParen),
                _ => None,
            }
        }

        let mut tokens: Vec<Token> = Vec::<Token>::with_capacity(self.ucs.len());

        let uc_idx = UcContentIndex::new(0);

        loop {
            let byte_idx_span = self.ucs.get(uc_idx.0).unwrap();
            let s = byte_idx_span.get_str(self);
            if s.len() == 1 {
                let c = s.chars().next().unwrap();
                if let Some(token_kind) = get_single_char_token_kind(c) {
                    tokens.push(Token::from(
                        token_kind,
                        UcContentIndexSpan {
                            start: uc_idx,
                            inclusive_end: uc_idx,
                        },
                    ));
                } else {
                    match c {
                        '1'..='9' => {
                            tokens.push(Token::from(
                                TokenKind::Int64,
                                UcContentIndexSpan {
                                    start: uc_idx,
                                    inclusive_end: take_while(self, uc_idx, uc_is_ascii_digit),
                                },
                            ));
                        }
                        ' ' => {
                            tokens.push(Token::from(
                                TokenKind::Spaces,
                                UcContentIndexSpan {
                                    start: uc_idx,
                                    inclusive_end: take_while(self, uc_idx, |s| s == " "),
                                },
                            ));
                        }
                        _ => tokens.push(Token::from(
                            TokenKind::Invalid,
                            UcContentIndexSpan {
                                start: uc_idx,
                                inclusive_end: uc_idx,
                            },
                        )),
                    }
                }
            } else {
                tokens.push(Token::from(
                    TokenKind::Invalid,
                    UcContentIndexSpan {
                        start: uc_idx,
                        inclusive_end: uc_idx,
                    },
                ));
            }
        }
    }
}

#[derive(Debug, PartialEq, Eq, std::hash::Hash, Clone, Copy)]
pub(crate) enum TokenKind {
    Spaces,
    NewLine,

    Int64,

    Plus,
    Dash,
    Star,
    Slash,
    Caret,

    LParen,
    RParen,

    Invalid,
}

impl std::fmt::Display for TokenKind {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            TokenKind::Spaces => write!(f, "Spaces"),
            TokenKind::NewLine => write!(f, "NewLine"),
            TokenKind::Int64 => write!(f, "Int64"),
            TokenKind::Plus => write!(f, "Plus"),
            TokenKind::Dash => write!(f, "Dash"),
            TokenKind::Star => write!(f, "Asterisk"),
            TokenKind::Slash => write!(f, "Slash"),
            TokenKind::Caret => write!(f, "Caret"),
            TokenKind::LParen => write!(f, "LParen"),
            TokenKind::RParen => write!(f, "RParen"),
            TokenKind::Invalid => write!(f, "Invalid"),
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
            let time = std::time::SystemTime::now()
                .duration_since(std::time::SystemTime::UNIX_EPOCH)
                .unwrap()
                .as_nanos();
            let pid = std::process::id();
            let uuid = format!("{}_{}", time, pid);

            let mut temp_path = std::env::temp_dir();
            temp_path.push(format!("life_lang_test_{}", uuid));
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

    #[test]
    fn test_creating_cu_from_file() {
        for (s, sz) in [("hi, de\n", 7), ("hi, 你好\n", 7)] {
            with_temp_file(|tf| {
                tf.write(s);
                let cu = CompilationUnit::from_file(&tf.path).unwrap();
                assert_eq!(cu.raw_content, s);
                assert_eq!(cu.ucs.len(), sz, "{:?}", cu.ucs);
            });
        }
    }

    #[test]
    fn test_creating_cu_from_file_not_exists() {
        assert!(CompilationUnit::from_file("please-dont-exist").is_err());
    }

    #[test]
    fn test_creating_cu_from_string() {
        for (s, sz) in [("hi, de\n", 7), ("hi, 你好\n", 7)] {
            let cu = CompilationUnit::from_string("mark", s);
            assert_eq!(cu.raw_content, s);
            assert_eq!(cu.ucs.len(), sz, "{:?}", cu.ucs);
        }
    }

    #[test]
    fn test_get_cu_origin_from_file() {
        with_temp_file(|tf| {
            tf.write("abcefgh");
            let cu = CompilationUnit::from_file(&tf.path).unwrap();
            assert_eq!(cu.get_origin(), tf.path.to_str().unwrap());
        });
    }

    #[test]
    fn test_get_cu_origin_from_string() {
        const MARK: &str = "mark";
        let cu = CompilationUnit::from_string(MARK, "abc");
        assert_eq!(cu.get_origin(), MARK);
    }

    //    fn test_token_span_created_by_uc_worker(s: &str) {
    //        let uc_char = UCChar {
    //            raw_str: SmolStr::from(s),
    //            start_byte_offset: 42,
    //        };
    //        let token = Token::from(TokenKind::Int64, &uc_char);
    //        assert_eq!(token.kind, TokenKind::Int64);
    //        assert_eq!(token.span.start.0, uc_char.start_byte_offset);
    //        assert_eq!(
    //            token.span.end.0,
    //            uc_char.start_byte_offset + uc_char.raw_str.len()
    //        );
    //    }
    //
    //    #[test]
    //    fn test_token_span_created_by_uc_has_ascii() {
    //        test_token_span_created_by_uc_worker("5")
    //    }
    //
    //    #[test]
    //    fn test_token_span_created_by_uc_has_unicode() {
    //        test_token_span_created_by_uc_worker("您")
    //    }
    //
    //    fn test_token_span_created_by_ucs_worker(s: &str) {
    //        let cu = CompilationUnit::from_string("mark", s).unwrap();
    //        let ucs: Vec<&UCChar> = cu.uc_iter().collect();
    //        assert_eq!(ucs.len(), 2, "{:?}", ucs);
    //        let token = Token::from_range(TokenKind::Int64, ucs[0], ucs[1]);
    //        assert_eq!(token.kind, TokenKind::Int64);
    //        assert_eq!(token.span.start.0, ucs[0].start_byte_offset);
    //        assert_eq!(
    //            token.span.end.0,
    //            ucs[1].start_byte_offset + ucs[1].raw_str.len()
    //        );
    //    }
    //
    //    #[test]
    //    fn test_token_span_created_by_ucs_has_ascii() {
    //        test_token_span_created_by_ucs_worker("12")
    //    }
    //
    //    #[test]
    //    fn test_token_span_created_by_ucs_has_unicode() {
    //        test_token_span_created_by_ucs_worker("您好")
    //    }
    //
    //    fn test_token_to_string_alike_worker<F>(input: &str, f: F, result: &str)
    //    where
    //        F: for<'a> Fn(&'a Token, &'a CompilationUnit) -> String,
    //    {
    //        let cu = CompilationUnit::from_string("mark", input).unwrap();
    //        let uc_char = UCChar {
    //            raw_str: SmolStr::from(input),
    //            start_byte_offset: 0,
    //        };
    //        let token = Token::from(TokenKind::Int64, &uc_char);
    //        assert_eq!(f(&token, &cu), result);
    //    }
    //
    //    #[test]
    //    fn test_token_to_str() {
    //        test_token_to_string_alike_worker("5", |token, cu| token.to_str(cu).to_owned(), "5")
    //    }
    //
    //    #[test]
    //    fn test_token_to_str_unicode() {
    //        test_token_to_string_alike_worker("您", |token, cu| token.to_str(cu).to_owned(), "您")
    //    }
    //
    //    #[test]
    //    fn test_token_serialize_ascii() {
    //        test_token_to_string_alike_worker("5", |token, cu| token.serialize(cu), "Int64 5")
    //    }
    //
    //    #[test]
    //    fn test_token_serialize_unicode() {
    //        test_token_to_string_alike_worker("您", |token, cu| token.serialize(cu), "Int64 您")
    //    }
    //
    //    #[test]
    //    fn test_lexing_integer() {
    //        let cu = CompilationUnit::from_string("mark", "  123 ").unwrap();
    //        let tokens = cu.token_iter().collect::<Result<Vec<_>, _>>().unwrap();
    //        assert_eq!(tokens.len(), 1, "{:?}", tokens);
    //        assert_eq!(tokens[0].kind, TokenKind::Int64);
    //        assert_eq!(tokens[0].span.start.0, 2);
    //        assert_eq!(tokens[0].span.end.0, 5);
    //    }
    //
    //    #[test]
    //    fn test_lexing_empty_content() {
    //        let cu = CompilationUnit::from_string("mark", "").unwrap();
    //        let tokens = cu.token_iter().collect::<Result<Vec<_>, _>>().unwrap();
    //        assert_eq!(tokens.len(), 0, "{:?}", tokens);
    //    }
}
