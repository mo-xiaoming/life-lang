#![allow(unused)]

mod get_tokens_utils;
mod indices;

use get_tokens_utils::{
    try_multi_byte_char, try_multi_byte_tokens, try_new_line, try_single_char_token,
};
use indices::{ByteIndex, ByteIndexSpan, UcContentIndex};
use unicode_segmentation::UnicodeSegmentation;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TokenIndex(usize);

impl TokenIndex {
    pub(crate) fn new(i: usize) -> Self {
        Self(i)
    }
    fn get(&self) -> usize {
        self.0
    }
}

impl std::ops::Add<usize> for TokenIndex {
    type Output = Self;

    fn add(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

impl std::ops::AddAssign<usize> for TokenIndex {
    fn add_assign(&mut self, rhs: usize) {
        *self = *self + rhs;
    }
}

impl std::ops::Sub<usize> for TokenIndex {
    type Output = Self;

    fn sub(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_sub(rhs).unwrap())
    }
}

impl std::ops::SubAssign<usize> for TokenIndex {
    fn sub_assign(&mut self, rhs: usize) {
        *self = *self - rhs;
    }
}

#[cfg(test)]
mod test_token_index {
    use super::*;

    #[test]
    fn test_token_index() {
        let index_value = 5;
        let token_index = TokenIndex::new(index_value);
        assert_eq!(token_index.get(), index_value);
    }

    #[test]
    fn test_add() {
        let index = TokenIndex::new(5);
        let result = index + 3;
        assert_eq!(result.get(), 8);
    }

    #[test]
    fn test_sub() {
        let index = TokenIndex::new(5);
        let result = index - 3;
        assert_eq!(result.get(), 2);
    }

    #[test]
    fn test_add_assign() {
        let mut index = TokenIndex::new(5);
        index += 3;
        assert_eq!(index.get(), 8);
    }

    #[test]
    fn test_sub_assign() {
        let mut index = TokenIndex::new(5);
        index -= 3;
        assert_eq!(index.get(), 2);
    }
}

#[derive(Debug, PartialEq, Eq, std::hash::Hash, Clone)]
pub(crate) enum TokenKind {
    Spaces,
    NewLine,
    SemiColon,

    I64,

    StringLiteral { content: String },

    Plus,
    Dash,
    Star,
    Slash,
    Percentage,

    LParen,
    RParen,

    Equal,

    Identifier,

    KwLet,
    KwVar,

    Invalid { msg: String },
}

impl std::fmt::Display for TokenKind {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            TokenKind::Spaces => write!(f, "Spaces"),
            TokenKind::NewLine => write!(f, "NewLine"),
            TokenKind::SemiColon => write!(f, "SemiColon"),
            TokenKind::I64 => write!(f, "I64"),
            TokenKind::StringLiteral { content } => write!(f, "\"{}\"", content),
            TokenKind::Plus => write!(f, "Plus"),
            TokenKind::Dash => write!(f, "Dash"),
            TokenKind::Star => write!(f, "Star"),
            TokenKind::Slash => write!(f, "Slash"),
            TokenKind::Percentage => write!(f, "Percentage"),
            TokenKind::Equal => write!(f, "Equal"),
            TokenKind::LParen => write!(f, "LParen"),
            TokenKind::RParen => write!(f, "RParen"),
            TokenKind::Identifier => write!(f, "Identifier"),
            TokenKind::KwLet => write!(f, "KwLet"),
            TokenKind::KwVar => write!(f, "KwVar"),
            TokenKind::Invalid { msg } => write!(f, "{}", msg),
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

    test_display!(
        Spaces, NewLine, SemiColon, I64, Plus, Dash, Star, Slash, Percentage, Equal, LParen,
        RParen, Identifier, KwLet, KwVar,
    );
}
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Token {
    kind: TokenKind,
    span: ByteIndexSpan,
}

impl Token {
    fn new(kind: TokenKind, span: ByteIndexSpan) -> Self {
        Self { kind, span }
    }

    pub(crate) fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        self.span.get_str(cu)
    }

    pub(crate) fn get_kind(&self) -> &TokenKind {
        &self.kind
    }
}

mod test_token {
    #[test]
    fn test_from() {
        use super::*;
        let span = ByteIndexSpan::new(ByteIndex::new(42), ByteIndex::new(47));
        let token = Token::new(TokenKind::I64, span);
        assert_eq!(token.kind, TokenKind::I64);
        assert_eq!(token.span, span);
    }

    #[test]
    fn test_get_str_and_serialize() {
        use super::*;
        let s = "您好";
        let cu = CompilationUnit::from_string("mark", &format!("hi, {}", s));
        let token = Token::new(
            TokenKind::I64,
            ByteIndexSpan::new(ByteIndex::new(4), ByteIndex::new(4 + s.len() - 1)),
        );
        assert_eq!(token.get_str(&cu), s);
    }
}

#[derive(Debug, Clone)]
pub(crate) struct Tokens(Vec<Token>);

impl Tokens {
    fn new(n: usize) -> Self {
        Self(Vec::with_capacity(n))
    }

    fn push_token(&mut self, token: Token) {
        self.0.push(token);
    }

