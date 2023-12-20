use super::{
    ast, lexer, IntermediateResult, PackedToken, PackedTokenIndex, PackedTokens, ParseError,
    ParseResult, ParseResultExt,
};

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
enum Associativity {
    Left,
    Right,
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Clone, Copy)]
struct Precedence(i64);

impl Precedence {
    fn new(v: i64) -> Self {
        Self(v)
    }

    fn get(&self) -> i64 {
        self.0
    }
}

impl std::ops::Add<i64> for Precedence {
    type Output = Self;

    fn add(self, rhs: i64) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

#[derive(Debug)]
struct TokenTrait {
    precedence: Precedence,
    associativity: Associativity,
}

// either returns error or UnaryOp node
fn must_be_i64_after_sub_sign(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
) -> ParseResult {
    let sub_sign_packed_token_idx = packed_tokens
        .get(start_packed_token_idx - 1)
        .unwrap_or_else(|| panic!("BUG: expected a token before `-`, but got None"))
        .token_idx;

    if let Some(PackedToken {
        token_idx: operand_token_idx,
        token: operand_token,
    }) = packed_tokens.get(start_packed_token_idx)
    {
        match operand_token.get_kind() {
            lexer::TokenKind::I64 => ParseResult::new_node(
                ast::AstNode::Expression(ast::Expr::UnaryOp {
                    operator: sub_sign_packed_token_idx,
                    operand: ast.push(ast::AstNode::Expression(ast::Expr::I64(*operand_token_idx))),
                }),
                start_packed_token_idx + 1,
            ),
            _ => ParseResult::new_error(ParseError::UnexpectedToken {
                msg: "unary `-` only accepts integer".to_owned(),
                start_token_idx: sub_sign_packed_token_idx,
                inclusive_end_token_idx: *operand_token_idx,
            }),
        }
    } else {
        ParseResult::new_error(ParseError::UnexpectedEndOfInput {
            msg: "negative number expects digits after `-`".to_owned(),
            start_token_idx: sub_sign_packed_token_idx,
        })
    }
}

pub(super) fn parse_module(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    mut next_token_idx: PackedTokenIndex,
) -> Result<ast::AstNode, ParseError> {
    let mut module_node = ast::AstNode::new_module(50);
    while let IntermediateResult::Node {
        node: statement,
        next_packed_token_idx,
    } = parse_statement(ast, packed_tokens, next_token_idx)?
    {
        let statement_node_idx = ast.push(statement);
        module_node.add_statement_to_module(ast, statement_node_idx);
        next_token_idx = next_packed_token_idx;
    }

    Ok(module_node)
}

// either returns a definiton statement or an error, never returns `IntermediateResult::Finished`
//
// precondition: cannot be eof
fn try_parse_definition_statement(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
) -> ParseResult {
    // has something
    let packed_token = packed_tokens
        .get(start_packed_token_idx)
        .unwrap_or_else(|| panic!("BUG: cannot be EOF when parsing definition statement"));
    let origin_start_token_idx = packed_token.token_idx;

    // must be either `let` or `var`
    let kind = packed_token.token.get_kind();
    if kind != &lexer::TokenKind::KwLet && kind != &lexer::TokenKind::KwVar {
        return ParseResult::new_error(ParseError::UnexpectedToken {
            msg: "expression should start with `let` or `var`, but got".to_owned(),
            start_token_idx: packed_token.token_idx,
            inclusive_end_token_idx: packed_token.token_idx,
        });
    }
    let kw_token_idx = packed_token.token_idx;
    let after_kw_packed_token_idx = start_packed_token_idx + 1;

    // lhs is an expression.
    // TODO: must be something can be assigned to, like identifier, or `a[3]`
    let IntermediateResult::Node {
        node: lhs_expr,
        next_packed_token_idx: after_lhs_packed_token_idx,
    } = parse_expression(
        ast,
        packed_tokens,
        after_kw_packed_token_idx,
        Precedence::new(0),
    )?
    else {
        return ParseResult::new_error(ParseError::UnexpectedEndOfInput {
            msg: "there must be some kinds of expressions after `let` or `var`".to_owned(),
            start_token_idx: origin_start_token_idx,
        });
    };

    // must be `=`
    let Some(packed_token) = packed_tokens.get(after_lhs_packed_token_idx) else {
        return ParseResult::new_error(ParseError::UnexpectedEndOfInput {
            msg: "expected definition but could not find `=`".to_owned(),
            start_token_idx: origin_start_token_idx,
        });
    };
    if !matches!(packed_token.token.get_kind(), &lexer::TokenKind::Equal) {
        return ParseResult::new_error(ParseError::UnexpectedToken {
            msg: "definition expects `=`".to_owned(),
            start_token_idx: origin_start_token_idx,
            inclusive_end_token_idx: packed_token.token_idx,
        });
    }
    let eq_token_idx = packed_token.token_idx;
    let after_eq_packed_token_idx = after_lhs_packed_token_idx + 1;

    // rhs is an expression
    let IntermediateResult::Node {
        node: rhs_expr,
        next_packed_token_idx: after_rhs_packed_token_idx,
    } = parse_expression(
        ast,
        packed_tokens,
        after_eq_packed_token_idx,
        Precedence::new(0),
    )?
    else {
        return ParseResult::new_error(ParseError::UnexpectedEndOfInput {
            msg: format!(
                "expecte an expression after {}, but got eof",
                lexer::TokenKind::Equal
            ),
            start_token_idx: origin_start_token_idx,
        });
    };

    // ;
    if let Some(e) = must_be_semicolon_ends_statement(
        packed_tokens,
        origin_start_token_idx,
        after_rhs_packed_token_idx,
    ) {
        return ParseResult::new_error(e);
    }

    // return
    ParseResult::new_node(
        ast::AstNode::Statement(ast::Stat::Definition {
            kw: kw_token_idx,
            lhs_expression_node_idx: ast.push(lhs_expr),
            eq: eq_token_idx,
            rhs_expression_node_idx: ast.push(rhs_expr),
        }),
        after_rhs_packed_token_idx + 1,
    )
}

// either returns an expression or an error, never returns `IntermediateResult::Finished`
// TODO: is only expression statement that makes sense a function call?
fn try_parse_expression_statement(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    next_packed_token_idx: PackedTokenIndex,
) -> ParseResult {
    let packed_token = packed_tokens
        .get(next_packed_token_idx)
        .unwrap_or_else(|| panic!("BUG: cannot be EOF when parsing definition statement"));
    let origin_start_token_idx = packed_token.token_idx;

    let IntermediateResult::Node {
        node,
        next_packed_token_idx,
    } = parse_expression(
        ast,
        packed_tokens,
        next_packed_token_idx,
        Precedence::new(0),
    )?
    else {
        panic!("BUG: parse_expression should always return a node, empty statements must be filtered out before this")
    };

    // ;
    if let Some(e) = must_be_semicolon_ends_statement(
        packed_tokens,
        origin_start_token_idx,
        next_packed_token_idx,
    ) {
        return ParseResult::new_error(e);
    }

    ParseResult::new_node(
        ast::AstNode::Statement(ast::Stat::Expression(ast.push(node))),
        next_packed_token_idx,
    )
}

fn skip_empty_statements(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    mut start_packed_token_idx: PackedTokenIndex,
) -> PackedTokenIndex {
    while let Some(packed_token) = packed_tokens.get(start_packed_token_idx) {
        if packed_token.token.get_kind() == &lexer::TokenKind::SemiColon {
            start_packed_token_idx += 1;
        } else {
            break;
        }
    }
    start_packed_token_idx
}

fn must_be_semicolon_ends_statement(
    packed_tokens: &PackedTokens,
    origin_start_token_idx: lexer::TokenIndex,
    next_packed_token_idx: PackedTokenIndex,
) -> Option<ParseError> {
    let Some(packed_token) = packed_tokens.get(next_packed_token_idx) else {
        return Some(ParseError::UnexpectedEndOfInput {
            msg: format!("statement must end with `{}`", lexer::TokenKind::SemiColon),
            start_token_idx: origin_start_token_idx,
        });
    };

    if packed_token.token.get_kind() == &lexer::TokenKind::SemiColon {
        None
    } else {
        Some(ParseError::UnexpectedToken {
            msg: format!("statement must end with `{}`", lexer::TokenKind::SemiColon),
            start_token_idx: origin_start_token_idx,
            inclusive_end_token_idx: packed_token.token_idx,
        })
    }
}

fn parse_statement(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
) -> ParseResult {
    let mut errors = Vec::with_capacity(5);

    let start_packed_token_idx = skip_empty_statements(ast, packed_tokens, start_packed_token_idx);
    if packed_tokens.get(start_packed_token_idx).is_none() {
        return ParseResult::new_finished();
    }

    match try_parse_definition_statement(ast, packed_tokens, start_packed_token_idx) {
        Err(e) => errors.push(e),
        a => return a,
    }

    match try_parse_expression_statement(ast, packed_tokens, start_packed_token_idx) {
        Err(e) => errors.push(e),
        a => return a,
    }

    assert!(!errors.is_empty());
    if errors.len() > 1 {
        for e in errors.iter() {
            dbg!(format!("{}, {:?}", e.get_string(ast), e));
        }
    }
    Err(errors[0].clone())
}

fn get_precedence(token: &lexer::Token) -> Option<TokenTrait> {
    match token.get_kind() {
        lexer::TokenKind::Plus => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Dash => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Star => Some(TokenTrait {
            precedence: Precedence::new(2),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Slash => Some(TokenTrait {
            precedence: Precedence::new(2),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Percentage => Some(TokenTrait {
            precedence: Precedence::new(2),
            associativity: Associativity::Left,
        }),
        _ => None,
    }
}

fn is_end_of_expression(token: &lexer::Token) -> bool {
    [
        lexer::TokenKind::Equal,
        lexer::TokenKind::SemiColon,
        lexer::TokenKind::RParen,
    ]
    .contains(token.get_kind())
}

fn can_shift_with_op(
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
    min_precedence: Precedence,
) -> Option<(&PackedToken, Precedence)> {
    let Some(op_packed_token) = packed_tokens.get(start_packed_token_idx) else {
        return None;
    };
    if is_end_of_expression(&op_packed_token.token) {
        return None;
    }

    let TokenTrait {
        precedence,
        associativity,
    } = get_precedence(&op_packed_token.token)?;
    if precedence < min_precedence {
        return None;
    }

    Some((
        op_packed_token,
        if associativity == Associativity::Left {
            precedence + 1
        } else {
            precedence
        },
    ))
}

/// parse_expression parses an expression.
///
/// # Note
///
/// - returns `IntermediateResult::Finished` if eof
/// - otherwise must return an expression or an error
fn parse_expression(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
    min_precedence: Precedence,
) -> ParseResult {
    // nothing
    if packed_tokens.get(start_packed_token_idx).is_none() {
        return ParseResult::new_finished();
    };

    // must be something or return error
    let IntermediateResult::Node {
        node: mut lhs,
        next_packed_token_idx: after_lhs_packed_token_idx,
    } = parse_primary(ast, packed_tokens, start_packed_token_idx)?
    else {
        panic!("BUG: parse_primary should always return a node")
    };

    let mut next_packed_token_idx = after_lhs_packed_token_idx;
    // if there is something, try to see if it is binary operation
    //     if it is, should we shift or reduce?
    while let Some((op_packed_token, min_precedence)) =
        can_shift_with_op(packed_tokens, next_packed_token_idx, min_precedence)
    {
        // if it is binary op, then there must be an expression after op sign
        let IntermediateResult::Node {
            node: rhs,
            next_packed_token_idx: after_rhs_packed_token_idx,
        } = parse_expression(
            ast,
            packed_tokens,
            next_packed_token_idx + 1,
            min_precedence,
        )?
        else {
            let op_packed_token = packed_tokens
                .get(next_packed_token_idx - 1)
                .unwrap_or_else(|| panic!("BUG: there must be a binary op"));

            return ParseResult::new_error(ParseError::UnexpectedEndOfInput {
                msg: format!("nothing after {}", op_packed_token.token.get_kind()),
                start_token_idx: op_packed_token.token_idx,
            });
        };

        let lhs_node_idx = ast.push(lhs);
        let rhs_node_idx = ast.push(rhs);
        lhs = ast::AstNode::Expression(ast::Expr::BinaryOp {
            operator: op_packed_token.token_idx,
            lhs: lhs_node_idx,
            rhs: rhs_node_idx,
        });
        next_packed_token_idx = after_rhs_packed_token_idx;
    }

    ParseResult::new_node(lhs, next_packed_token_idx)
}

/// parse_primary parses a primary expression.
///
/// A primary expression is one of the following:
/// - an (negative) integer literal
/// - an identifier
/// - a string literal
/// - a parenthesized expression
///
/// # Note
/// it never returns `IntermediateResult::Finished`
fn parse_primary(
    ast: &mut ast::Ast,
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
) -> ParseResult {
    let packed_token = packed_tokens
        .get(start_packed_token_idx)
        .unwrap_or_else(|| panic!("BUG: cannot be EOF when parsing primary"));
    match packed_token.token.get_kind() {
        lexer::TokenKind::I64 => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::I64(packed_token.token_idx)),
            start_packed_token_idx + 1,
        ),
        lexer::TokenKind::Identifier => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::Identifier(packed_token.token_idx)),
            start_packed_token_idx + 1,
        ),
        // don't allow chained unary operators
        lexer::TokenKind::Dash => {
            must_be_i64_after_sub_sign(ast, packed_tokens, start_packed_token_idx + 1)
        }
        lexer::TokenKind::LParen => {
            must_be_paren_expression(ast, packed_tokens, start_packed_token_idx + 1)
        }
        lexer::TokenKind::StringLiteral { content } => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::StringLiteral {
                token_idx: packed_token.token_idx,
                content: content.clone(),
            }),
            start_packed_token_idx + 1,
        ),
        _ => ParseResult::new_error(ParseError::UnexpectedToken {
            msg: "expected an expression, but got".to_owned(),
            start_token_idx: packed_token.token_idx,
            inclusive_end_token_idx: packed_token.token_idx,
        }),
    }
}

