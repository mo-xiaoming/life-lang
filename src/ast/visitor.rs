use super::{Ast, AstError, AstNode, Expr, Stat};
use crate::lexer;

pub trait AstNodeVisitor<R> {
    fn visit(&mut self, node: &AstNode) -> R;
}

#[derive(Debug)]
pub struct AstPrinter<'cu, E: AstError> {
    ast: &'cu Ast<'cu, E>,
}

impl<'cu, E: AstError> AstPrinter<'cu, E> {
    pub fn new(ast: &'cu Ast<'cu, E>) -> Self {
        Self { ast }
    }
}

impl<'cu, E: AstError> AstNodeVisitor<String> for AstPrinter<'cu, E> {
    fn visit(&mut self, node: &AstNode) -> String {
        match node {
            AstNode::Module {
                statements_node_indices,
            } => statements_node_indices
                .iter()
                .map(|idx| self.visit(self.ast.get_node_unchecked(*idx)))
                .collect(),
            AstNode::Statement(Stat::Definition {
                kw,
                lhs_expression_node_idx,
                eq,
                rhs_expression_node_idx,
            }) => {
                format!(
                    "{} {} {} {};\n",
                    self.ast.get_string_unchecked(*kw),
                    self.ast.get_string_unchecked(*lhs_expression_node_idx),
                    self.ast.get_string_unchecked(*eq),
                    self.ast.get_string_unchecked(*rhs_expression_node_idx)
                )
            }
            AstNode::Statement(Stat::Expression(expression_node_idx)) => {
                format!(
                    "{};\n",
                    self.visit(self.ast.get_node_unchecked(*expression_node_idx))
                )
            }
            AstNode::Expression(Expr::If {
                if_kw,
                condition_node_idx,
                then_block_node_idx,
                else_kw,
                else_block_node_idx,
                if_node_idx,
            }) => {
                format!(
                    "{} {} {} {}",
                    self.ast.get_string_unchecked(*if_kw),
                    self.ast.get_string_unchecked(*condition_node_idx),
                    self.ast.get_string_unchecked(*then_block_node_idx),
                    {
                        let mut else_str = String::with_capacity(200);
                        if let Some(else_kw) = else_kw {
                            else_str.push_str(&self.ast.get_string_unchecked(*else_kw));
                            else_str.push(' ');
                            if let Some(else_block_node_idx) = else_block_node_idx {
                                else_str
                                    .push_str(&self.ast.get_string_unchecked(*else_block_node_idx));
                            }
                            if let Some(if_node_idx) = if_node_idx {
                                else_str.push_str(&self.ast.get_string_unchecked(*if_node_idx));
                            }
                        }
                        else_str
                    }
                )
            }
            AstNode::Expression(Expr::I64(token_idx)) => self.ast.get_string_unchecked(*token_idx),
            AstNode::Expression(Expr::Identifier(token_idx)) => {
                self.ast.get_string_unchecked(*token_idx)
            }
            AstNode::Expression(Expr::StringLiteral { content, .. }) => content.clone(),
            AstNode::Expression(Expr::ArithmeticOrLogical { operator, lhs, rhs }) => {
                format!(
                    "{} {} {}",
                    self.ast.get_string_unchecked(*lhs),
                    self.ast.get_string_unchecked(*operator),
                    self.ast.get_string_unchecked(*rhs)
                )
            }
            AstNode::Expression(Expr::Negation { operator, operand }) => {
                match self.ast.get_token_unchecked(*operator).get_kind() {
                    lexer::TokenKind::Minus => {
                        format!("-{}", self.ast.get_string_unchecked(*operand))
                    }
                    _ => panic!(
                        "BUG: unsupported unary operator `{}`",
                        self.ast.get_string_unchecked(*operator)
                    ),
                }
            }
            AstNode::Expression(Expr::Grouped {
                lparen,
                expression_node_idx,
                rparen,
            }) => {
                format!(
                    "{} {} {}",
                    self.ast.get_string_unchecked(*lparen),
                    self.ast.get_string_unchecked(*expression_node_idx),
                    self.ast.get_string_unchecked(*rparen)
                )
            }
            AstNode::Expression(Expr::Block {
                lcurlybracket,
                statements_node_indices,
                rcurlybracket,
            }) => {
                format!(
                    "{}\n{}{}",
                    self.ast.get_string_unchecked(*lcurlybracket),
                    statements_node_indices
                        .iter()
                        .map(|idx| self.visit(self.ast.get_node_unchecked(*idx)))
                        .collect::<Vec<_>>()
                        .join(""),
                    self.ast.get_string_unchecked(*rcurlybracket)
                )
            }
            AstNode::Expression(Expr::Return {
                return_kw,
                expression_node_idx,
            }) => {
                format!(
                    "{} {}",
                    self.ast.get_string_unchecked(*return_kw),
                    expression_node_idx
                        .map(|idx| self.ast.get_string_unchecked(idx))
                        .unwrap_or_else(|| "".to_owned())
                )
            }
        }
    }
}

#[derive(Debug)]
pub struct AstEvaluator<'cu, E: AstError> {
    ast: &'cu Ast<'cu, E>,
}

impl<'cu, E: AstError> AstEvaluator<'cu, E> {
    pub fn new(ast: &'cu Ast<'cu, E>) -> Self {
        Self { ast }
    }
}

