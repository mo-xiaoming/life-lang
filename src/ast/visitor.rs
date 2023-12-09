use super::{Ast, AstNode, Expr};
use crate::lexer;

pub trait AstNodeVisitor<R> {
    fn visit(&mut self, node: &AstNode) -> R;
}

#[derive(Debug)]
pub struct AstPrinter<'cu> {
    ast: &'cu Ast<'cu>,
}

impl<'cu> AstPrinter<'cu> {
    pub fn new(ast: &'cu Ast) -> Self {
        Self { ast }
    }
}

impl<'cu> AstNodeVisitor<String> for AstPrinter<'cu> {
    fn visit(&mut self, node: &AstNode) -> String {
        match node {
            AstNode::Module {
                statements_node_indices,
            } => statements_node_indices
                .iter()
                .map(|idx| self.visit(&self.ast[*idx]))
                .collect(),
            AstNode::Statement {
                expression_node_idx,
            } => self.visit(&self.ast[*expression_node_idx]),
            AstNode::Expression(Expr::I64(token)) => self.ast.to_string(*token),
            AstNode::Expression(Expr::BinaryOp { operator, lhs, rhs }) => {
                format!(
                    "({} {} {})",
                    self.ast.to_string(*lhs),
                    self.ast.to_string(*operator),
                    self.ast.to_string(*rhs)
                )
            }
            AstNode::Expression(Expr::UnaryOp { operator, operand }) => {
                match self.ast[*operator].get_kind() {
                    lexer::TokenKind::Dash => {
                        format!("-{}", self.ast.to_string(*operand))
                    }
                    _ => panic!(
                        "BUG: unsupported unary operator `{}`",
                        self.ast.to_string(*operator)
                    ),
                }
            }
        }
    }
}

#[derive(Debug)]
pub struct AstEvaluator<'cu> {
    ast: &'cu Ast<'cu>,
}

impl<'cu> AstEvaluator<'cu> {
    pub fn new(ast: &'cu Ast<'cu>) -> Self {
        Self { ast }
    }
}

impl<'cu> AstNodeVisitor<Result<Option<i64>, String>> for AstEvaluator<'cu> {
    fn visit(&mut self, node: &AstNode) -> Result<Option<i64>, String> {
        match node {
            AstNode::Module {
                statements_node_indices,
            } => {
                // TODO: should be (), just returning the value of the first statement for now
                if statements_node_indices.is_empty() {
                    return Ok(None);
                }
                self.visit(&self.ast[statements_node_indices[0]])
            }
            AstNode::Statement {
                expression_node_idx,
            } => {
                // TODO: should be (), just returning the value for expression for now
                self.visit(&self.ast[*expression_node_idx])
            }
            AstNode::Expression(Expr::I64(token_idx)) => {
                let token = &self.ast[*token_idx];
                match token.get_kind() {
                    lexer::TokenKind::Int64 => self
                        .ast
                        .to_string(*token_idx)
                        .parse()
                        .map_err(|e| {
                            format!(
                                "BUG: failed to parse i64 token `{}`: {}",
                                self.ast.to_string(*token_idx),
                                e
                            )
                        })
                        .map(Some),
                    _ => panic!("BUG: expected i64 token, but got `{:?}`", token.get_kind()),
                }
            }
            AstNode::Expression(Expr::BinaryOp { operator, lhs, rhs }) => {
                let lhs_str = self.ast.to_string(*lhs);
                let rhs_str = self.ast.to_string(*rhs);
                let lhs_value = self.visit(&self.ast[*lhs])?.unwrap_or_else(|| {
                    panic!(
                        "BUG: expected lhs to be Some, but got None: {}",
                        self.ast.to_string(*lhs)
                    )
                });
                let rhs_value = self.visit(&self.ast[*rhs])?.unwrap_or_else(|| {
                    panic!(
                        "BUG: expected rhs to be Some, but got None: {}",
                        self.ast.to_string(*rhs)
                    )
                });
                match self.ast[*operator].get_kind() {
                    lexer::TokenKind::Plus => lhs_value
                        .checked_add(rhs_value)
                        .ok_or_else(|| format!("`{}` + `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Dash => lhs_value
                        .checked_sub(rhs_value)
                        .ok_or_else(|| format!("`{}` - `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Star => lhs_value
                        .checked_mul(rhs_value)
                        .ok_or_else(|| format!("`{}` * `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Slash => lhs_value
                        .checked_div(rhs_value)
                        .ok_or_else(|| format!("`{}` / `{}` overflows", lhs_str, rhs_str)),
                    lexer::TokenKind::Percentage => lhs_value
                        .checked_rem(rhs_value)
                        .ok_or_else(|| format!("`{}` % `{}` overflows", lhs_str, rhs_str)),
                    _ => panic!(
                        "BUG: unsupported binary operator `{}`",
                        self.ast.to_string(*operator)
                    ),
                }
                .map(Some)
            }
            AstNode::Expression(Expr::UnaryOp { operator, operand }) => {
                match self.ast[*operator].get_kind() {
                    lexer::TokenKind::Dash => format!("-{}", self.ast.to_string(*operand))
                        .parse()
                        .map_err(|e| {
                            format!(
                                "BUG: failed to parse i64 token `{}`: {}",
                                self.ast.to_string(*operand),
                                e
                            )
                        })
                        .map(Some),
                    _ => panic!(
                        "BUG: unsupported unary operator `{}`",
                        self.ast.to_string(*operator)
                    ),
                }
            }
        }
    }
}
