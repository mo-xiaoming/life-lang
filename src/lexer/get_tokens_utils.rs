use super::{indices::UcIdx, indices::UcSpan, CompilationUnit, TokenIdx, TokenKind, Tokens};

pub(super) trait StringLike {
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

fn find_string_end(cu: &CompilationUnit, mut start: UcIdx) -> Option<UcIdx> {
    let mut possible_new_line = None;
    while let Some(s) = cu.get_str(start) {
        if s == "\n" {
            possible_new_line = Some(start);
        } else if s == r#"""# && !matches!(cu.get_str(start - 1), Some("\\")) {
            return Some(start);
        }
        start += 1;
    }
    possible_new_line
}

fn take_unicode(
    cu: &CompilationUnit,
    after_rquote_uc_idx: UcIdx,
    lbrace_uc_idx: UcIdx,
) -> Result<(UcIdx, String), TakeStringError> {
    // start with {
    if cu.get_str(lbrace_uc_idx).map_or(true, |s| s != "{") {
        return Err(TakeStringError {
            error_uc_idx: lbrace_uc_idx,
            msg: "unicode should be in the format of \\u{...}".to_owned(),
            next_uc_idx: after_rquote_uc_idx,
        });
    }
    let mut hex_num_uc_idx = lbrace_uc_idx + 1;

    // take hex numbers
    while let Some(s) = cu.get_str(hex_num_uc_idx) {
        if s == "}" {
            break;
        }
        if s.len() != 1 || !s.chars().next().unwrap().is_ascii_hexdigit() {
            return Err(TakeStringError {
                error_uc_idx: hex_num_uc_idx,
                msg: format!(
                    "only hex numbers are allowed in unicode sequence, `{}` is not allowed",
                    s
                ),
                next_uc_idx: after_rquote_uc_idx,
            });
        }
        hex_num_uc_idx += 1;
    }
    let rbrace_uc_idx = hex_num_uc_idx;

    // empty {}?
    if rbrace_uc_idx == lbrace_uc_idx + 1 {
        return Err(TakeStringError {
            error_uc_idx: rbrace_uc_idx,
            msg: "unicode should be in the format of \\u{...}, cannot be empty between `{}`"
                .to_owned(),
            next_uc_idx: after_rquote_uc_idx,
        });
    }

    // convert hex to char
    let mut s = cu
        .get_str(UcSpan::new(lbrace_uc_idx + 1, rbrace_uc_idx - 1))
        .unwrap()
        .to_owned();
    if s.len() % 2 != 0 {
        s = format!("0{}", s);
    }
    let unicode_err_fn = || TakeStringError {
        error_uc_idx: lbrace_uc_idx + 1,
        msg: format!("`{}` is not a valid unicode code point", s),
        next_uc_idx: after_rquote_uc_idx,
    };
    let n = u32::from_str_radix(&s, 16).map_err(|_| unicode_err_fn())?;
    let c = char::from_u32(n).ok_or_else(unicode_err_fn)?;

    Ok((rbrace_uc_idx, c.to_string()))
}

struct TakeStringError {
    error_uc_idx: UcIdx,
    msg: String,
    next_uc_idx: UcIdx,
}

fn take_string(cu: &CompilationUnit, mut start: UcIdx) -> Result<(UcIdx, String), TakeStringError> {
    let lquote_uc_idx = start - 1;
    let unterminated_err_fn = || TakeStringError {
        error_uc_idx: lquote_uc_idx,
        msg: "unterminated string literal".to_owned(),
        next_uc_idx: UcIdx::new(cu.ucs.len()),
    };
    let Some(rquote_uc_idx) = find_string_end(cu, start) else {
        return Err(unterminated_err_fn());
    };
    let after_rquote_uc_idx = rquote_uc_idx + 1;

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
                let (new_start, chunk) = take_unicode(cu, after_rquote_uc_idx, start + 1)?;
                start = new_start + 1;
                content.push_str(&chunk);
            } else if escaped_chars.contains_key(&s) {
                start += 1;
                content.push_str(escaped_chars[&s]);
            } else {
                return Err(TakeStringError {
                    error_uc_idx: start,
                    msg: format!("invalid escape char `{}`", s),
                    next_uc_idx: after_rquote_uc_idx,
                });
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

    Err(unterminated_err_fn())
}

fn get_single_char_token_kind(c: char) -> Option<TokenKind> {
    match c {
        '+' => Some(TokenKind::Plus),
        '-' => Some(TokenKind::Minus),
        '*' => Some(TokenKind::Star),
        '/' => Some(TokenKind::Slash),
        '%' => Some(TokenKind::Percent),
        '(' => Some(TokenKind::LParen),
        ')' => Some(TokenKind::RParen),
        ';' => Some(TokenKind::SemiColon),
        '{' => Some(TokenKind::LCurlyBrace),
        '}' => Some(TokenKind::RCurlyBrace),
        ':' => Some(TokenKind::Colon),
        _ => None,
    }
}

fn get_keyword(s: &str) -> Option<TokenKind> {
    match s {
        "let" => Some(TokenKind::KwLet),
        "var" => Some(TokenKind::KwVar),
        "if" => Some(TokenKind::KwIf),
        "else" => Some(TokenKind::KwElse),
        "return" => Some(TokenKind::KwReturn),
        _ => None,
    }
}

