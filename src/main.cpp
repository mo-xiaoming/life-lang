#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "diagnostics.hpp"
#include "parser/parser.hpp"
#include "parser/sexp.hpp"
#include "version.hpp"

int main(int argc, char* argv[]) {
  // Handle command line arguments
  if (argc > 1) {
    std::string_view const arg{argv[1]};
    if (arg == "--version" || arg == "-v") {
      std::cout << std::format("life-lang compiler version {}\n", life_lang::k_version);
      return 0;
    }
    if (arg == "--help" || arg == "-h") {
      std::cout << std::format("Usage: {} [OPTIONS]\n", argv[0]);
      std::cout << "Options:\n";
      std::cout << "  -v, --version    Show version information\n";
      std::cout << "  -h, --help       Show this help message\n";
      std::cout << "  -                Read source from stdin\n";
      return 0;
    }
    if (arg == "-") {
      std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());

      life_lang::Source_File_Registry registry;
      life_lang::File_Id const file_id = registry.register_file("<stdin>", std::move(input));
      life_lang::Diagnostic_Engine diagnostics{registry, file_id};
      life_lang::parser::Parser parser{diagnostics};
      auto const result = parser.parse_module();
      if (result) {
        // Print AST as indented S-expression (use 0 for compact)
        std::cout << std::format("{}\n", life_lang::ast::to_sexp_string(*result, 2));
        return 0;
      }
      diagnostics.print(std::cerr);
      return 1;
    }
  }
}
