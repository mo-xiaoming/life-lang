use life_lang::{ast, lexer, parser};

fn main() {
    let cu = lexer::CompilationUnit::from_string(
        "stdin",
        "7%2 + 3 * (12 / ( 15 / - 3+1 - - 1) ) - 2 - 1 + 1",
    );
    let ast = parser::parse(&cu).unwrap();
    let printer = &mut ast::AstEvaluator::new(&ast);
    println!(
        "should be -13, got {}",
        ast.accept(printer).unwrap().unwrap()
    );
}
