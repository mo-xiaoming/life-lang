use super::{ByteIdx, ByteSpan, CompilationUnit, TokenIdx, Tokens};
use unicode_segmentation::UnicodeSegmentation;
use unicode_width::UnicodeWidthStr;

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
pub(crate) struct Diagnostics<'cu>(Vec<SingleLineDiagnostic<'cu>>);

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
pub(crate) struct DiagCtx<'cu> {
    line_starts: Vec<(ByteIdx, &'cu str)>,
}

impl<'cu> DiagCtx<'cu> {
    pub(super) fn with_capacity(n: usize) -> Self {
        Self {
            line_starts: Vec::with_capacity(n),
        }
    }
    pub(super) fn push(&mut self, line_starts_byte_idx: ByteIdx, line: &'cu str) {
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
                        if line_byte_len > error_start_idx {
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

    pub(crate) fn get_diag_with_error_token(
        &self,
        error_token_idx: TokenIdx,
        tokens: &'cu Tokens,
        cu: &'cu CompilationUnit,
    ) -> Diagnostics<'cu> {
        let error_token = &tokens[error_token_idx];

        self.get_diags(None, error_token.uc_span.get_byte_span(cu), cu)
    }
    pub(crate) fn get_diag_with_ctx_token(
        &self,
        ctx_token_idx: TokenIdx,
        tokens: &'cu Tokens,
        cu: &'cu CompilationUnit,
    ) -> Diagnostics<'cu> {
        let ctx_token = &tokens[ctx_token_idx];
        let ctx_token_start_byte_idx = ctx_token.uc_span.get_start_byte_idx_unchecked(cu);

        self.get_diags(Some(ctx_token_start_byte_idx), None, cu)
    }
    pub(crate) fn get_diag_with_ctx_and_error_tokens(
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
