#![allow(dead_code)]
mod visitor;

use crate::lexer::TokenIdx;

use super::lexer;
pub use visitor::{AstEvaluator, AstNodeVisitor, AstPrinter};

pub trait AstError: std::marker::Sized + std::fmt::Debug {
    fn get_string<'cu>(&self, ast: &'cu Ast<'cu, Self>) -> String;
}

#[derive(Debug)]
pub struct Ast<'cu, E: AstError> {
    cu: &'cu lexer::CompilationUnit,
    nodes: AstNodes, // last node is always a module
    tokens: lexer::Tokens,
    diag_ctx: lexer::DiagCtx<'cu>,
    error: Option<E>,
}

impl<'cu, E: AstError> Ast<'cu, E> {
    pub(crate) fn new(cu: &'cu lexer::CompilationUnit) -> Self {
        let (tokens, diag_ctx) = cu.get_tokens();
        let tokens_len = tokens.len();
        Self {
            cu,
            nodes: AstNodes::with_capacity(tokens_len),
            tokens,
            diag_ctx,
            error: None,
        }
    }
    pub(crate) fn get_tokens(&self) -> &lexer::Tokens {
        &self.tokens
    }
    pub(crate) fn get_diag_ctx(&self) -> &lexer::DiagCtx {
        &self.diag_ctx
    }
    pub fn accept<R>(&self, visitor: &mut impl AstNodeVisitor<R>) -> R {
        visitor.visit(self.nodes.last())
    }
    fn get_token(&self, token_idx: lexer::TokenIdx) -> Option<&lexer::Token> {
        self.tokens.get(token_idx)
    }
    fn get_token_unchecked(&self, token_idx: lexer::TokenIdx) -> &lexer::Token {
        self.tokens
            .get(token_idx)
            .unwrap_or_else(|| panic!("BUG: failed to get token {:?} from ast {}", token_idx, self))
    }
    fn get_node(&self, node_idx: AstNodeIdx) -> Option<&AstNode> {
        self.nodes.get(node_idx)
    }
    fn get_node_unchecked(&self, node_idx: AstNodeIdx) -> &AstNode {
        self.nodes
            .get(node_idx)
            .unwrap_or_else(|| panic!("BUG: failed to get node {:?} from ast {}", node_idx, self))
    }
    pub(crate) fn get_string_unchecked(
        &'cu self,
        index: impl Indexable + std::fmt::Debug,
    ) -> String {
        index.get_string(self).unwrap_or_else(|| {
            panic!(
                "BUG: failed to get string from index {:?} in {:?}",
                index, self
            )
        })
    }
    pub fn get_diagnostics(&self, start_token_idx: TokenIdx) -> String {
        self.get_diag_ctx()
            .get_context(start_token_idx, &self.tokens, self.cu)
            .to_string()
    }
    pub fn get_diagnostics_with_error_token(
        &self,
        start_token_idx: TokenIdx,
        error_token_idx: TokenIdx,
    ) -> String {
        self.get_diag_ctx()
            .get_context_with_error_token(start_token_idx, error_token_idx, &self.tokens, self.cu)
            .to_string()
    }
    pub(crate) fn push_node(&mut self, node: AstNode) -> AstNodeIdx {
        self.nodes.push(node)
    }
    pub(crate) fn set_module(&mut self, module: AstNode) {
        match module {
            AstNode::Module { .. } => {
                self.push_node(module);
            }
            _ => panic!("BUG: expected Module, but got `{:?}`", module),
        }
    }
    pub(crate) fn set_error(&mut self, error: E) {
        self.error = Some(error);
    }
    pub(crate) fn get_error(&self) -> Option<&E> {
        self.error.as_ref()
    }
}

