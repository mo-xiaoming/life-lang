#![allow(dead_code)]

use unicode_segmentation::UnicodeSegmentation;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct ByteIndex(usize);

impl ByteIndex {
    fn new(i: usize) -> Self {
        Self(i)
    }
    fn get(&self) -> usize {
        self.0
    }
}

#[cfg(test)]
mod test_byte_index {
    use super::*;

    #[test]
    fn test_new() {
        for v in [0, usize::MAX, 42] {
            assert_eq!(ByteIndex::new(v).get(), v);
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct ByteIndexSpan {
    start: ByteIndex,
    inclusive_end: ByteIndex,
}

impl ByteIndexSpan {
    fn new(start: ByteIndex, inclusive_end: ByteIndex) -> Self {
        Self {
            start,
            inclusive_end,
        }
    }
    fn byte_idx_start(&self) -> ByteIndex {
        self.start
    }
    fn byte_idx_inclusive_end(&self) -> ByteIndex {
        self.inclusive_end
    }
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        cu.raw_content
            .get(self.byte_idx_start().get()..=self.byte_idx_inclusive_end().get())
            .unwrap()
    }
    fn merge(&self, other: &Self) -> Self {
        Self {
            start: ByteIndex::new(
                self.byte_idx_start()
                    .get()
                    .min(other.byte_idx_start().get()),
            ),
            inclusive_end: ByteIndex::new(
                self.byte_idx_inclusive_end()
                    .get()
                    .max(other.byte_idx_inclusive_end().get()),
            ),
        }
    }
}

#[cfg(test)]
mod test_byte_index_span {
    use super::*;

    #[test]
    fn test_raw_content_start_and_end() {
        let start = ByteIndex::new(5);
        let end = ByteIndex::new(10);
        let span = ByteIndexSpan::new(start, end);
        assert_eq!(span.byte_idx_start(), start);
        assert_eq!(span.byte_idx_inclusive_end(), end);
    }

    #[test]
    fn test_merge() {
        for ((s1, e1), (s2, e2), (s3, e3)) in
            [((5, 10), (8, 15), (5, 15)), ((5, 10), (10, 15), (5, 15))]
        {
            let span1 = ByteIndexSpan::new(ByteIndex::new(s1), ByteIndex::new(e1));
            let span2 = ByteIndexSpan::new(ByteIndex::new(s2), ByteIndex::new(e2));
            let merged = span1.merge(&span2);
            assert_eq!(
                merged,
                ByteIndexSpan::new(ByteIndex::new(s3), ByteIndex::new(e3))
            );
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct UcContentIndex(usize);

impl UcContentIndex {
    fn new(i: usize) -> Self {
        Self(i)
    }
    fn get(&self) -> usize {
        self.0
    }
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        cu.ucs.get(self.0).unwrap().get_str(cu)
    }
}

#[cfg(test)]
mod test_uc_content_index {
    use super::*;

    #[test]
    fn test_new_and_get() {
        for v in [0, usize::MAX, 42] {
            assert_eq!(UcContentIndex::new(v).get(), v);
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

#[cfg(test)]
fn cu_has_unicode() -> CompilationUnit {
    CompilationUnit::from_string("mark", "hi, 您好")
}

#[cfg(test)]
mod test_uc_content_index_span {
    use super::*;

    #[test]
    fn test_span_get_str() {
        let cu = cu_has_unicode();
        for (b, e, s) in [(2, 2, ","), (4, 4, "您"), (5, 5, "好")] {
            let span = UcContentIndexSpan {
                start: UcContentIndex::new(b),
                inclusive_end: UcContentIndex::new(e),
            };
            assert_eq!(span.get_str(&cu), s);
        }
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
}

#[cfg(test)]
mod test_token {
    use super::*;

    #[test]
    fn test_from() {
        let span = UcContentIndexSpan {
            start: UcContentIndex::new(4),
            inclusive_end: UcContentIndex::new(5),
        };
        let token = Token::from(TokenKind::Invalid, span);
        assert_eq!(token.kind, TokenKind::Invalid);
        assert_eq!(token.span, span);
    }

    #[test]
    fn test_get_str_and_serialize() {
        let cu = cu_has_unicode();
        let span = UcContentIndexSpan {
            start: UcContentIndex::new(4),
            inclusive_end: UcContentIndex::new(5),
        };
        let token = Token::from(TokenKind::Invalid, span);
        assert_eq!(token.get_str(&cu), "您好");
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

#[cfg(test)]
mod test_compile_unit {
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
    #[test]
    fn test_get_tokens() {
        let cu = CompilationUnit::from_string("mark", "1 + 27 *  3 ^ (4 - -3)\n");
        let tokens = cu.get_tokens();
        let expected = [
            (TokenKind::Int64, "1"),
            (TokenKind::Spaces, " "),
            (TokenKind::Plus, "+"),
            (TokenKind::Spaces, " "),
            (TokenKind::Int64, "27"),
            (TokenKind::Spaces, " "),
            (TokenKind::Star, "*"),
            (TokenKind::Spaces, " "),
            (TokenKind::Spaces, " "),
            (TokenKind::Int64, "3"),
            (TokenKind::Spaces, " "),
            (TokenKind::Caret, "^"),
            (TokenKind::Spaces, " "),
            (TokenKind::LParen, "("),
            (TokenKind::Int64, "4"),
            (TokenKind::Spaces, " "),
            (TokenKind::Dash, "-"),
            (TokenKind::Spaces, " "),
            (TokenKind::Dash, "-"),
            (TokenKind::Int64, "3"),
            (TokenKind::RParen, ")"),
            (TokenKind::NewLine, "\n"),
        ];
        assert_eq!(tokens.len(), expected.len());
        for (token, (expected_kind, expected_str)) in tokens.iter().zip(expected.into_iter()) {
            assert_eq!(token.kind, expected_kind);
            assert_eq!(token.get_str(&cu), expected_str);
        }
    }

    #[test]
    fn test_lexing_empty_content() {
        let cu = CompilationUnit::from_string("mark", "");
        let tokens = cu.get_tokens();
        assert_eq!(tokens.len(), 0, "{:?}", tokens);
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
            TokenKind::Star => write!(f, "Star"),
            TokenKind::Slash => write!(f, "Slash"),
            TokenKind::Caret => write!(f, "Caret"),
            TokenKind::LParen => write!(f, "LParen"),
            TokenKind::RParen => write!(f, "RParen"),
            TokenKind::Invalid => write!(f, "Invalid"),
        }
    }
}

#[cfg(test)]
mod test_token_kind {
    use super::*;

    macro_rules! test_display {
        ($($kind:ident),* $(,)?) => {
            #[test]
            fn test() {
                $(
                assert_eq!(format!("{}", TokenKind::$kind), stringify!($kind));
                )*
            }
        }
    }

    test_display!(Spaces, NewLine, Int64, Plus, Dash, Star, Slash, Caret, LParen, RParen, Invalid,);
}
