use super::{
    ast, lexer,
    lexer::TokenKindRepr,
    lexer::{Token, TokenIdx, Tokens},
    HappyPath, ParseError, ParseResult, ParseResultExt, SingleParseError,
};

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
enum Associativity {
    Left,
    _Right,
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

fn find_recovery_idx(tokens: &Tokens, mut next_token_idx: TokenIdx) -> TokenIdx {
    let Some((token_idx, token)) = tokens.find_next_non_blank_token(next_token_idx) else {
        return next_token_idx;
    };
    next_token_idx = token_idx;

    fn find_next_block_end(tokens: &Tokens, mut next_token_idx: TokenIdx) -> Option<TokenIdx> {
        let mut block_level = 0;
        while let Some((token_idx, token)) = tokens.find_next_non_blank_token(next_token_idx) {
            match token.get_kind() {
                lexer::TokenKind::LCurlyBrace => block_level += 1,
                lexer::TokenKind::RCurlyBrace => {
                    block_level -= 1;
                    if block_level == 0 {
                        return Some(token_idx);
                    }
                }
                _ => {}
            }
            next_token_idx = token_idx + 1;
        }
        None
    }

    fn find_next_stat_end(tokens: &Tokens, mut next_token_idx: TokenIdx) -> Option<TokenIdx> {
        let mut block_level = 0;
        while let Some((token_idx, token)) = tokens.find_next_non_blank_token(next_token_idx) {
            match token.get_kind() {
                lexer::TokenKind::LCurlyBrace => block_level += 1,
                lexer::TokenKind::RCurlyBrace => {
                    block_level -= 1;
                    if block_level == 0 {
                        return Some(token_idx);
                    }
                }
                lexer::TokenKind::SemiColon if block_level == 0 => return Some(token_idx),
                _ => {}
            }
            next_token_idx = token_idx + 1;
        }
        None
    }

    match token.get_kind() {
        lexer::TokenKind::KwIf | lexer::TokenKind::LCurlyBrace => {
            let Some(rcurlybrace_token_idx) = find_next_block_end(tokens, next_token_idx) else {
                return tokens.invalid_token_idx();
            };
            match tokens.find_next_non_blank_token(rcurlybrace_token_idx + 1) {
                Some((else_kw_token_idx, else_kw_token))
                    if else_kw_token.get_kind() == &lexer::TokenKind::KwElse =>
                {
                    find_recovery_idx(tokens, else_kw_token_idx)
                }
                Some(_) => find_recovery_idx(tokens, rcurlybrace_token_idx + 1),
                None => tokens.invalid_token_idx(),
            }
        }
        _ => match find_next_stat_end(tokens, next_token_idx) {
            Some(token_idx) => token_idx + 1,
            None => tokens.invalid_token_idx(),
        },
    }
}

// either returns error or UnaryOp node
fn must_be_i64_after_dash_sign(
    ast: &mut ast::Ast<ParseError>,
    next_token_idx: TokenIdx,
    dash_token_idx: TokenIdx,
) -> ParseResult {
    let Some((num_token_idx, num_token)) =
        ast.get_tokens().find_next_non_blank_token(next_token_idx)
    else {
        return ParseResult::new_error_unexpected_eof(
            format!(
                "expected a number after `{}`",
                lexer::TokenKind::Minus.get_string_repr()
            ),
            dash_token_idx,
        );
    };

    match num_token.get_kind() {
        lexer::TokenKind::I64 => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::Negation {
                operator: dash_token_idx,
                operand: ast.push_node(ast::AstNode::Expression(ast::Expr::I64(num_token_idx))),
            }),
            num_token_idx + 1,
        ),
        lexer::TokenKind::Minus => ParseResult::new_error_unexpected_token(
            format!(
                "`{}` cannot be chained",
                lexer::TokenKind::Minus.get_string_repr(),
            ),
            dash_token_idx,
            num_token_idx,
        ),
        _ => ParseResult::new_error_unexpected_token(
            format!(
                "expected a number after `{}`",
                lexer::TokenKind::Minus.get_string_repr()
            ),
            dash_token_idx,
            num_token_idx,
        ),
    }
}

