mod get_tokens_utils;
mod indices;

use get_tokens_utils::{
    try_multi_byte_char, try_multi_byte_tokens, try_new_line, try_single_char_token, try_string,
};
use indices::{ByteIdx, ByteSpan, UcIdx, UcSpan};
use unicode_segmentation::UnicodeSegmentation;

#[derive(Debug, Clone, Copy)]
pub struct TokenIdx(usize);

impl TokenIdx {
    pub(super) fn new(i: usize) -> Self {
        Self(i)
    }
    fn get(&self) -> usize {
        self.0
    }
}

impl std::ops::Add<usize> for TokenIdx {
    type Output = Self;

    fn add(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

impl std::ops::AddAssign<usize> for TokenIdx {
    fn add_assign(&mut self, rhs: usize) {
        *self = *self + rhs;
    }
}

impl std::ops::Sub<usize> for TokenIdx {
    type Output = Self;

    fn sub(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_sub(rhs).unwrap())
    }
}

impl std::ops::SubAssign<usize> for TokenIdx {
    fn sub_assign(&mut self, rhs: usize) {
        *self = *self - rhs;
    }
}

#[derive(Debug, PartialEq, Eq, Clone)]
pub(super) enum TokenKind {
    Spaces,
    NewLine,
    SemiColon,

    I64,

    StringLiteral { content: String },
    Comment,

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
            TokenKind::Comment => write!(f, "//"),
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

#[derive(Debug)]
pub struct DiagCtx {
    line_starts: Vec<TokenIdx>,
    lines: Vec<ByteIdx>,
}

impl DiagCtx {
    fn with_capacity(n: usize) -> Self {
        Self {
            line_starts: Vec::with_capacity(n),
            lines: Vec::with_capacity(n),
        }
    }
    fn push(&mut self, after_new_line_token_idx: TokenIdx, line_starts_byte_idx: ByteIdx) {
        self.line_starts.push(after_new_line_token_idx);
        self.lines.push(line_starts_byte_idx);
    }
}

#[derive(Debug, Clone)]
pub(crate) struct Token {
    kind: TokenKind,
    uc_span: UcSpan,
}

impl Token {
    fn new(kind: TokenKind, uc_span: UcSpan) -> Self {
        Self { kind, uc_span }
    }

    pub(crate) fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> &'cu str {
        cu.get_str(self.uc_span).unwrap_or_else(|| {
            panic!(
                "BUG: failed to get str from token {:?} in {:?}",
                self,
                cu.get_origin()
            )
        })
    }

    pub(crate) fn get_kind(&self) -> &TokenKind {
        &self.kind
    }
}

#[derive(Debug, Clone)]
pub struct Tokens(Vec<Token>);

impl Tokens {
    fn with_capacity(n: usize) -> Self {
        Self(Vec::with_capacity(n))
    }

    fn push_token(&mut self, token: Token) {
        self.0.push(token);
    }

    fn push(&mut self, kind: TokenKind, start: UcIdx, inclusive_end: UcIdx) {
        self.push_token(Token::new(kind, UcSpan::new(start, inclusive_end)));
    }

    pub(super) fn get(&self, idx: TokenIdx) -> Option<&Token> {
        self.0.get(idx.get())
    }

    pub(super) fn len(&self) -> usize {
        self.0.len()
    }

    pub(super) fn iter(&self) -> impl Iterator<Item = &Token> {
        self.0.iter()
    }

    pub(super) fn into_iter(self) -> impl Iterator<Item = Token> {
        self.0.into_iter()
    }
}

#[derive(Debug)]
struct UcContent(Vec<ByteSpan>);

impl UcContent {
    fn from(s: &str) -> Self {
        Self(
            s.grapheme_indices(true)
                .map(|(idx, s)| ByteSpan::new(ByteIdx::new(idx), ByteIdx::new(idx + s.len() - 1)))
                .collect(),
        )
    }

    fn len(&self) -> usize {
        self.0.len()
    }

    fn get_byte_span(&self, uc_idx: UcIdx) -> Option<&ByteSpan> {
        self.0.get(uc_idx.get())
    }
}

#[derive(Debug)]
enum CompilationUnitKind {
    FromFile { path: std::path::PathBuf },
    FromString { mark: String },
}

#[derive(Debug)]
pub struct CompilationUnit {
    kind: CompilationUnitKind,
    raw_content: String,
    ucs: UcContent,
}

impl CompilationUnit {
    fn normalize_new_lines(s: &str) -> String {
        s.replace("\r\n", "\n")
    }

    pub fn from_file<P>(filename: P) -> Result<Self, String>
    where
        P: AsRef<std::path::Path>,
    {
        let raw_content = Self::normalize_new_lines(
            &std::fs::read_to_string(&filename).map_err(|e| e.to_string())?,
        );
        let ucs = UcContent::from(&raw_content);
        Ok(CompilationUnit {
            kind: CompilationUnitKind::FromFile {
                path: filename.as_ref().to_path_buf(),
            },
            raw_content,
            ucs,
        })
    }

    pub fn from_string(mark: &str, input: &str) -> Self {
        let raw_content = Self::normalize_new_lines(input);
        let ucs = UcContent::from(&raw_content);
        CompilationUnit {
            kind: CompilationUnitKind::FromString {
                mark: String::from(mark),
            },
            raw_content,
            ucs,
        }
    }

    fn get_origin(&self) -> String {
        match &self.kind {
            CompilationUnitKind::FromFile { path } => format!("{}", path.display()),
            CompilationUnitKind::FromString { mark } => String::from(mark),
        }
    }

    fn get_str(&self, span: impl Span) -> Option<&str> {
        span.get_str(self)
    }

    pub fn get_tokens(&self) -> (Tokens, DiagCtx) {
        let mut tokens = Tokens::with_capacity(self.ucs.len());
        let mut diag_ctx = DiagCtx::with_capacity(self.ucs.len());
        let mut uc_idx = UcIdx::new(0);

        while let Some(s) = self.get_str(uc_idx) {
            // illegal mutli-byte char
            if let Some(new_uc_idx) = try_multi_byte_char(self, &mut tokens, uc_idx, s) {
                uc_idx = new_uc_idx;
                continue;
            }

            let c = s.chars().next().unwrap();
            if let Some(new_uc_idx) = try_new_line(self, &mut diag_ctx, &mut tokens, uc_idx, c) {
                // new linesrc_loc, &mut
                uc_idx = new_uc_idx;
            } else if let Some(new_uc_idx) = try_single_char_token(self, &mut tokens, uc_idx, c) {
                // single-char tokens
                uc_idx = new_uc_idx;
            } else if let Some(new_uc_idx) = try_string(self, &mut diag_ctx, &mut tokens, uc_idx, c)
            {
                // string
                uc_idx = new_uc_idx;
            } else if let Some(new_uc_idx) = try_multi_byte_tokens(self, &mut tokens, uc_idx, c) {
                // multi-char tokens
                uc_idx = new_uc_idx;
            }
        }

        (tokens, diag_ctx)
    }
}

trait Span {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str>;
}

impl Span for ByteSpan {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str> {
        cu.raw_content
            .get(self.get_start().get()..=self.get_inclusive_end().get())
    }
}

impl Span for UcIdx {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str> {
        cu.get_str(self.get_byte_span(cu)?)
    }
}

impl Span for (UcIdx, UcIdx) {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str> {
        cu.get_str(UcSpan::new(self.0, self.1))
    }
}

impl Span for UcSpan {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str> {
        cu.get_str(self.get_byte_span(cu)?)
    }
}