pub(crate) fn try_new_line(
    _cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    if !c.is_new_line() {
        return None;
    }

    tokens.push(TokenKind::NewLine, uc_idx, uc_idx);
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
        tokens.push(TokenKind::FakeTokenForInvalid, uc_idx, new_uc_idx);
        tokens.push(
            TokenKind::Invalid {
                msg: format!(
                    "multi-char unicode like `{}` only supported in strings and comments",
                    s
                ),
                error_fake_token_idx: TokenIdx::new(tokens.len() - 1),
            },
            uc_idx,
            new_uc_idx,
        );
        Some(new_uc_idx + 1)
    } else {
        None
    }
}

pub(crate) fn try_single_byte_token(
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
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    if c != '"' {
        return None;
    }

    let new_uc_idx = take_string(cu, uc_idx + 1);
    match new_uc_idx {
        Ok((new_uc_idx, content)) => {
            tokens.push(TokenKind::StringLiteral { content }, uc_idx, new_uc_idx);
            Some(new_uc_idx + 1)
        }
        Err(TakeStringError {
            error_uc_idx,
            msg,
            next_uc_idx,
        }) => {
            tokens.push(TokenKind::FakeTokenForInvalid, error_uc_idx, error_uc_idx);
            tokens.push(
                TokenKind::Invalid {
                    msg,
                    error_fake_token_idx: TokenIdx::new(tokens.len() - 1),
                },
                uc_idx,
                error_uc_idx,
            );
            Some(next_uc_idx)
        }
    }
}

pub(crate) fn try_multi_byte_tokens(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> Option<UcIdx> {
    match c {
        '0' => Some(must_be_single_zero(cu, tokens, uc_idx)),
        '1'..='9' => Some(must_be_integer(cu, tokens, uc_idx)),
        ' ' => Some(must_be_spaces(cu, tokens, uc_idx)),
        '#' => Some(must_be_comment(cu, tokens, uc_idx)),
        'a'..='z' | 'A'..='Z' | '_' => Some(must_be_name(cu, tokens, uc_idx, c)),
        '>' => Some(must_be_gt_or_gteq(cu, tokens, uc_idx)),
        '<' => Some(must_be_lt_or_lteq(cu, tokens, uc_idx)),
        '=' => Some(must_be_eq_or_assign(cu, tokens, uc_idx)),
        '!' => Some(must_be_not_eq_or_bitwise_not(cu, tokens, uc_idx)),
        _ => None,
    }
}

fn must_be_not_eq_or_bitwise_not(
    cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
) -> UcIdx {
    let (new_uc_idx, kind) = if cu.get_str(uc_idx + 1) == Some("=") {
        (uc_idx + 1, TokenKind::Ne)
    } else {
        (uc_idx, TokenKind::Not)
    };
    tokens.push(kind, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_gt_or_gteq(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let (new_uc_idx, kind) = if cu.get_str(uc_idx + 1) == Some("=") {
        (uc_idx + 1, TokenKind::Ge)
    } else {
        (uc_idx, TokenKind::Gt)
    };
    tokens.push(kind, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_lt_or_lteq(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let (new_uc_idx, kind) = if cu.get_str(uc_idx + 1) == Some("=") {
        (uc_idx + 1, TokenKind::Le)
    } else {
        (uc_idx, TokenKind::Lt)
    };
    tokens.push(kind, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_eq_or_assign(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let (new_uc_idx, kind) = if cu.get_str(uc_idx + 1) == Some("=") {
        (uc_idx + 1, TokenKind::EqEq)
    } else {
        (uc_idx, TokenKind::Eq)
    };
    tokens.push(kind, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

pub(crate) fn must_be_comment(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let new_uc_idx = take_while(cu, uc_idx + 1, |s| !s.is_new_line());
    tokens.push(TokenKind::Comment, uc_idx, new_uc_idx);
    new_uc_idx + 1
}

fn must_be_single_zero(cu: &CompilationUnit, tokens: &mut Tokens, uc_idx: UcIdx) -> UcIdx {
    let new_uc_idx = take_while(cu, uc_idx + 1, uc_is_ascii_digit);
    let kind = if new_uc_idx == uc_idx {
        TokenKind::I64
    } else {
        tokens.push(TokenKind::FakeTokenForInvalid, uc_idx, new_uc_idx);
        TokenKind::Invalid {
            msg: "leading zero is not allowed".to_owned(),
            error_fake_token_idx: TokenIdx::new(tokens.len() - 1),
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
    tokens.push(
        TokenKind::Spaces {
            count: (new_uc_idx - uc_idx).width(),
        },
        uc_idx,
        new_uc_idx,
    );
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
        tokens.push(
            TokenKind::Identifier { name: s.to_owned() },
            uc_idx,
            new_uc_idx,
        );
    }
    new_uc_idx + 1
}

pub(super) fn must_be_invalid_stuff(
    _cu: &CompilationUnit,
    tokens: &mut Tokens,
    uc_idx: UcIdx,
    c: char,
) -> UcIdx {
    tokens.push(TokenKind::FakeTokenForInvalid, uc_idx, uc_idx);
    tokens.push(
        TokenKind::Invalid {
            msg: format!("unsupported `{}`", c),
            error_fake_token_idx: TokenIdx::new(tokens.len() - 1),
        },
        uc_idx,
        uc_idx,
    );
    uc_idx + 1
}
