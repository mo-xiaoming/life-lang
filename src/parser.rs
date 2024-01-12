mod dp;

use crate::{ast, lexer};
use colored::*;

#[derive(Debug)]
enum HappyPath {
    Finished,
    Node {
        node: ast::AstNode,
        next_token_idx: lexer::TokenIdx,
    },
}

type ParseResult = Result<HappyPath, ParseError>;

trait ParseResultExt {
    fn is_finished(&self) -> bool;
    fn new_node(node: ast::AstNode, next_token_idx: lexer::TokenIdx) -> Self;
    fn new_finished() -> Self;
    fn new_single_error(error: SingleParseError) -> Self;
    fn new_error_lex_error(errors: Vec<(lexer::TokenIdx, String)>) -> Self;
    fn new_error_unexpected_token(
        msg: impl Into<String>,
        start_token_idx: lexer::TokenIdx,
        inclusive_end_token_idx: lexer::TokenIdx,
    ) -> Self;
    fn new_error_unexpected_eof(msg: impl Into<String>, start_token_idx: lexer::TokenIdx) -> Self;
    fn new_error_mismatched_paren(
        lparen: lexer::TokenIdx,
        start_token_idx: lexer::TokenIdx,
    ) -> Self;
    fn add_upper_context_to_error(ctx_msg: impl Into<String>, error: ParseError) -> Self;
}

impl ParseResultExt for ParseResult {
    fn is_finished(&self) -> bool {
        matches!(self, Ok(HappyPath::Finished))
    }
    fn new_node(node: ast::AstNode, next_token_idx: lexer::TokenIdx) -> Self {
        Ok(HappyPath::Node {
            node,
            next_token_idx,
        })
    }
    fn new_finished() -> Self {
        Ok(HappyPath::Finished)
    }
    fn new_single_error(error: SingleParseError) -> Self {
        Err(ParseError::new_single_error(error))
    }
    fn new_error_lex_error(errors: Vec<(lexer::TokenIdx, String)>) -> Self {
        Err(ParseError::new_single_error(SingleParseError::LexErrors(
            errors,
        )))
    }
    fn new_error_unexpected_token(
        msg: impl Into<String>,
        start_token_idx: lexer::TokenIdx,
        error_token_idx: lexer::TokenIdx,
    ) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::UnexpectedToken {
                msg: msg.into(),
                ctx_start_token_idx: start_token_idx,
                error_token_idx,
            },
        ))
    }
    fn new_error_unexpected_eof(msg: impl Into<String>, start_token_idx: lexer::TokenIdx) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::UnexpectedEof {
                msg: msg.into(),
                ctx_token_idx: start_token_idx,
            },
        ))
    }
    fn new_error_mismatched_paren(
        lparen: lexer::TokenIdx,
        start_token_idx: lexer::TokenIdx,
    ) -> Self {
        Err(ParseError::new_single_error(
            SingleParseError::MismatchedParentheses {
                lparen,
                error_token_idx: start_token_idx,
            },
        ))
    }
    fn add_upper_context_to_error(ctx_msg: impl Into<String>, error: ParseError) -> Self {
        Err(error.add_error_context(ctx_msg))
    }
}

#[derive(Debug, Clone)]
pub enum ParseError {
    Empty,
    MutilParseError(Vec<ParseError>),
    SingleParseError(SingleParseError), // bare error
    ErrorWithContext(Vec<ParseError>),  // error with context [e, ctx1, ctx2, ...]
}