impl<'cu, E: AstError> std::fmt::Display for Ast<'cu, E> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "\ntokens:\n{}",
            self.tokens.iter().enumerate().fold(
                String::with_capacity(1000),
                |mut acc, (i, token)| {
                    acc.push_str(&format!("{:>5}: `{}`\n", i, token.get_str(self.cu)));
                    acc
                }
            )
        )?;
        write!(
            f,
            "\nnodes:\n{}",
            self.nodes.0.iter().enumerate().fold(
                String::with_capacity(1000),
                |mut acc, (i, node)| {
                    acc.push_str(&format!("{:>5}: {:?}\n", i, node));
                    acc
                }
            )
        )?;
        if let Some(e) = self.get_error() {
            let s = e.get_string(self);
            write!(f, "\nerror:\n{}", s)?;
        }
        Ok(())
    }
}

pub(crate) trait Indexable {
    fn get_string<'cu, E: AstError>(&self, ast: &'cu Ast<'cu, E>) -> Option<String>;
}

impl Indexable for lexer::TokenIdx {
    fn get_string<'cu, E: AstError>(&self, ast: &'cu Ast<'cu, E>) -> Option<String> {
        ast.get_token(*self)
            .map(|token| token.get_str(ast.cu).to_owned())
    }
}

impl Indexable for AstNodeIdx {
    fn get_string<'cu, E: AstError>(&self, ast: &'cu Ast<'cu, E>) -> Option<String> {
        let mut printer = AstPrinter::new(ast);
        ast.get_node(*self).map(|node| printer.visit(node))
    }
}

#[derive(Debug)]
pub enum AstNode {
    Module {
        statements_node_indices: Vec<AstNodeIdx>,
    },
    Statement(Stat),
    Expression(Expr),
}

impl AstNode {
    pub(crate) fn new_module(n: usize) -> Self {
        Self::Module {
            statements_node_indices: Vec::with_capacity(n),
        }
    }
    pub(crate) fn add_statement_to_module<'cu, E: AstError>(
        &mut self,
        ast: &'cu Ast<'cu, E>,
        statement_node_idx: AstNodeIdx,
    ) {
        let printer = &mut AstPrinter::new(ast);

        let AstNode::Module {
            statements_node_indices,
        } = self
        else {
            panic!("BUG: expected Module, but got `{:?}`", ast.accept(printer));
        };

        let Some(AstNode::Statement { .. }) = ast.get_node(statement_node_idx) else {
            panic!(
                "BUG: expected Statement, but got `{:?}`",
                ast.accept(printer)
            );
        };

        statements_node_indices.push(statement_node_idx);
    }
}

#[derive(Debug)]
pub enum Expr {
    I64(lexer::TokenIdx),
    Identifier(lexer::TokenIdx),
    StringLiteral {
        token_idx: lexer::TokenIdx,
        content: String,
    },
    BinaryOp {
        operator: lexer::TokenIdx,
        lhs: AstNodeIdx,
        rhs: AstNodeIdx,
    },
    UnaryOp {
        operator: lexer::TokenIdx,
        operand: AstNodeIdx,
    },
}

#[derive(Debug)]
pub enum Stat {
    Expression(AstNodeIdx),
    Definition {
        kw: lexer::TokenIdx,
        lhs_expression_node_idx: AstNodeIdx,
        eq: lexer::TokenIdx,
        rhs_expression_node_idx: AstNodeIdx,
    },
}

#[derive(Debug)]
pub(crate) struct AstNodes(Vec<AstNode>);

impl AstNodes {
    fn with_capacity(n: usize) -> Self {
        Self(Vec::with_capacity(n))
    }
    fn len(&self) -> usize {
        self.0.len()
    }
    fn get(&self, idx: AstNodeIdx) -> Option<&AstNode> {
        self.0.get(idx.get())
    }
    #[must_use]
    fn push(&mut self, node: AstNode) -> AstNodeIdx {
        self.0.push(node);
        AstNodeIdx::new(self.len() - 1)
    }
    fn last(&self) -> &AstNode {
        self.0.last().unwrap_or_else(|| {
            panic!("BUG: each AstNodes must have at least one node, which is the Module node")
        })
    }
}

#[derive(Debug, Clone, Copy)]
pub struct AstNodeIdx(usize);

impl AstNodeIdx {
    fn new(index: usize) -> Self {
        Self(index)
    }
    fn get(&self) -> usize {
        self.0
    }
}