    fn push(&mut self, kind: TokenKind, span: ByteIndexSpan) {
        self.push_token(Token::new(kind, span));
    }

    pub(crate) fn len(&self) -> usize {
        self.0.len()
    }

    pub(crate) fn iter(&self) -> impl Iterator<Item = &Token> {
        self.0.iter()
    }

    pub(crate) fn into_iter(self) -> impl Iterator<Item = Token> {
        self.0.into_iter()
    }
}

impl std::ops::Index<TokenIndex> for Tokens {
    type Output = Token;

    fn index(&self, index: TokenIndex) -> &Self::Output {
        self.0.get(index.get()).unwrap_or_else(|| {
            panic!(
                "BUG: TokenIndex out of bounds: {} >= {}, tokens: {:?}",
                index.get(),
                self.0.len(),
                self,
            )
        })
    }
}

#[cfg(test)]
mod test_tokens {
    use super::*;

    #[test]
    fn test() {
        // Create a new Tokens instance
        let mut tokens = Tokens::new(2);

        // Add some tokens
        let token1 = Token::new(
            TokenKind::Spaces,
            ByteIndexSpan::new(ByteIndex::new(0), ByteIndex::new(0)),
        );
        let token2 = Token::new(
            TokenKind::NewLine,
            ByteIndexSpan::new(ByteIndex::new(1), ByteIndex::new(1)),
        );
        tokens.push_token(token1.clone());
        tokens.push_token(token2.clone());

        // Check the length
        assert_eq!(tokens.len(), 2);

        // Check the iterator
        let mut iter = tokens.iter();
        assert_eq!(iter.next(), Some(&token1));
        assert_eq!(iter.next(), Some(&token2));
        assert_eq!(iter.next(), None);

        // Check indexing
        assert_eq!(tokens[TokenIndex::new(0)], token1);
        assert_eq!(tokens[TokenIndex::new(1)], token2);
    }
}

#[derive(Debug)]
struct UcContent(Vec<ByteIndexSpan>);

impl UcContent {
    fn try_to_byte_index_span(&self, index: UcContentIndex) -> Option<&ByteIndexSpan> {
        self.0.get(index.get())
    }

    fn len(&self) -> usize {
        self.0.len()
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
    ucs: UcContent,
}

fn str_to_ucs(s: &str) -> UcContent {
    UcContent(
        s.grapheme_indices(true)
            .map(|(idx, s)| {
                ByteIndexSpan::new(ByteIndex::new(idx), ByteIndex::new(idx + s.len() - 1))
            })
            .collect(),
    )
}

#[cfg(test)]
mod test_str_to_ucs {
    use super::*;

    #[test]
    fn test() {
        let s = "a̐éö̲😀\n\r\n\n";
        assert_eq!(s.len(), 19);

        // Convert the string to a UcContent instance
        let ucs = str_to_ucs(s);

        // Check the length of the UcContent instance
        assert_eq!(ucs.len(), 7);

        for (i, (start, inclusive_end)) in [
            (0, 2),   // a̐
            (3, 5),   // é
            (6, 10),  // ö̲
            (11, 14), // 😀
            (15, 15), // \n
            (16, 17), // \r\n
            (18, 18), // \n
        ]
        .iter()
        .enumerate()
        {
            // Check the start and inclusive_end of each grapheme cluster
            assert_eq!(
                ucs.try_to_byte_index_span(UcContentIndex::new(i)),
                Some(&ByteIndexSpan::new(
                    ByteIndex::new(*start),
                    ByteIndex::new(*inclusive_end)
                ))
            );
        }
    }
}

trait IndexSpan {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str;
}

impl IndexSpan for ByteIndexSpan {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        cu.raw_content
            .get(self.get_start().get()..=self.get_inclusive_end().get())
            .unwrap_or_else(|| panic!("failed to get str from {:?} in {:?}", self, cu))
    }
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

    pub(crate) fn byte_len(&self) -> usize {
        self.raw_content.len()
    }

    pub(crate) fn get_origin(&self) -> String {
        match &self.kind {
            CompilationUnitKind::FromFile { path } => format!("{}", path.display()),
            CompilationUnitKind::FromString { mark } => String::from(mark),
        }
    }