impl ParseError {
    fn no_error() -> Self {
        Self::Empty
    }
    fn new_single_error(error: SingleParseError) -> Self {
        Self::SingleParseError(error)
    }
    fn add_error_context(self, msg: impl Into<String>) -> Self {
        let context = ParseError::new_single_error(SingleParseError::Context { msg: msg.into() });
        match self {
            Self::ErrorWithContext(mut errors) => {
                errors.push(context);
                Self::ErrorWithContext(errors)
            }
            Self::Empty => panic!("BUG: cannot add context to empty error"),
            e => Self::ErrorWithContext(vec![e, context]),
        }
    }
    fn add_new_error(self, error: ParseError) -> Self {
        match self {
            Self::MutilParseError(mut errors) => {
                errors.push(error);
                Self::MutilParseError(errors)
            }
            Self::Empty => error,
            e => Self::MutilParseError(vec![e, error]),
        }
    }

    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseError>) -> String {
        const LEN_PER_ERROR: usize = 100;
        match self {
            Self::Empty => panic!("BUG: cannot get string from empty error"),
            Self::SingleParseError(error) => error.get_string(ast),
            Self::ErrorWithContext(errors) => {
                let mut s = String::with_capacity(errors.len() * LEN_PER_ERROR);
                for e in errors {
                    s.push_str(&e.get_string(ast));
                }
                s
            }
            Self::MutilParseError(errors) => {
                let mut s = String::with_capacity(errors.len() * LEN_PER_ERROR);
                for e in errors {
                    s.push_str(&e.get_string(ast));
                }
                s
            }
        }
    }
}

impl ast::AstError for ParseError {
    type E = ParseError;

    fn is_empty(&self) -> bool {
        matches!(self, Self::Empty)
    }
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, Self>) -> String {
        self.get_string(ast)
    }
}

#[derive(Debug, Clone)]
pub enum SingleParseError {
    Context {
        msg: String,
    },
    UnexpectedEof {
        msg: String,
        ctx_token_idx: lexer::TokenIdx,
    },
    IntegerOverflow {
        token: lexer::TokenIdx,
    },
    MismatchedParentheses {
        lparen: lexer::TokenIdx,
        error_token_idx: lexer::TokenIdx,
    },
    UnexpectedToken {
        msg: String,
        ctx_start_token_idx: lexer::TokenIdx,
        error_token_idx: lexer::TokenIdx,
    },
    LexErrors(Vec<(lexer::TokenIdx, String)>),
}

impl SingleParseError {
    fn get_context_msg(&self, msg: &str) -> String {
        format!("context: {}\n", msg).blue().bold().to_string()
    }
    fn get_error_msg<'cu>(
        &self,
        ast: &'cu ast::Ast<'cu, ParseError>,
        msg: &str,
        diag: &str,
    ) -> String {
        format!(
            "{filename}: {cate}: {msg}\n{diag}",
            filename = ast.get_input_origin().bold(),
            cate = "error".red().bold(),
            msg = msg.red().bold(),
            diag = diag,
        )
    }
    fn get_string<'cu>(&self, ast: &'cu ast::Ast<'cu, ParseError>) -> String {
        match self {
            Self::Context { msg } => self.get_context_msg(msg),
            Self::UnexpectedEof { msg, ctx_token_idx } => self.get_error_msg(
                ast,
                &format!("unexpected end of file, {}", msg),
                &ast.get_diag_with_ctx_token(*ctx_token_idx),
            ),
            Self::IntegerOverflow { token: token_idx } => self.get_error_msg(
                ast,
                &format!(
                    "integer overflow `{}`",
                    ast.get_string_unchecked(*token_idx)
                ),
                &ast.get_diag_with_ctx_token(*token_idx),
            ),
            Self::MismatchedParentheses {
                lparen,
                error_token_idx,
            } => self.get_error_msg(
                ast,
                &format!(
                    "mismatched parentheses `{}`",
                    ast.get_string_unchecked(*lparen)
                ),
                &[
                    ast.get_diag_with_ctx_token(*lparen),
                    ast.get_diag_with_ctx_token(*error_token_idx),
                ]
                .join(""),
            ),
            Self::UnexpectedToken {
                msg,
                ctx_start_token_idx: start_token_idx,
                error_token_idx,
            } => self.get_error_msg(
                ast,
                msg,
                &ast.get_diag_with_ctx_and_error_tokens(*start_token_idx, *error_token_idx),
            ),
            Self::LexErrors(errors) => {
                let mut result = String::with_capacity(errors.len() * 80);
                for (error_token_idx, msg) in errors {
                    let formatted = self.get_error_msg(
                        ast,
                        msg,
                        &ast.get_diag_with_error_token(*error_token_idx),
                    );
                    result.push_str(&formatted);
                }
                result
            }
        }
    }
}

