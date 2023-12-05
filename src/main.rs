use life_lang::{lexer, parser};

fn main() {
    let cu = lexer::CompilationUnit::from_string(
        "stdin",
        " 7^2 + 3 * (12 / (+15 / -( 3+1) - - 1) ) - 2^3^4",
    );
    let parser = parser::Parser::new();
    let ast = parser.parse(&cu).unwrap();
    let printer = parser::PrintAstNodesVisitor;
    println!("{}", ast.accept(&printer).unwrap());
}