    pub(crate) fn get_tokens(&self) -> Tokens {
        let mut tokens = Tokens::new(self.ucs.len());

        let mut uc_idx = UcContentIndex::new(0);

        while let Some(byte_idx_span) = self.ucs.try_to_byte_index_span(uc_idx) {
            let s = byte_idx_span.get_str(self);

            // new line
            if let Some(new_uc_idx) = try_new_line(self, &mut tokens, uc_idx, s) {
                uc_idx = new_uc_idx;
                continue;
            }

            // illegal mutli-byte char
            if let Some(new_uc_idx) = try_multi_byte_char(self, &mut tokens, uc_idx, s) {
                uc_idx = new_uc_idx;
                continue;
            }

            let c = s.chars().next().unwrap();

            // single-char tokens
            if let Some(new_uc_idx) = try_single_char_token(self, &mut tokens, uc_idx, c) {
                uc_idx = new_uc_idx;
                continue;
            }

            // multi-char tokens
            if let Some(new_uc_idx) = try_multi_byte_tokens(self, &mut tokens, uc_idx, c) {
                uc_idx = new_uc_idx;
            } else {
                continue;
            }
        }

        tokens
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
    fn test_lexing_empty_content() {
        let cu = CompilationUnit::from_string("mark", "");
        let tokens = cu.get_tokens();
        assert_eq!(tokens.len(), 0, "{:?}", tokens);
    }

    #[test]
    fn test_get_tokens() {
        let cu = CompilationUnit::from_string("mark", "1 + 27 *  3 % (4 - -3);\n\r\n");
        let tokens = cu.get_tokens();
        let expected = [
            (TokenKind::I64, "1"),
            (TokenKind::Spaces, " "),
            (TokenKind::Plus, "+"),
            (TokenKind::Spaces, " "),
            (TokenKind::I64, "27"),
            (TokenKind::Spaces, " "),
            (TokenKind::Star, "*"),
            (TokenKind::Spaces, "  "),
            (TokenKind::I64, "3"),
            (TokenKind::Spaces, " "),
            (TokenKind::Percentage, "%"),
            (TokenKind::Spaces, " "),
            (TokenKind::LParen, "("),
            (TokenKind::I64, "4"),
            (TokenKind::Spaces, " "),
            (TokenKind::Dash, "-"),
            (TokenKind::Spaces, " "),
            (TokenKind::Dash, "-"),
            (TokenKind::I64, "3"),
            (TokenKind::RParen, ")"),
            (TokenKind::SemiColon, ";"),
            (TokenKind::NewLine, "\n"),
            (TokenKind::NewLine, "\r\n"),
        ];
        assert_eq!(tokens.len(), expected.len(), "{:?} {:?}", tokens, expected);
        for (token, (expected_kind, expected_str)) in tokens.iter().zip(expected.into_iter()) {
            assert_eq!(token.kind, expected_kind);
            assert_eq!(token.get_str(&cu), expected_str);
        }
    }

    #[test]
    fn test_let() {
        let cu = CompilationUnit::from_string("mark", "let x = 3 + 2;\nvar y = x - 1;");
        let tokens = cu.get_tokens();
        let expected = [
            (TokenKind::KwLet, "let"),
            (TokenKind::Spaces, " "),
            (TokenKind::Identifier, "x"),
            (TokenKind::Spaces, " "),
            (TokenKind::Equal, "="),
            (TokenKind::Spaces, " "),
            (TokenKind::I64, "3"),
            (TokenKind::Spaces, " "),
            (TokenKind::Plus, "+"),
            (TokenKind::Spaces, " "),
            (TokenKind::I64, "2"),
            (TokenKind::SemiColon, ";"),
            (TokenKind::NewLine, "\n"),
            (TokenKind::KwVar, "var"),
            (TokenKind::Spaces, " "),
            (TokenKind::Identifier, "y"),
            (TokenKind::Spaces, " "),
            (TokenKind::Equal, "="),
            (TokenKind::Spaces, " "),
            (TokenKind::Identifier, "x"),
            (TokenKind::Spaces, " "),
            (TokenKind::Dash, "-"),
            (TokenKind::Spaces, " "),
            (TokenKind::I64, "1"),
            (TokenKind::SemiColon, ";"),
        ];
        assert_eq!(tokens.len(), expected.len(), "{:?} {:?}", tokens, expected);
        for (token, (expected_kind, expected_str)) in tokens.iter().zip(expected.into_iter()) {
            assert_eq!(token.kind, expected_kind);
            assert_eq!(token.get_str(&cu), expected_str);
        }
    }

    #[test]
    fn test_lexing_empty_string() {
        for s in [r#""""#, r#""abc""#] {
            let cu = CompilationUnit::from_string("mark", s);
            let tokens = cu.get_tokens();
            assert_eq!(tokens.len(), 1, "{:?}", tokens);
            let token = &tokens[TokenIndex::new(0)];
            assert_eq!(token.get_str(&cu), s);
            assert!(
                matches!(token.get_kind(), TokenKind::StringLiteral { content } if format!(r#""{}""#, content) == s),
                "{:?}",
                token.get_kind(),
            );
        }
    }

    #[test]
    fn test_escaped_string() {
        for (input, expected_content) in [
            (r#""ab \r \n xx\\ \"xx\t\0y""#, "ab \r \n xx\\ \"xx\t\0y"),
            (
                r#"" \u{41} x\u{4f60}xy{}\u{597d}a\u{1f198}""#,
                " A x你xy{}好a🆘",
            ),
        ] {
            let cu = CompilationUnit::from_string("mark", input);
            let tokens = cu.get_tokens();
            assert_eq!(tokens.len(), 1, "{:?}", tokens);
            let token = &tokens[TokenIndex::new(0)];
            assert_eq!(token.get_str(&cu), input);
            assert!(
                matches!(token.get_kind(), TokenKind::StringLiteral { content } if content == expected_content),
                "expected: `{:?}`, got: `{:?}`",
                expected_content,
                token.get_kind(),
            );
        }
    }
}