pub fn parse(cu: &lexer::CompilationUnit) -> ast::Ast<ParseError> {
    let mut ast = ast::Ast::new(cu);

    let tokens = ast.get_tokens();
    match dp::get_lex_errors(tokens) {
        Some(lex_errors) => {
            ast.set_error(ParseError::new_single_error(lex_errors));
        }
        None => dp::parse_module(&mut ast, lexer::TokenIdx::new(0)),
    }
    ast
}

#[cfg(test)]
mod test_parser {
    use super::*;

    #[test]
    fn test_empty_ast() {
        for s in ["", " ", ";", "\r\n\n;", "  ;", "\r\n\n", ";;"] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu);
            // TODO: how to make error non-ignoreable?
            assert!(ast.get_error().is_none(), "ast: {}", ast);
        }
    }

    #[test]
    fn test_negative_numbers() {
        for (expected, test_data) in [
            ("-42;\n", vec!["-42;", "- 42;", " - 42 ;", " -42 ;"]),
            (
                "3 - -2;\n",
                vec!["3--2;", "3- -2;", " 3  - - 2 ;", "3 - -2 ;", " 3 --2 ;"],
            ),
            (
                "-3 - -2;\n",
                vec![
                    "-3--2;",
                    "-3- -2;",
                    "- 3 - - 2 ;",
                    " - 3  --2 ;",
                    " -3 -- 2 ;",
                ],
            ),
        ] {
            for s in test_data {
                let cu = lexer::CompilationUnit::from_string("stdin", s);
                let ast = parse(&cu);
                assert!(ast.get_error().is_none(), "ast: {}", ast);
                let result = ast.accept(&mut ast::AstPrinter::new(&ast));
                assert_eq!(result, expected, "input: {}, ast: {}", s, ast);
            }
        }
    }

    #[test]
    fn test_definitions() {
        let cu = lexer::CompilationUnit::from_string("stdin", "let x = 3; var y = x - 42;");
        let ast = parse(&cu);
        assert!(ast.get_error().is_none(), "ast: {}", ast);
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(
            ast.accept(printer),
            "let x = 3;\nvar y = x - 42;\n",
            "ast: {}",
            ast
        );
    }

    #[test]
    fn test_eval() {
        for (s, expected) in [
            ("1;", 1i64),
            ("1+1;", 2),
            ("1-1;", 0),
            ("7%2 + 3 * (12 / ( 15 / - 3+1 - - 1) ) - 2 - 1 + 1;", -13),
        ] {
            let cu = lexer::CompilationUnit::from_string("stdin", s);
            let ast = parse(&cu);
            assert!(ast.get_error().is_none(), "ast: {}", ast);
            let result = ast.accept(&mut ast::AstEvaluator::new(&ast));
            assert!(
                matches!(result, Ok(Some(got)) if expected == got),
                "expected: {:?}, got: {:?}",
                expected,
                result
            );
        }
    }

    #[test]
    fn test_string() {
        let cu = lexer::CompilationUnit::from_string(
            "stdin",
            r#"" \u{41} x\u{4f60}xy{}\u{597d}a\u{1f316}";"#,
        );
        let ast = parse(&cu);
        assert!(ast.get_error().is_none(), "ast: {}", ast);
        let printer = &mut ast::AstPrinter::new(&ast);
        assert_eq!(ast.accept(printer), " A x你xy{}好a🌖;\n");
    }

    #[test]
    fn test_if_else() {
        let cu = lexer::CompilationUnit::from_string(
            "stdin",
            r#"
let x = if 3 > y {
    return 9;
} else if 3 == y {
    return 42;
} else {
    return 0;
};
"#,
        );
        let ast = parse(&cu);
        assert!(ast.get_error().is_none(), "ast: {}", ast);
        let printer = &mut ast::AstPrinter::new(&ast);
        use pretty_assertions::assert_eq;
        assert_eq!(
            ast.accept(printer),
            r#"let x = if 3 > y {
return 9;
} else if 3 == y {
return 42;
} else {
return 0;
};
"#
        );
    }
}
