use super::{
    indices::UcIdx, indices::UcSpan, CompilationUnit, DiagCtx, TokenIdx, TokenKind, Tokens,
};

trait StringLike {
    fn is_new_line(&self) -> bool;
}

impl StringLike for char {
    fn is_new_line(&self) -> bool {
        *self == '\n'
    }
}

impl StringLike for &str {
    fn is_new_line(&self) -> bool {
        self == &"\n"
    }
}

fn uc_is_ascii_digit(s: &str) -> bool {
    s.len() == 1 && s.chars().next().unwrap().is_ascii_digit()
}

fn take_while(cu: &CompilationUnit, mut start: UcIdx, f: impl Fn(&str) -> bool) -> UcIdx {
    while let Some(s) = cu.get_str(start) {
        if !f(s) {
            return start - 1;
        }
        start += 1;
    }
    start - 1
}

fn take_unicode(
    cu: &CompilationUnit,
    mut start: UcIdx,
) -> Result<(UcIdx, String), (UcIdx, String)> {
    let origin_start = start;

    if let Some(s) = cu.get_str(start) {
        if s != "{" {
            return Err((
                start,
                "unicode should be in the format of \\u{{...}}".to_owned(),
            ));
        }
    } else {
        return Err((
            start,
            "unicode should be in the format of \\u{abcdef}".to_owned(),
        ));
    };
    start += 1;

    while let Some(s) = cu.get_str(start) {
        if s == "}" {
            break;
        }
        if s.len() != 1 || !s.chars().next().unwrap().is_ascii_hexdigit() {
            return Err((
                start,
                format!(
                    "only hex numbers are allowed in unicode sequence, `{}` is not allowed",
                    s
                ),
            ));
        }
        start += 1;
    }

    if start == origin_start + 1 {
        return Err((
            start,
            "unicode should be in the format of \\u{...}, cannot be empty between `{}`".to_owned(),
        ));
    }
    let mut s = cu
        .get_str(UcSpan::new(origin_start + 1, start - 1))
        .unwrap()
        .to_owned();
    if s.len() % 2 != 0 {
        s = format!("0{}", s);
    }
    let c = std::char::from_u32(
        u32::from_str_radix(&s, 16)
            .unwrap_or_else(|e| panic!("BUG: failed to parse `{}` as hex number: {}", s, e)),
    )
    .ok_or_else(|| {
        (
            start,
            format!("failed to convert `{}` to unicode character", s),
        )
    })?;
    Ok((start, c.to_string()))
}

fn take_string(cu: &CompilationUnit, mut start: UcIdx) -> Result<(UcIdx, String), (UcIdx, String)> {
    let mut content = String::with_capacity(50);

    let escaped_chars: std::collections::HashMap<&str, &str> = [
        ("\\", "\\"),
        ("\"", "\""),
        ("n", "\n"),
        ("r", "\r"),
        ("t", "\t"),
        ("0", "\0"),
    ]
    .into_iter()
    .collect();
    let mut in_escape = false;
    while let Some(s) = cu.get_str(start) {
        if in_escape {
            if s == "u" {
                let (new_start, chunk) = take_unicode(cu, start + 1)?;
                start = new_start + 1;
                content.push_str(&chunk);
            } else if escaped_chars.contains_key(&s) {
                start += 1;
                content.push_str(escaped_chars[&s]);
            } else {
                return Err((start - 1, format!("unrecogonized escape `{}`", s)));
            }
            in_escape = false;
            continue;
        } else if s == r#"""# {
            return Ok((start, content));
        }
        if s == "\\" {
            in_escape = true;
        } else {
            content.push_str(s);
        }
        start += 1;
    }

    Err((start - 1, "unfinished string".to_owned()))
}

fn get_single_char_token_kind(c: char) -> Option<TokenKind> {
    match c {
        '+' => Some(TokenKind::Plus),
        '-' => Some(TokenKind::Dash),
        '*' => Some(TokenKind::Star),
        '/' => Some(TokenKind::Slash),
        '%' => Some(TokenKind::Percentage),
        '(' => Some(TokenKind::LParen),
        ')' => Some(TokenKind::RParen),
        ';' => Some(TokenKind::SemiColon),
        '=' => Some(TokenKind::Equal),
        _ => None,
    }
}

fn get_keyword(s: &str) -> Option<TokenKind> {
    match s {
        "let" => Some(TokenKind::KwLet),
        "var" => Some(TokenKind::KwVar),
        _ => None,
    }
}

pub(crate) fn try_new_line(
    cu: &CompilationUnit,
    diag_ctx: &mut DiagCtx,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    if !c.is_new_line() {
        return None;
    }

    tokens.push(TokenKind::NewLine, uc_idx, uc_idx);
    diag_ctx.push(
        TokenIdx::new(tokens.len()),
        uc_idx
            .get_byte_span(cu)
            .map(|span| span.get_inclusive_end() + 1)
            .unwrap_or_else(|| panic!("BUG: failed to get byte span for {:?}, no way!", uc_idx)),
    );
    Some(uc_idx + 1)
}

