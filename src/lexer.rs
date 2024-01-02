mod get_tokens_utils;
mod indices;

use get_tokens_utils::{
    try_multi_byte_char, try_multi_byte_tokens, try_new_line, try_single_char_token, try_string,
};
use indices::{ByteIdx, ByteSpan, UcIdx, UcSpan};
use unicode_segmentation::UnicodeSegmentation;
use unicode_width::UnicodeWidthStr;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
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
    Spaces {
        count: usize,
    },
    NewLine,
    SemiColon,

    I64,

    StringLiteral {
        content: String,
    },
    Comment,

    Plus,
    Dash,
    Star,
    Slash,
    Percentage,

    LParen,
    RParen,

    Equal,

    Identifier {
        name: String,
    },

    KwLet,
    KwVar,

    Invalid {
        msg: String,
        error_fake_token_idx: TokenIdx,
    },

    FakeTokenForInvalid,
}

impl std::fmt::Display for TokenKind {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            TokenKind::Spaces { .. } => write!(f, "Spaces"),
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
            TokenKind::Identifier { .. } => write!(f, "Identifier"),
            TokenKind::KwLet => write!(f, "KwLet"),
            TokenKind::KwVar => write!(f, "KwVar"),
            TokenKind::Invalid { msg, .. } => write!(f, "{}", msg),
            TokenKind::FakeTokenForInvalid => write!(f, "FakeTokenForInvalid"),
        }
    }
}

pub(super) trait TokenKindRepr {
    fn get_string_repr(&self) -> String;
}

impl TokenKindRepr for TokenKind {
    fn get_string_repr(&self) -> String {
        match self {
            TokenKind::Spaces { count } => " ".repeat(*count),
            TokenKind::NewLine => String::from("\n"),
            TokenKind::SemiColon => String::from(";"),
            TokenKind::StringLiteral { content } => format!("\"{}\"", content),
            TokenKind::Comment => String::from("//"),
            TokenKind::Plus => String::from("+"),
            TokenKind::Dash => String::from("-"),
            TokenKind::Star => String::from("*"),
            TokenKind::Slash => String::from("/"),
            TokenKind::Percentage => String::from("%"),
            TokenKind::Equal => String::from("="),
            TokenKind::LParen => String::from("("),
            TokenKind::RParen => String::from(")"),
            TokenKind::Identifier { name } => name.clone(),
            TokenKind::KwLet => String::from("let"),
            TokenKind::KwVar => String::from("var"),
            _ => self.to_string(),
        }
    }
}

#[derive(Debug, Clone, Copy)]
struct LineNr(usize);

impl LineNr {
    fn new(i: usize) -> Self {
        Self(i)
    }
    fn get(&self) -> usize {
        self.0
    }
}

#[derive(Debug, Clone, Copy)]
struct ColNr(usize);

impl ColNr {
    fn new(i: usize) -> Self {
        Self(i)
    }
}

#[derive(Debug)]
pub(super) struct SingleLineDiagnostic<'cu> {
    line: &'cu str,
    line_nr: LineNr,
    _col_nr: ColNr,
    leading_width: usize,
    ctx_width: usize,
    error_width: usize,
}

impl std::fmt::Display for SingleLineDiagnostic<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{: >5}|{}", self.line_nr.get(), self.line)?;
        if !self.line.ends_with('\n') {
            writeln!(f)?;
        }
        if self.line != "\n" {
            writeln!(
                f,
                "{: >5}|{: >leading_str_width$}{:~>ctx_width$}{:^>error_width$}",
                "",
                "",
                "",
                "",
                leading_str_width = self.leading_width,
                ctx_width = self.ctx_width,
                error_width = self.error_width
            )?;
        }
        Ok(())
    }
}

#[derive(Debug)]
pub(super) struct Diagnostics<'cu>(Vec<SingleLineDiagnostic<'cu>>);

impl<'cu> Diagnostics<'cu> {
    fn new() -> Self {
        Self(Vec::new())
    }

    fn push(&mut self, diag: SingleLineDiagnostic<'cu>) {
        self.0.push(diag);
    }

    fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
}

impl std::fmt::Display for Diagnostics<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        for diag in &self.0 {
            write!(f, "{}", diag)?;
        }
        Ok(())
    }
}

#[derive(Debug)]
pub(super) struct DiagCtx<'cu> {
    line_starts: Vec<(ByteIdx, &'cu str)>,
}

impl<'cu> DiagCtx<'cu> {
    fn with_capacity(n: usize) -> Self {
        Self {
            line_starts: Vec::with_capacity(n),
        }
    }
    fn push(&mut self, line_starts_byte_idx: ByteIdx, line: &'cu str) {
        self.line_starts.push((line_starts_byte_idx, line));
    }

