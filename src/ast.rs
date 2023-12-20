mod visitor;
use crate::lexer;

pub use visitor::{AstEvaluator, AstNodeVisitor, AstPrinter};

//#[derive(Debug)]
//struct LineNumber(usize);
//
//#[derive(Debug)]
//struct ColumnNumber(usize);
//
//#[derive(Debug)]
//pub(crate) struct SourceLocation {
//    line_nr: LineNumber,
//    column_nr: ColumnNumber,
//}
//
//#[derive(Debug)]
//pub(crate) struct SourceRange {
//    start: SourceLocation,
//    end: SourceLocation,
//}
//
//fn get_src_location(ast: &Ast, token_idx: lexer::TokenIndex) -> SourceLocation {
//    let token = &ast[token_idx];
//    let line_nr = LineNumber(token.get_line_nr());
//    let column_nr = ColumnNumber(token.get_column_nr());
//    SourceLocation { line_nr, column_nr }
//}
//
//fn split_lines(ast: &Ast) -> Vec<lexer::TokenIndex> {
//    ast.get_tokens()
//        .iter()
//        .enumerate()
//        .filter_map(|(i, token)| {
//            if token.get_kind() == &lexer::TokenKind::NewLine {
//                Some(lexer::TokenIndex::new(i))
//            } else {
//                None
//            }
//        })
//        .collect()
//}

#[derive(Debug)]
pub struct Ast<'cu> {
    cu: &'cu lexer::CompilationUnit,
    nodes: AstNodes, // last node is always a module
    tokens: lexer::Tokens,
}

impl<'cu> Ast<'cu> {
    pub(crate) fn new(cu: &'cu lexer::CompilationUnit) -> Self {
        let tokens = cu.get_tokens();
        let tokens_len = tokens.len();
        Self {
            cu,
            nodes: AstNodes::new(tokens_len),
            tokens,
        }
    }
    pub(crate) fn get_tokens(&self) -> &lexer::Tokens {
        &self.tokens
    }
    pub fn accept<R>(&self, visitor: &mut impl AstNodeVisitor<R>) -> R {
        visitor.visit(self.nodes.last())
    }
    pub(crate) fn to_string(&self, index: impl Indexable) -> String {
        index.get_string(self)
    }
    pub(crate) fn push(&mut self, node: AstNode) -> AstNodeIndex {
        self.nodes.push(node)
    }
    pub(crate) fn set_module(&mut self, module: AstNode) {
        match module {
            AstNode::Module { .. } => {
                self.push(module);
            }
            _ => panic!("BUG: expected Module, but got `{:?}`", module),
        }
    }
}

#[cfg(test)]
impl std::fmt::Display for Ast<'_> {
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
        )
    }
}

pub(crate) trait Indexable {
    fn get_string(&self, ast: &Ast) -> String;
}

impl Indexable for lexer::TokenIndex {
    fn get_string(&self, ast: &Ast) -> String {
        ast[*self].get_str(ast.cu).to_owned()
    }
}

impl Indexable for AstNodeIndex {
    fn get_string(&self, ast: &Ast) -> String {
        let mut printer = AstPrinter::new(ast);
        printer.visit(&ast[*self])
    }
}

#[derive(Debug)]
pub enum AstNode {
    Module {
        statements_node_indices: Vec<AstNodeIndex>,
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
    pub(crate) fn add_statement_to_module(&mut self, ast: &Ast, statement_node_idx: AstNodeIndex) {
        let printer = &mut AstPrinter::new(ast);

        let AstNode::Module {
            statements_node_indices,
        } = self
        else {
            panic!("BUG: expected Module, but got `{:?}`", ast.accept(printer));
        };

        let AstNode::Statement { .. } = &ast[statement_node_idx] else {
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
    I64(lexer::TokenIndex),
    Identifier(lexer::TokenIndex),
    StringLiteral {
        token_idx: lexer::TokenIndex,
        content: String,
    },
    BinaryOp {
        operator: lexer::TokenIndex,
        lhs: AstNodeIndex,
        rhs: AstNodeIndex,
    },
    UnaryOp {
        operator: lexer::TokenIndex,
        operand: AstNodeIndex,
    },
}

#[derive(Debug)]
pub enum Stat {
    Expression(AstNodeIndex),
    Definition {
        kw: lexer::TokenIndex,
        lhs_expression_node_idx: AstNodeIndex,
        eq: lexer::TokenIndex,
        rhs_expression_node_idx: AstNodeIndex,
    },
}

#[derive(Debug)]
pub(crate) struct AstNodes(Vec<AstNode>);

impl AstNodes {
    fn new(n: usize) -> Self {
        Self(Vec::with_capacity(n))
    }
    fn len(&self) -> usize {
        self.0.len()
    }
    fn push(&mut self, node: AstNode) -> AstNodeIndex {
        self.0.push(node);
        AstNodeIndex::new(self.len() - 1)
    }
    fn last(&self) -> &AstNode {
        self.0.last().unwrap_or_else(|| {
            panic!("BUG: each AstNodes must have at least one node, which is the Module node")
        })
    }
}

impl<'cu> std::ops::Index<AstNodeIndex> for Ast<'cu> {
    type Output = AstNode;

    fn index(&self, index: AstNodeIndex) -> &Self::Output {
        match self.nodes.0.get(index.get()) {
            Some(node) => node,
            None => panic!("BUG: AstNodeIndex out of bounds"),
        }
    }
}

impl<'cu> std::ops::Index<lexer::TokenIndex> for Ast<'cu> {
    type Output = lexer::Token;

    fn index(&self, index: lexer::TokenIndex) -> &Self::Output {
        &self.tokens[index]
    }
}

#[derive(Debug, Clone, Copy)]
pub struct AstNodeIndex(usize);

impl AstNodeIndex {
    fn new(index: usize) -> Self {
        Self(index)
    }
    fn get(&self) -> usize {
        self.0
    }
}
