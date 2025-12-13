#ifndef LIFE_LANG_RULES_HPP
#define LIFE_LANG_RULES_HPP

#include <string>
#include <string_view>
#include <tl/expected.hpp>

#include "ast.hpp"
#include "diagnostics.hpp"

namespace life_lang::parser {
using Iterator_Type = std::string::const_iterator;

template <typename Ast>
using Parse_Result = tl::expected<Ast, Diagnostic_Engine>;

// ============================================================================
// PUBLIC API
// ============================================================================

// Parse a complete module (compilation unit)
// Returns parsed module or diagnostic engine with errors
Parse_Result<ast::Module> parse_module(std::string_view a_source, std::string a_filename = "<input>");

}  // namespace life_lang::parser

#endif  // LIFE_LANG_RULES_HPP
