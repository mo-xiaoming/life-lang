use colored::*;
use life_lang::{lexer, parser};

const PROG_NAME: &str = "llangc";

fn print_fatal_error(msg: &str) {
    eprintln!(
        "{}",
        &format!(
            "{prog}: {cate}: {msg}",
            prog = PROG_NAME.bold(),
            cate = "fatal error".bold().red(),
            msg = msg.red().bold()
        )
    );
}

fn print_usage() {
    eprintln!("Usage: {} [OPTIONS] <INPUT>", PROG_NAME);
}

fn main() {
    let args = std::env::args().collect::<Vec<_>>();
    if args.len() < 2 {
        print_fatal_error("no input files");
        print_usage();
        return;
    }

    for filename in &args[1..] {
        match lexer::CompilationUnit::from_file(filename) {
            Ok(cu) => {
                let ast = parser::parse(&cu);
                if let Some(diag) = ast.get_diagnostics() {
                    eprintln!("{}", diag);
                }
            }
            Err(e) => {
                print_fatal_error(&format!("failed to read source file `{}`, {}", filename, e));
            }
        }
    }
}