pub(super) fn parse_module(ast: &mut ast::Ast<ParseError>, mut next_token_idx: TokenIdx) {
    let mut module_node = ast::AstNode::new_module_with_capacity(50);
    let mut error = ParseError::no_error();
    loop {
        match parse_statement(ast, next_token_idx) {
            Ok(HappyPath::Node {
                node: statement,
                next_token_idx: token_idx,
            }) => {
                let statement_node_idx = ast.push_node(statement);
                module_node.add_statement_to_module(ast, statement_node_idx);
                next_token_idx = token_idx;
            }
            Ok(HappyPath::Finished) => {
                break;
            }
            Err(e) => {
                error = error.add_new_error(e);
                next_token_idx = find_recovery_idx(ast.get_tokens(), next_token_idx);
            }
        }
    }

    ast.set_error(error);
    ast.set_module(module_node);
}

fn parse_type_from_colon(ast: &mut ast::Ast<ParseError>, colon_token_idx: TokenIdx) -> ParseResult {
    let no_type_err_fn = || "expected a type expression";

    let Some((type_start_token_idx, type_start_token)) = ast
        .get_tokens()
        .find_next_non_blank_token(colon_token_idx + 1)
    else {
        return ParseResult::new_error_unexpected_eof(no_type_err_fn(), colon_token_idx + 1);
    };

    match type_start_token.get_kind() {
        lexer::TokenKind::Identifier { .. } => ParseResult::new_node(
            ast::AstNode::Annotation(ast::Anno::Type {
                token_idx: type_start_token_idx,
            }),
            type_start_token_idx + 1,
        ),
        _ => ParseResult::new_error_unexpected_token(
            no_type_err_fn(),
            colon_token_idx,
            type_start_token_idx,
        ),
    }
}

// either returns a definiton statement or an error, never returns `IntermediateResult::Finished`
//
// first token is `let` or `var`
fn parse_definition_statement(
    ast: &mut ast::Ast<ParseError>,
    kw_token_idx: TokenIdx,
    kw_token: &Token,
) -> ParseResult {
    let kw_kind = kw_token.get_kind();

    // lhs is an expression.
    // TODO: must be something can be assigned to, like identifier, or `a[3]`
    let no_lhs_expr_err_fn = || {
        format!(
            "expect an expression after `{}` for a definition",
            kw_kind.get_string_repr()
        )
    };
    let HappyPath::Node {
        node: lhs_expr,
        next_token_idx: after_lhs_token_idx,
    } = parse_expression(ast, kw_token_idx + 1, Precedence::new(0))
        .map_err(|e| e.add_error_context(no_lhs_expr_err_fn()))?
    else {
        return ParseResult::new_error_unexpected_eof(no_lhs_expr_err_fn(), kw_token_idx);
    };

    // is there an type annotation?
    let mut colon_token_idx = None;
    let mut type_token_idx = None;
    let mut next_token_idx_before_eq = after_lhs_token_idx;
    if let Some((colon_token_idx_, colon_token)) = ast
        .get_tokens()
        .find_next_non_blank_token(after_lhs_token_idx)
    {
        if colon_token.get_kind() == &lexer::TokenKind::Colon {
            let no_type_error_fn = || {
                format!(
                    "expected a type expression after `{}`",
                    lexer::TokenKind::Colon.get_string_repr()
                )
            };
            let HappyPath::Node {
                node: type_expr,
                next_token_idx: after_type_token_idx,
            } = parse_type_from_colon(ast, colon_token_idx_)
                .map_err(|e| e.add_error_context(no_type_error_fn()))?
            else {
                return ParseResult::new_error_unexpected_eof(no_type_error_fn(), colon_token_idx_);
            };
            colon_token_idx = Some(colon_token_idx_);
            type_token_idx = Some(ast.push_node(type_expr));
            next_token_idx_before_eq = after_type_token_idx;
        }
    }

    // must be `=`
    let eq_token_idx = must_find(
        ast.get_tokens(),
        kw_token_idx,
        next_token_idx_before_eq,
        || {
            format!(
                "expected definition format `{kw} ... {eq} ...`, but could not find `{eq}`",
                kw = kw_kind.get_string_repr(),
                eq = lexer::TokenKind::Eq.get_string_repr()
            )
        },
        |token_kind| token_kind == &lexer::TokenKind::Eq,
    )?;

    // rhs is an expression
    let no_rhs_expr_err_fn = || {
        format!(
            "expect an expression after `{}` for a definition",
            lexer::TokenKind::Eq.get_string_repr()
        )
    };
    let HappyPath::Node {
        node: rhs_expr,
        next_token_idx: after_rhs_token_idx,
    } = parse_expression(ast, eq_token_idx + 1, Precedence::new(0))
        .map_err(|e| e.add_error_context(no_rhs_expr_err_fn()))?
    else {
        return ParseResult::new_error_unexpected_eof(no_rhs_expr_err_fn(), kw_token_idx);
    };

    // ;
    let semicolon_token_idx = must_find(
        ast.get_tokens(),
        kw_token_idx,
        after_rhs_token_idx,
        || {
            format!(
                "statement must end with `{}`",
                lexer::TokenKind::SemiColon.get_string_repr()
            )
        },
        |token_kind| token_kind == &lexer::TokenKind::SemiColon,
    )?;
    ParseResult::new_node(
        ast::AstNode::Statement(ast::Stat::Definition {
            kw: kw_token_idx,
            lhs_expression_node_idx: ast.push_node(lhs_expr),
            colon: colon_token_idx,
            type_node_idx: type_token_idx,
            eq: eq_token_idx,
            rhs_expression_node_idx: ast.push_node(rhs_expr),
        }),
        semicolon_token_idx + 1,
    )
}

