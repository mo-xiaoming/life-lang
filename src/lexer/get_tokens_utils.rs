use super::{ByteIndexSpan, CompilationUnit, IndexSpan, TokenKind, Tokens, UcContentIndex};

fn uc_is_ascii_digit(s: &str) -> bool {
    s.len() == 1 && s.chars().next().unwrap().is_ascii_digit()
}

fn take_while(
    cu: &CompilationUnit,
    mut start: UcContentIndex,
    f: impl Fn(&str) -> bool,
) -> UcContentIndex {
    while let Some(byte_idx_span) = cu.ucs.try_to_byte_index_span(start) {
        let s = byte_idx_span.get_str(cu);
        if !f(s) {
            return start - 1;
        }
        start += 1;
    }
    start - 1
}

fn take_unicode(
    cu: &CompilationUnit,
    mut start: UcContentIndex,
) -> Result<(UcContentIndex, String), (UcContentIndex, String)> {
    let origin_start = start;

    if let Some(byte_idx_span) = cu.ucs.try_to_byte_index_span(start) {
        let s = byte_idx_span.get_str(cu);
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

    while let Some(byte_idx_span) = cu.ucs.try_to_byte_index_span(start) {
        let s = byte_idx_span.get_str(cu);
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
    let mut s = ByteIndexSpan::new(
        cu.ucs
            .try_to_byte_index_span(origin_start + 1)
            .unwrap()
            .get_start(),
        cu.ucs
            .try_to_byte_index_span(start - 1)
            .unwrap()
            .get_inclusive_end(),
    )
    .get_str(cu)
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

fn take_string(
    cu: &CompilationUnit,
    mut start: UcContentIndex,
) -> Result<(UcContentIndex, String), (UcContentIndex, String)> {
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
    while let Some(byte_idx_span) = cu.ucs.try_to_byte_index_span(start) {
        let s = byte_idx_span.get_str(cu);
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

fn to_byte_range(
    cu: &CompilationUnit,
    start: UcContentIndex,
    inclusive_end: UcContentIndex,
) -> ByteIndexSpan {
    ByteIndexSpan::new(
        cu.ucs
            .try_to_byte_index_span(start)
            .unwrap_or_else(|| panic!("failed to get byte index span for {:?} in {:?}", start, cu))
            .get_start(),
        cu.ucs
            .try_to_byte_index_span(inclusive_end)
            .unwrap_or_else(|| {
                panic!(
                    "failed to get byte index span for {:?} in {:?}",
                    inclusive_end, cu
                )
            })
            .get_inclusive_end(),
    )
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
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
    s: &str,
) -> Option<UcContentIndex> {
    if s == "\r\n" || s == "\n" {
        tokens.push(TokenKind::NewLine, to_byte_range(cu, uc_idx, uc_idx));
        Some(uc_idx + 1)
    } else {
        None
    }
}

pub(crate) fn try_multi_byte_char(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
    s: &str,
) -> Option<UcContentIndex> {
    if s.len() != 1 {
        let new_uc_idx = take_while(cu, uc_idx + 1, |s| s.len() != 1);
        tokens.push(
            TokenKind::Invalid {
                msg: format!(
                    "multi-char unicode like `{}` only supported in strings and comments",
                    s
                ),
            },
            to_byte_range(cu, uc_idx, new_uc_idx),
        );
        Some(new_uc_idx + 1)
    } else {
        None
    }
}

pub(crate) fn try_single_char_token(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
    c: char,
) -> Option<UcContentIndex> {
    if let Some(token_kind) = get_single_char_token_kind(c) {
        tokens.push(token_kind, to_byte_range(cu, uc_idx, uc_idx));
        Some(uc_idx + 1)
    } else {
        None
    }
}

fn must_be_single_zero(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
) -> UcContentIndex {
    let new_uc_idx = take_while(cu, uc_idx + 1, uc_is_ascii_digit);
    let kind = if new_uc_idx == uc_idx {
        TokenKind::I64
    } else {
        TokenKind::Invalid {
            msg: "leading zero is not allowed".to_owned(),
        }
    };
    tokens.push(kind, to_byte_range(cu, uc_idx, new_uc_idx));
    new_uc_idx + 1
}

fn must_be_integer(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
) -> UcContentIndex {
    let new_uc_idx = take_while(cu, uc_idx + 1, uc_is_ascii_digit);
    tokens.push(TokenKind::I64, to_byte_range(cu, uc_idx, new_uc_idx));
    new_uc_idx + 1
}

fn must_be_spaces(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
) -> UcContentIndex {
    let new_uc_idx = take_while(cu, uc_idx + 1, |s| s == " ");
    tokens.push(TokenKind::Spaces, to_byte_range(cu, uc_idx, new_uc_idx));
    new_uc_idx + 1
}

fn must_be_string(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
) -> UcContentIndex {
    let new_uc_idx = take_string(cu, uc_idx + 1);
    let (kind, new_uc_idx) = match new_uc_idx {
        Ok((new_uc_idx, content)) => (TokenKind::StringLiteral { content }, new_uc_idx),
        Err((new_uc_idx, msg)) => (TokenKind::Invalid { msg }, new_uc_idx),
    };
    tokens.push(kind, to_byte_range(cu, uc_idx, new_uc_idx));
    new_uc_idx + 1
}

fn must_be_name(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
    c: char,
) -> UcContentIndex {
    let new_uc_idx = if c == '_' {
        take_while(cu, uc_idx + 1, |s| {
            s.len() == 1 && s.chars().next().unwrap().is_ascii_alphanumeric()
        })
    } else {
        take_while(cu, uc_idx + 1, |s| {
            s.len() == 1 && s.chars().next().unwrap().is_ascii_alphanumeric() || s.starts_with('_')
        })
    };

    let byte_range = to_byte_range(cu, uc_idx, new_uc_idx);
    let s = byte_range.get_str(cu);
    if let Some(kind) = get_keyword(s) {
        tokens.push(kind, byte_range);
    } else {
        tokens.push(TokenKind::Identifier, byte_range);
    }
    new_uc_idx + 1
}

fn invalid_stuff(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
    c: char,
) -> UcContentIndex {
    tokens.push(
        TokenKind::Invalid {
            msg: format!("unsupported {}", c),
        },
        to_byte_range(cu, uc_idx, uc_idx),
    );
    uc_idx + 1
}

pub(crate) fn try_multi_byte_tokens(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcContentIndex,
    c: char,
) -> Option<UcContentIndex> {
    Some(match c {
        '0' => must_be_single_zero(cu, tokens, uc_idx),
        '1'..='9' => must_be_integer(cu, tokens, uc_idx),
        ' ' => must_be_spaces(cu, tokens, uc_idx),
        '"' => must_be_string(cu, tokens, uc_idx),
        'a'..='z' | 'A'..='Z' | '_' => must_be_name(cu, tokens, uc_idx, c),
        _ => invalid_stuff(cu, tokens, uc_idx, c),
    })
}