    fn get_line(&self, start_byte_idx: ByteIdx) -> (LineNr, &'cu str) {
        self.line_starts
            .iter()
            .zip(
                self.line_starts
                    .iter()
                    .skip(1)
                    .chain(std::iter::repeat(&(ByteIdx::MAX, ""))),
            )
            .enumerate()
            .find(|(_, ((idx1, _), (idx2, _)))| *idx1 <= start_byte_idx && start_byte_idx < *idx2)
            .map(|(line_nr, ((_, line), _))| (LineNr::new(line_nr + 1), *line))
            .unwrap_or_else(|| panic!("BUG: no line found"))
    }

    fn get_diags(
        &self,
        mut ctx_start_byte_idx: Option<ByteIdx>,
        mut error_byte_span: Option<ByteSpan>,
        cu: &'cu CompilationUnit,
    ) -> Diagnostics<'cu> {
        let mut diags = Diagnostics::new();

        loop {
            let start_byte_idx = match (ctx_start_byte_idx, error_byte_span) {
                (Some(real_ctx_start_byte_idx), Some(real_error_byte_span))
                    if real_ctx_start_byte_idx == real_error_byte_span.get_start() =>
                {
                    // ctx and error are the same
                    ctx_start_byte_idx = None;
                    real_error_byte_span.get_start()
                }
                (Some(ctx_start_byte_idx), _) => ctx_start_byte_idx,
                (None, Some(error_byte_span)) => error_byte_span.get_start(),
                (None, None) => break,
            };
            if start_byte_idx.get() == cu.bytes_len() {
                break;
            }
            let (line_nr, line) = self.get_line(start_byte_idx);
            let line_start_byte_idx = cu.bytes_offset(line);

            let (leading_str, ctx_str, error_str) = match ctx_start_byte_idx {
                // has ctx
                Some(real_ctx_start_byte_idx) => {
                    let leading_str =
                        &line[..(real_ctx_start_byte_idx.get() - line_start_byte_idx)];
                    // has error
                    let (ctx_str, error_str) = if let Some(real_error_byte_span) = error_byte_span {
                        let error_start_idx =
                            real_error_byte_span.get_start().get() - line_start_byte_idx;
                        let error_inclusive_end_idx =
                            real_error_byte_span.get_inclusive_end().get() - line_start_byte_idx;
                        let line_byte_len = line.as_bytes().len();
                        if line_byte_len > error_start_idx + 1 {
                            // enough space for ctx, might not for error
                            let ctx_str = &line[leading_str.len()..error_start_idx];
                            ctx_start_byte_idx = None;
                            let error_str = if line_byte_len > error_inclusive_end_idx {
                                // enough space for error
                                error_byte_span = None;
                                &line[error_start_idx..=error_inclusive_end_idx]
                            } else {
                                // not enough space for error
                                let error_str = &line[error_start_idx..];
                                error_byte_span = Some(ByteSpan::new(
                                    real_error_byte_span.get_start() + error_str.as_bytes().len(),
                                    real_error_byte_span.get_inclusive_end(),
                                ));
                                error_str
                            };
                            (ctx_str, error_str)
                        } else {
                            // not enough space for ctx
                            let ctx_str = &line[leading_str.len()..];
                            ctx_start_byte_idx =
                                Some(real_ctx_start_byte_idx + ctx_str.as_bytes().len());
                            (ctx_str, "")
                        }
                    } else {
                        // no error
                        let ctx_str = &line[leading_str.len()..];
                        ctx_start_byte_idx =
                            Some(real_ctx_start_byte_idx + ctx_str.as_bytes().len());
                        (ctx_str, "")
                    };
                    (leading_str, ctx_str, error_str)
                }
                None => {
                    // no ctx
                    let real_error_byte_span = error_byte_span.unwrap_or_else(|| {
                        panic!("BUG: no ctx_start_byte_idx or error_byte_span",)
                    });
                    let error_start_idx =
                        real_error_byte_span.get_start().get() - line_start_byte_idx;
                    let error_inclusive_end_idx =
                        real_error_byte_span.get_inclusive_end().get() - line_start_byte_idx;
                    let line_byte_len = line.as_bytes().len();
                    let leading_str = &line[..error_start_idx];
                    let error_str = if line_byte_len > error_inclusive_end_idx {
                        // enough for error
                        error_byte_span = None;
                        &line[error_start_idx..=error_inclusive_end_idx]
                    } else {
                        // not enough for error
                        let error_str = &line[error_start_idx..];
                        error_byte_span = Some(ByteSpan::new(
                            real_error_byte_span.get_start() + error_str.as_bytes().len(),
                            real_error_byte_span.get_inclusive_end(),
                        ));
                        error_str
                    };
                    (leading_str, "", error_str)
                }
            };
            let leading_width = UnicodeWidthStr::width(leading_str);
            let error_width = UnicodeWidthStr::width(error_str);
            let ctx_width = UnicodeWidthStr::width(ctx_str);
            let col_nr = ColNr::new(leading_str.grapheme_indices(true).count() + 1);

            diags.push(SingleLineDiagnostic {
                line,
                line_nr,
                _col_nr: col_nr,
                leading_width,
                ctx_width,
                error_width,
            });
        }