// either returns an expression or an error, never returns `IntermediateResult::Finished`
// precondition: no eof
// TODO: is only expression statement that makes sense a function call?
fn try_parse_expression_statement(
    ast: &mut ast::Ast<ParseError>,
    next_token_idx: TokenIdx,
) -> ParseResult {
    let HappyPath::Node {
        node,
        next_token_idx: after_expr_token_idx,
    } = parse_expression(ast, next_token_idx, Precedence::new(0))
        .map_err(|e| e.add_error_context("this must be an expression"))?
    else {
        panic!("BUG: parse_expression should always return a node, empty statements must be filtered out before this")
    };

    // ;
    let semicolon_token_idx = must_find(
        ast.get_tokens(),
        next_token_idx,
        after_expr_token_idx,
        || {
            format!(
                "statement must end with `{}`",
                lexer::TokenKind::SemiColon.get_string_repr()
            )
        },
        |token_kind| token_kind == &lexer::TokenKind::SemiColon,
    )?;
    ParseResult::new_node(
        ast::AstNode::Statement(ast::Stat::Expression(ast.push_node(node))),
        semicolon_token_idx + 1,
    )
}

fn find_start_of_non_empty_statement<'cu>(
    ast: &'cu ast::Ast<ParseError>,
    mut next_token_idx: TokenIdx,
) -> Option<(TokenIdx, &'cu Token)> {
    while let Some((token_idx, token)) = ast.get_tokens().find_next_non_blank_token(next_token_idx)
    {
        match token.get_kind() {
            &lexer::TokenKind::SemiColon | &lexer::TokenKind::Comment { .. } => {
                next_token_idx = token_idx + 1;
            }
            _ => return Some((token_idx, token)),
        }
    }
    None
}

fn must_find(
    tokens: &Tokens,
    ctx_start_token_idx: TokenIdx,
    next_token_idx: TokenIdx,
    err_msg_fn: impl FnOnce() -> String,
    match_token_kind_fn: impl FnOnce(&lexer::TokenKind) -> bool,
) -> Result<TokenIdx, SingleParseError> {
    let Some((token_idx, token)) = tokens.find_next_non_blank_token(next_token_idx) else {
        return Err(SingleParseError::UnexpectedEof {
            msg: err_msg_fn(),
            ctx_token_idx: ctx_start_token_idx,
        });
    };

    if match_token_kind_fn(token.get_kind()) {
        Ok(token_idx)
    } else {
        Err(SingleParseError::UnexpectedToken {
            msg: err_msg_fn(),
            ctx_start_token_idx,
            error_token_idx: token_idx,
        })
    }
}