impl<'cu, E: AstError> AstNodeVisitor<Result<Option<i64>, String>> for AstEvaluator<'cu, E> {
    fn visit(&mut self, node: &AstNode) -> Result<Option<i64>, String> {
        match node {
            AstNode::Module {
                statements_node_indices,
            } => {
                // TODO: should be (), just returning the value of the first statement for now
                if statements_node_indices.is_empty() {
                    return Ok(None);
                }
                self.visit(self.ast.get_node_unchecked(statements_node_indices[0]))
            }
            AstNode::Expression(Expr::If {
                condition_node_idx,
                then_block_node_idx: then_node_idx,
                else_block_node_idx: else_node_idx,
                ..
            }) => {
                let condition_value = self
                    .visit(self.ast.get_node_unchecked(*condition_node_idx))?
                    .unwrap_or_else(|| {
                        panic!(
                            "BUG: expected condition to be Some, but got None: {}",
                            self.ast.get_string_unchecked(*condition_node_idx)
                        )
                    });
                if condition_value != 0 {
                    self.visit(self.ast.get_node_unchecked(*then_node_idx))
                } else if let Some(else_node_idx) = else_node_idx {
                    self.visit(self.ast.get_node_unchecked(*else_node_idx))
                } else {
                    Ok(None)
                }
            }
            AstNode::Expression(Expr::Block {
                statements_node_indices,
                ..
            }) => {
                unimplemented!("{:?}", statements_node_indices);
            }
            AstNode::Statement(Stat::Expression(expression_node_idx)) => {
                // TODO: should be (), just returning the value for expression for now
                self.visit(self.ast.get_node_unchecked(*expression_node_idx))
            }
            AstNode::Statement(Stat::Definition { .. }) => {
                panic!("doesn't support eval a definition")
            }
            AstNode::Expression(Expr::I64(token_idx)) => {
                let token = &self.ast.get_token_unchecked(*token_idx);
                match token.get_kind() {
                    lexer::TokenKind::I64 => self
                        .ast
                        .get_string_unchecked(*token_idx)
                        .parse()
                        .map_err(|e| {
                            format!(
                                "BUG: failed to parse i64 token `{}`: {}",
                                self.ast.get_string_unchecked(*token_idx),
                                e
                            )
                        })
                        .map(Some),
                    _ => panic!("BUG: expected i64 token, but got `{:?}`", token.get_kind()),
                }
            }
            AstNode::Expression(Expr::StringLiteral { .. }) => {
                panic!("doesn't support eval a string literal")
            }
            AstNode::Expression(Expr::Identifier(_)) => {
                panic!("BUG: doesn't support eval an identifier")
            }
            AstNode::Expression(Expr::ArithmeticOrLogical { operator, lhs, rhs }) => {
                let lhs_str = self.ast.get_string_unchecked(*lhs);
                let rhs_str = self.ast.get_string_unchecked(*rhs);
                let lhs_value = self
                    .visit(self.ast.get_node_unchecked(*lhs))?
                    .unwrap_or_else(|| {
                        panic!(
                            "BUG: expected lhs to be Some, but got None: {}",
                            self.ast.get_string_unchecked(*lhs)
                        )
                    });
                let rhs_value = self
                    .visit(self.ast.get_node_unchecked(*rhs))?
                    .unwrap_or_else(|| {
                        panic!(
                            "BUG: expected rhs to be Some, but got None: {}",
                            self.ast.get_string_unchecked(*rhs)
                        )
                    });
                match self.ast.get_token_unchecked(*operator).get_kind() {
                    lexer::TokenKind::Plus => lhs_value
                        .checked_add(rhs_value)
                        .ok_or_else(|| format!("`{}` + `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Minus => lhs_value
                        .checked_sub(rhs_value)
                        .ok_or_else(|| format!("`{}` - `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Star => lhs_value
                        .checked_mul(rhs_value)
                        .ok_or_else(|| format!("`{}` * `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Slash => lhs_value
                        .checked_div(rhs_value)
                        .ok_or_else(|| format!("`{}` / `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Percent => lhs_value
                        .checked_rem(rhs_value)
                        .ok_or_else(|| format!("`{}` % `{}` overflows", lhs_str, rhs_str)),
                    _ => panic!(
                        "BUG: unsupported binary operator `{}`",
                        self.ast.get_string_unchecked(*operator)
                    ),
                }
                .map(Some)
            }
            AstNode::Expression(Expr::Negation { operator, operand }) => {
                match self.ast.get_token_unchecked(*operator).get_kind() {
                    lexer::TokenKind::Minus => {
                        format!("-{}", self.ast.get_string_unchecked(*operand))
                            .parse()
                            .map_err(|e| {
                                format!(
                                    "BUG: failed to parse i64 token `{}`: {}",
                                    self.ast.get_string_unchecked(*operand),
                                    e
                                )
                            })
                            .map(Some)
                    }
                    _ => panic!(
                        "BUG: unsupported unary operator `{}`",
                        self.ast.get_string_unchecked(*operator)
                    ),
                }
            }
            AstNode::Expression(Expr::Grouped {
                expression_node_idx,
                ..
            }) => self.visit(self.ast.get_node_unchecked(*expression_node_idx)),
            AstNode::Expression(Expr::Return {
                expression_node_idx,
                ..
            }) => self.visit(self.ast.get_node_unchecked(expression_node_idx.unwrap())),
        }
    }
}