        if diags.is_empty() {
            panic!("BUG: no diags");
        }

        diags
    }

    pub(super) fn get_diag_with_error_token(
        &self,
        error_token_idx: TokenIdx,
        tokens: &'cu Tokens,
        cu: &'cu CompilationUnit,
    ) -> Diagnostics<'cu> {
        let error_token = &tokens[error_token_idx];

        self.get_diags(None, error_token.uc_span.get_byte_span(cu), cu)
    }
    pub(super) fn get_diag_with_ctx_token(
        &self,
        ctx_token_idx: TokenIdx,
        tokens: &'cu Tokens,
        cu: &'cu CompilationUnit,
    ) -> Diagnostics<'cu> {
        let ctx_token = &tokens[ctx_token_idx];
        let ctx_token_start_byte_idx = ctx_token.uc_span.get_start_byte_idx_unchecked(cu);

        self.get_diags(Some(ctx_token_start_byte_idx), None, cu)
    }
    pub(super) fn get_diag_with_ctx_and_error_tokens(
        &self,
        ctx_token_idx: TokenIdx,
        error_token_idx: TokenIdx,
        tokens: &'cu Tokens,
        cu: &'cu CompilationUnit,
    ) -> Diagnostics<'cu> {
        let ctx_token = &tokens[ctx_token_idx];
        let ctx_start_byte_idx = ctx_token.uc_span.get_start_byte_idx_unchecked(cu);

        let error_token = &tokens[error_token_idx];

        self.get_diags(
            Some(ctx_start_byte_idx),
            error_token.uc_span.get_byte_span(cu),
            cu,
        )
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
pub(crate) struct Tokens(Vec<Token>);

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

    pub(super) fn find_next_non_blank_token(
        &self,
        next_token_idx: TokenIdx,
    ) -> Option<(TokenIdx, &Token)> {
        self.iter()
            .skip(next_token_idx.get())
            .enumerate()
            .find(|(_, token)| {
                !matches!(
                    token.get_kind(),
                    TokenKind::Spaces { .. } | TokenKind::NewLine
                )
            })
            .map(|(i, token)| (TokenIdx::new(i + next_token_idx.get()), token))
    }
}

impl std::ops::Index<TokenIdx> for Tokens {
    type Output = Token;

    fn index(&self, index: TokenIdx) -> &Self::Output {
        self.get(index).unwrap_or_else(|| {
            panic!(
                "BUG: failed to get token {:?} from tokens {:?}",
                index, self
            )
        })
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

    fn bytes_len(&self) -> usize {
        self.raw_content.as_bytes().len()
    }

    fn bytes_offset(&self, s: &str) -> usize {
        s.as_ptr() as usize - self.raw_content.as_ptr() as usize
    }

    pub(crate) fn get_tokens(&self) -> (Tokens, DiagCtx) {
        let mut tokens = Tokens::with_capacity(self.ucs.len());
        let mut uc_idx = UcIdx::new(0);

        while let Some(s) = self.get_str(uc_idx) {
            // illegal mutli-byte char
            if let Some(new_uc_idx) = try_multi_byte_char(self, &mut tokens, uc_idx, s) {
                uc_idx = new_uc_idx;
                continue;
            }

            let c = s.chars().next().unwrap();
            if let Some(new_uc_idx) = try_new_line(self, &mut tokens, uc_idx, c) {
                // new linesrc_loc, &mut
                uc_idx = new_uc_idx;
            } else if let Some(new_uc_idx) = try_single_char_token(self, &mut tokens, uc_idx, c) {
                // single-char tokens
                uc_idx = new_uc_idx;
            } else if let Some(new_uc_idx) = try_string(self, &mut tokens, uc_idx, c) {
                // string
                uc_idx = new_uc_idx;
            } else if let Some(new_uc_idx) = try_multi_byte_tokens(self, &mut tokens, uc_idx, c) {
                // multi-char tokens
                uc_idx = new_uc_idx;
            }
        }

        let diag_ctx = {
            let mut diag_ctx = DiagCtx::with_capacity(tokens.len() / 25);
            let mut byte_idx = 0;
            for line in self.raw_content.split_inclusive('\n') {
                diag_ctx.push(ByteIdx::new(byte_idx), line);
                byte_idx += line.as_bytes().len();
            }
            diag_ctx
        };

        (tokens, diag_ctx)
    }
}

trait Span {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str>;
}

impl Span for (ByteIdx, Option<ByteIdx>) {
    fn get_str<'cu>(&self, cu: &'cu CompilationUnit) -> Option<&'cu str> {
        match self.1 {
            Some(e) => cu.get_str(ByteSpan::new(self.0, e)),
            None => cu.raw_content.get(self.0.get()..),
        }
    }
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