pub(crate) fn try_multi_byte_char(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    s: &str,
) -> Option<UcIdx> {
    if s.len() != 1 {
        let new_uc_idx = take_while(cu, uc_idx + 1, |s| s.len() != 1);
        tokens.push(
            TokenKind::Invalid {
                msg: format!(
                    "multi-char unicode like `{}` only supported in strings and comments",
                    s
                ),
            },
            uc_idx,
            new_uc_idx,
        );
        Some(new_uc_idx + 1)
    } else {
        None
    }
}

pub(crate) fn try_single_char_token(
    _cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    if let Some(token_kind) = get_single_char_token_kind(c) {
        tokens.push(token_kind, uc_idx, uc_idx);
        Some(uc_idx + 1)
    } else {
        None
    }
}

pub(crate) fn try_string(
    cu: &CompilationUnit,
    diag_ctx: &mut DiagCtx,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    if c != '"' {
        return None;
    }

    let new_uc_idx = take_string(cu, uc_idx + 1);
    let (kind, new_uc_idx) = match new_uc_idx {
        Ok((new_uc_idx, content)) => {
            content
                .char_indices()
                .filter(|(_, c)| c.is_new_line())
                .for_each(|(i, _)| {
                    diag_ctx.push(
                        TokenIdx::new(tokens.len()),
                        uc_idx
                            .get_byte_span(cu)
                            .map(|span| span.get_inclusive_end() + 1 + i)
                            .unwrap_or_else(|| {
                                panic!("BUG: failed to get byte span for {:?}, no way!", uc_idx)
                            }),
                    );
                });
            (TokenKind::StringLiteral { content }, new_uc_idx)
        }
        Err((new_uc_idx, msg)) => (TokenKind::Invalid { msg }, new_uc_idx),
    };
    tokens.push(kind, uc_idx, new_uc_idx);
    Some(new_uc_idx + 1)
}

pub(crate) fn try_multi_byte_tokens(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    Some(match c {
        '0' => must_be_single_zero(cu, tokens, uc_idx),
        '1'..='9' => must_be_integer(cu, tokens, uc_idx),
        ' ' => must_be_spaces(cu, tokens, uc_idx),
        'a'..='z' | 'A'..='Z' | '_' => must_be_name(cu, tokens, uc_idx, c),
        '/' => must_be_comment(cu, tokens, uc_idx),
        _ => invalid_stuff(cu, tokens, uc_idx, c),
    })
}

fn must_be_comment(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    if let Some(next_slash) = cu.get_str(uc_idx + 1) {
        if next_slash == "/" {
            let new_uc_idx = take_while(cu, uc_idx + 2, |s| !s.is_new_line());
            tokens.push(TokenKind::Comment, uc_idx, new_uc_idx);
            new_uc_idx + 1
        } else {
            tokens.push(
                TokenKind::Invalid {
                    msg: "comment must be starts with `//`".to_owned(),
                },
                uc_idx,
                uc_idx + 1,
            );
            uc_idx + 1
        }
    } else {
        tokens.push(
            TokenKind::Invalid {
                msg: "comment must be starts with `//`".to_owned(),
            },
            uc_idx,
            uc_idx,
        );
        uc_idx + 1
    }
}

fn must_be_single_zero(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let new_uc_idx = take_while(cu, uc_idx + 1, uc_is_ascii_digit);
    let kind = if new_uc_idx == uc_idx {
        TokenKind::I64
    } else {
        TokenKind::Invalid {
            msg: "leading zero is not allowed".to_owned(),
        }
    };
    tokens.push(kind, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_integer(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let new_uc_idx = take_while(cu, uc_idx + 1, uc_is_ascii_digit);
    tokens.push(TokenKind::I64, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_spaces(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let new_uc_idx = take_while(cu, uc_idx + 1, |s| s == " ");
    tokens.push(TokenKind::Spaces, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_name(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx, c: char) -> UcIdx {
    let new_uc_idx = if c == '_' {
        take_while(cu, uc_idx + 1, |s| {
            s.len() == 1 && s.chars().next().unwrap().is_ascii_alphanumeric()
        })
    } else {
        take_while(cu, uc_idx + 1, |s| {
            s.len() == 1 && s.chars().next().unwrap().is_ascii_alphanumeric() || s.starts_with('_')
        })
    };

    let s = cu
        .get_str((uc_idx, new_uc_idx))
        .unwrap_or_else(|| panic!("BUG: failed to get str"));
    if let Some(kind) = get_keyword(s) {
        tokens.push(kind, uc_idx, new_uc_idx);
    } else {
        tokens.push(TokenKind::Identifier, uc_idx, new_uc_idx);
    }
    new_uc_idx + 1
}

fn invalid_stuff(_cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx, c: char) -> UcIdx {
    tokens.push(
        TokenKind::Invalid {
            msg: format!("unsupported `{}`", c),
        },
        uc_idx,
        uc_idx,
    );
    uc_idx + 1
}