// either returns an expresion or an error
fn must_be_paren_expression(
    ast: &mut ast::Ast<'_>,
    packed_tokens: &PackedTokens,
    start_packed_token_idx: PackedTokenIndex,
) -> ParseResult {
    let lparen_packed_token_idx = packed_tokens
        .get(start_packed_token_idx - 1)
        .unwrap_or_else(|| panic!("BUG: expected a lparen, but got None"))
        .token_idx;

    let IntermediateResult::Node {
        node: expr,
        next_packed_token_idx: after_expr_packed_token_idx,
    } = parse_expression(
        ast,
        packed_tokens,
        start_packed_token_idx,
        Precedence::new(0),
    )?
    else {
        return ParseResult::new_error(ParseError::UnexpectedEndOfInput {
            msg: format!("nothing after {}", lexer::TokenKind::LParen),
            start_token_idx: lparen_packed_token_idx,
        });
    };

    match packed_tokens.get(after_expr_packed_token_idx) {
        Some(token) if token.token.get_kind() == &lexer::TokenKind::RParen => {
            ParseResult::new_node(expr, after_expr_packed_token_idx + 1)
        }
        _ => ParseResult::new_error(ParseError::MismatchedParentheses {
            lparen: lparen_packed_token_idx,
        }),
    }
}

pub(super) fn partition_packed_tokens_and_lex_errors(
    tokens: lexer::Tokens,
) -> (PackedTokens, Vec<(lexer::TokenIndex, String)>) {
    let (tokens, invalid_tokens) = tokens
        .into_iter()
        .enumerate()
        .filter(|(_, token)| {
            token.get_kind() != &lexer::TokenKind::Spaces
                && token.get_kind() != &lexer::TokenKind::NewLine
        })
        .map(|(i, token)| (lexer::TokenIndex::new(i), token))
        .partition(|(_, token)| !matches!(token.get_kind(), lexer::TokenKind::Invalid { .. }));

    (
        tokens,
        invalid_tokens
            .into_iter()
            .map(|PackedToken { token_idx, token }| match token.get_kind() {
                lexer::TokenKind::Invalid { msg } => (token_idx, msg.clone()),
                _ => panic!("BUG: expected invalid token, got {:?}", token),
            })
            .collect(),
    )
}