fn parse_statement(ast: &mut ast::Ast<ParseError>, next_token_idx: TokenIdx) -> ParseResult {
    // filter out spaces and comments
    let Some((next_token_idx, next_token)) = find_start_of_non_empty_statement(ast, next_token_idx)
    else {
        return ParseResult::new_finished();
    };

    match next_token.get_kind() {
        &lexer::TokenKind::KwLet | &lexer::TokenKind::KwVar => {
            parse_definition_statement(ast, next_token_idx, &next_token.clone())
        }
        _ => try_parse_expression_statement(ast, next_token_idx),
    }
}

fn get_precedence(token: &lexer::Token) -> Option<TokenTrait> {
    match token.get_kind() {
        lexer::TokenKind::EqEq => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Ne => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Lt => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Le => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Gt => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Ge => Some(TokenTrait {
            precedence: Precedence::new(1),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Plus => Some(TokenTrait {
            precedence: Precedence::new(5),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Minus => Some(TokenTrait {
            precedence: Precedence::new(5),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Star => Some(TokenTrait {
            precedence: Precedence::new(6),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Slash => Some(TokenTrait {
            precedence: Precedence::new(6),
            associativity: Associativity::Left,
        }),
        lexer::TokenKind::Percent => Some(TokenTrait {
            precedence: Precedence::new(6),
            associativity: Associativity::Left,
        }),
        _ => None,
    }
}

fn is_end_of_expression(token: &lexer::Token) -> bool {
    [
        lexer::TokenKind::Eq,
        lexer::TokenKind::SemiColon,
        lexer::TokenKind::RParen,
    ]
    .contains(token.get_kind())
}

fn can_shift_with_op(
    tokens: &Tokens,
    next_token_idx: TokenIdx,
    min_precedence: Precedence,
) -> Option<(TokenIdx, &Token, Precedence)> {
    let Some((op_token_idx, op_token)) = tokens.find_next_non_blank_token(next_token_idx) else {
        return None;
    };
    if is_end_of_expression(op_token) {
        return None;
    }

    let TokenTrait {
        precedence,
        associativity,
    } = get_precedence(op_token)?;
    if precedence < min_precedence {
        return None;
    }

    Some((
        op_token_idx,
        op_token,
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
    ast: &mut ast::Ast<ParseError>,
    next_token_idx: TokenIdx,
    min_precedence: Precedence,
) -> ParseResult {
    // nothing
    let Some((expr_start_token_idx, expr_start_token)) =
        ast.get_tokens().find_next_non_blank_token(next_token_idx)
    else {
        return ParseResult::new_finished();
    };

    // try if .. else ..
    if expr_start_token.get_kind() == &lexer::TokenKind::KwIf {
        return parse_if_expression(ast, expr_start_token_idx);
    }

    // try return
    if expr_start_token.get_kind() == &lexer::TokenKind::KwReturn {
        return parse_return_expression(ast, expr_start_token_idx);
    }

    // must be something or return error
    let HappyPath::Node {
        node: mut lhs,
        next_token_idx: after_lhs_token_idx,
    } = parse_primary(ast, expr_start_token_idx, &expr_start_token.clone())?
    // following doens't seem to be a very useful error context
    //.map_err(|e| e.add_error_context("an expression must start with an expression"))?
    else {
        panic!("BUG: parse_primary should always return a node")
    };

    let mut next_token_idx = after_lhs_token_idx;
    // if there is something, try to see if it is binary operation
    //     if it is, should we shift or reduce?
    while let Some((op_token_idx, op_token, min_precedence)) =
        can_shift_with_op(ast.get_tokens(), next_token_idx, min_precedence)
    {
        let op_token = op_token.clone();
        // if it is binary op, then there must be an expression after op sign
        let no_rhs_expr_err_fn = || {
            format!(
                "operator `{}` must be followed by an expression",
                op_token.get_kind().get_string_repr()
            )
        };
        let HappyPath::Node {
            node: rhs,
            next_token_idx: after_rhs_token_idx,
        } = parse_expression(ast, op_token_idx + 1, min_precedence)
            .map_err(|e| e.add_error_context(no_rhs_expr_err_fn()))?
        else {
            return ParseResult::new_error_unexpected_eof(no_rhs_expr_err_fn(), op_token_idx);
        };

        let lhs_node_idx = ast.push_node(lhs);
        let rhs_node_idx = ast.push_node(rhs);
        lhs = ast::AstNode::Expression(ast::Expr::ArithmeticOrLogical {
            operator: op_token_idx,
            lhs: lhs_node_idx,
            rhs: rhs_node_idx,
        });
        next_token_idx = after_rhs_token_idx;
    }

    ParseResult::new_node(lhs, next_token_idx)
}

fn parse_return_expression(
    ast: &mut ast::Ast<ParseError>,
    return_kw_token_idx: TokenIdx,
) -> ParseResult {
    let no_return_expr_err_fn = || {
        format!(
            "expected an expression after `{}`",
            lexer::TokenKind::KwReturn.get_string_repr()
        )
    };
    let HappyPath::Node {
        node: expr,
        next_token_idx: after_expr_token_idx,
    } = parse_expression(ast, return_kw_token_idx + 1, Precedence::new(0))
        .map_err(|e| e.add_error_context(no_return_expr_err_fn()))?
    else {
        return ParseResult::new_error_unexpected_eof(no_return_expr_err_fn(), return_kw_token_idx);
    };

    ParseResult::new_node(
        ast::AstNode::Expression(ast::Expr::Return {
            return_kw: return_kw_token_idx,
            expression_node_idx: Some(ast.push_node(expr)),
        }),
        after_expr_token_idx,
    )
}

fn parse_block_expression(
    ast: &mut ast::Ast<ParseError>,
    lcurlybrace_token_idx: TokenIdx,
) -> ParseResult {
    let mut statements_node_indices = Vec::with_capacity(25);
    let mut next_token_idx = lcurlybrace_token_idx + 1;
    let mut error = ParseError::no_error();
    loop {
        match ast.get_tokens().find_next_non_blank_token(next_token_idx) {
            Some((rcurlybrace_token_idx, token))
                if token.get_kind() == &lexer::TokenKind::RCurlyBrace =>
            {
                return ParseResult::new_node(
                    ast::AstNode::Expression(ast::Expr::Block {
                        lcurlybracket: lcurlybrace_token_idx,
                        statements_node_indices,
                        rcurlybracket: rcurlybrace_token_idx,
                    }),
                    rcurlybrace_token_idx + 1,
                );
            }
            _ => {}
        }

        match parse_statement(ast, next_token_idx) {
            Ok(HappyPath::Node {
                node: statement,
                next_token_idx: token_idx,
            }) => {
                statements_node_indices.push(ast.push_node(statement));
                next_token_idx = token_idx;
            }
            Ok(HappyPath::Finished) => {
                error = error.add_new_error(ParseError::SingleParseError(
                    SingleParseError::UnexpectedEof {
                        msg: format!(
                            "no matching `{}`",
                            lexer::TokenKind::RCurlyBrace.get_string_repr()
                        ),
                        ctx_token_idx: lcurlybrace_token_idx,
                    },
                ));
                break;
            }
            Err(e) => {
                error = error.add_new_error(e);
                next_token_idx = find_recovery_idx(ast.get_tokens(), next_token_idx);
            }
        }
    }

    Err(error)
}

fn parse_if_expression(ast: &mut ast::Ast<ParseError>, if_kw_token_idx: TokenIdx) -> ParseResult {
    // codition expression
    let no_cond_expr_err_fn = || {
        format!(
            "expected a logical expression after `{}`",
            lexer::TokenKind::KwIf.get_string_repr()
        )
    };

    let HappyPath::Node {
        node: cond_expr,
        next_token_idx: after_cond_expr_token_idx,
    } = parse_expression(ast, if_kw_token_idx + 1, Precedence::new(0))
        .map_err(|e| e.add_error_context(no_cond_expr_err_fn()))?
    else {
        return ParseResult::new_error_unexpected_eof(no_cond_expr_err_fn(), if_kw_token_idx);
    };

    // then {
    let then_lcurlybrace_token_idx = must_find(
        ast.get_tokens(),
        if_kw_token_idx,
        after_cond_expr_token_idx,
        || {
            format!(
                "expected a `{}` after `{}`",
                lexer::TokenKind::LCurlyBrace.get_string_repr(),
                lexer::TokenKind::KwIf.get_string_repr()
            )
        },
        |token_kind| token_kind == &lexer::TokenKind::LCurlyBrace,
    )?;

    let no_valid_block_err_fn = || "not a valid block";
    let HappyPath::Node {
        node: curly_block,
        next_token_idx: after_curly_block_token_idx,
    } = parse_block_expression(ast, then_lcurlybrace_token_idx)
        .map_err(|e| e.add_error_context(no_valid_block_err_fn()))?
    else {
        return ParseResult::new_error_unexpected_eof(no_valid_block_err_fn(), if_kw_token_idx);
    };

    // else
    let (else_kw_token_idx, _) = match ast
        .get_tokens()
        .find_next_non_blank_token(after_curly_block_token_idx)
    {
        Some((token_idx, token)) if token.get_kind() == &lexer::TokenKind::KwElse => {
            (token_idx, token)
        }
        _ => {
            return ParseResult::new_node(
                ast::AstNode::Expression(ast::Expr::If {
                    if_kw: if_kw_token_idx,
                    condition_node_idx: ast.push_node(cond_expr),
                    then_block_node_idx: ast.push_node(curly_block),
                    else_kw: None,
                    else_block_node_idx: None,
                    if_node_idx: None,
                }),
                after_curly_block_token_idx,
            );
        }
    };

    // whatever follows else, could be `if` or `{.. }`
    let nothing_after_else_err_fn = || {
        format!(
            "expected `{} .. {}` or `{}` after `{}`",
            lexer::TokenKind::LCurlyBrace.get_string_repr(),
            lexer::TokenKind::RCurlyBrace.get_string_repr(),
            lexer::TokenKind::KwIf.get_string_repr(),
            lexer::TokenKind::KwElse.get_string_repr()
        )
    };

    match ast
        .get_tokens()
        .find_next_non_blank_token(else_kw_token_idx + 1)
    {
        // else if
        Some((else_if_kw_token_idx, else_if_kw_token))
            if else_if_kw_token.get_kind() == &lexer::TokenKind::KwIf =>
        {
            let no_else_if_expr_err_fn = || {
                format!(
                    "not a valid `{} {}` expression",
                    lexer::TokenKind::KwElse.get_string_repr(),
                    lexer::TokenKind::KwIf.get_string_repr()
                )
            };
            let HappyPath::Node {
                node: else_if_expr,
                next_token_idx: after_else_if_expr_token_idx,
            } = parse_if_expression(ast, else_if_kw_token_idx)
                .map_err(|e| e.add_error_context(no_else_if_expr_err_fn()))?
            else {
                return ParseResult::new_error_unexpected_eof(
                    no_else_if_expr_err_fn(),
                    else_kw_token_idx,
                );
            };
            ParseResult::new_node(
                ast::AstNode::Expression(ast::Expr::If {
                    if_kw: if_kw_token_idx,
                    condition_node_idx: ast.push_node(cond_expr),
                    then_block_node_idx: ast.push_node(curly_block),
                    else_kw: Some(else_kw_token_idx),
                    else_block_node_idx: None,
                    if_node_idx: Some(ast.push_node(else_if_expr)),
                }),
                after_else_if_expr_token_idx,
            )
        }
        // else {
        Some((else_lcurlybrace_token_idx, else_lcurlybrace_token))
            if else_lcurlybrace_token.get_kind() == &lexer::TokenKind::LCurlyBrace =>
        {
            let no_else_block_err_fn = || {
                format!(
                    "not a valid `{} {}` expression",
                    lexer::TokenKind::KwElse.get_string_repr(),
                    lexer::TokenKind::LCurlyBrace.get_string_repr()
                )
            };
            let HappyPath::Node {
                node: else_block,
                next_token_idx: after_else_block_token_idx,
            } = parse_block_expression(ast, else_lcurlybrace_token_idx)
                .map_err(|e| e.add_error_context(no_else_block_err_fn()))?
            else {
                return ParseResult::new_error_unexpected_eof(
                    no_else_block_err_fn(),
                    else_kw_token_idx,
                );
            };
            ParseResult::new_node(
                ast::AstNode::Expression(ast::Expr::If {
                    if_kw: if_kw_token_idx,
                    condition_node_idx: ast.push_node(cond_expr),
                    then_block_node_idx: ast.push_node(curly_block),
                    else_kw: Some(else_kw_token_idx),
                    else_block_node_idx: Some(ast.push_node(else_block)),
                    if_node_idx: None,
                }),
                after_else_block_token_idx,
            )
        }
        Some((error_token_idx, _)) => ParseResult::new_error_unexpected_token(
            nothing_after_else_err_fn(),
            else_kw_token_idx,
            error_token_idx,
        ),
        None => {
            ParseResult::new_error_unexpected_eof(nothing_after_else_err_fn(), else_kw_token_idx)
        }
    }
}

/// parse_primary parses a primary expression.
///
/// # Note
/// it never returns `IntermediateResult::Finished`, cannot be eof
fn parse_primary(
    ast: &mut ast::Ast<ParseError>,
    start_token_idx: TokenIdx,
    start_token: &Token,
) -> ParseResult {
    match start_token.get_kind() {
        lexer::TokenKind::I64 => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::I64(start_token_idx)),
            start_token_idx + 1,
        ),
        lexer::TokenKind::Identifier { .. } => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::Identifier(start_token_idx)),
            start_token_idx + 1,
        ),
        // don't allow chained unary operators
        lexer::TokenKind::Minus => {
            must_be_i64_after_dash_sign(ast, start_token_idx + 1, start_token_idx)
        }
        lexer::TokenKind::LParen => must_be_paren_expression(ast, start_token_idx),
        lexer::TokenKind::StringLiteral { content } => ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::StringLiteral {
                token_idx: start_token_idx,
                content: content.clone(),
            }),
            start_token_idx + 1,
        ),
        _ => ParseResult::new_error_unexpected_token(
            "expected an expression",
            start_token_idx,
            start_token_idx,
        ),
    }
}

// either returns an expresion or an error
fn must_be_paren_expression(
    ast: &mut ast::Ast<ParseError>,
    lparen_token_idx: TokenIdx,
) -> ParseResult {
    let HappyPath::Node {
        node: expr,
        next_token_idx: after_expr_token_idx,
    } = parse_expression(ast, lparen_token_idx + 1, Precedence::new(0)).map_err(|e| {
        e.add_error_context(format!(
            "not a valid expression between `{}{}`",
            lexer::TokenKind::LParen.get_string_repr(),
            lexer::TokenKind::RParen.get_string_repr()
        ))
    })?
    else {
        return ParseResult::new_error_unexpected_eof(
            format!(
                "nothing after `{}`",
                lexer::TokenKind::LParen.get_string_repr()
            ),
            lparen_token_idx,
        );
    };

    let Some((rparen_token_idx, rparen_token)) = ast
        .get_tokens()
        .find_next_non_blank_token(after_expr_token_idx)
    else {
        return ParseResult::new_error_unexpected_eof(
            format!(
                "no matching `{}`",
                lexer::TokenKind::RParen.get_string_repr()
            ),
            lparen_token_idx,
        );
    };

    if rparen_token.get_kind() == &lexer::TokenKind::RParen {
        ParseResult::new_node(
            ast::AstNode::Expression(ast::Expr::Grouped {
                lparen: lparen_token_idx,
                expression_node_idx: ast.push_node(expr),
                rparen: rparen_token_idx,
            }),
            rparen_token_idx + 1,
        )
    } else {
        ParseResult::new_error_mismatched_paren(lparen_token_idx, rparen_token_idx)
    }
}

pub(super) fn get_lex_errors(tokens: &lexer::Tokens) -> Option<SingleParseError> {
    let invalid_errors = tokens
        .iter()
        .filter_map(|token| match token.get_kind() {
            lexer::TokenKind::Invalid {
                msg,
                error_fake_token_idx,
            } => Some((*error_fake_token_idx, msg.clone())),
            _ => None,
        })
        .collect::<Vec<_>>();

    if invalid_errors.is_empty() {
        None
    } else {
        Some(SingleParseError::LexErrors(invalid_errors))
    }
}
