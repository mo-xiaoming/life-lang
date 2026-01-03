// Parser Implementation for life-lang
//
// CRITICAL: This parser must implement the grammar defined in doc/GRAMMAR.md exactly.
//
// Grammar Synchronization Rules:
// 1. doc/GRAMMAR.md is the authoritative source of truth for language syntax
// 2. Every parse_* method corresponds to a grammar rule in doc/GRAMMAR.md
// 3. When adding/modifying parse_* methods, update doc/GRAMMAR.md accordingly
// 4. When changing grammar rules, update corresponding parse_* methods
// 5. Parser must NOT accept inputs that violate the grammar
//
// Key Implementation Notes:
// - parse_module(): Enforces module = { item }, rejects non-item statements
// - Recursive descent: Each non-terminal becomes a parse_* method
// - Diagnostics: All errors recorded in m_impl->diagnostics with source positions
//
// See doc/GRAMMAR.md for the complete EBNF specification.

#include "parser.hpp"

#include "../diagnostics.hpp"
#include "utils.hpp"

#include <array>
#include <cctype>
#include <format>
#include <string>

namespace life_lang::parser {

namespace {
// Identifier checks
[[nodiscard]] bool is_identifier_start(char ch_) {
  return std::isalpha(static_cast<unsigned char>(ch_)) != 0 || ch_ == '_';
}

[[nodiscard]] bool is_identifier_continue(char ch_) {
  return std::isalnum(static_cast<unsigned char>(ch_)) != 0 || ch_ == '_';
}

// Operator precedence and parsing helpers
[[nodiscard]] constexpr int get_precedence(ast::Binary_Op op_) {
  // Precedence levels (higher = tighter binding):
  // 1: || (logical OR)
  // 2: && (logical AND)
  // 3: | (bitwise OR)
  // 4: ^ (bitwise XOR)
  // 5: & (bitwise AND)
  // 6: ==, != (equality)
  // 7: <, >, <=, >= (comparison)
  // 8: <<, >> (shift)
  // 9: +, - (additive)
  // 10: *, /, % (multiplicative)

  switch (op_) {
    case ast::Binary_Op::Or:
      return 1;
    case ast::Binary_Op::And:
      return 2;
    case ast::Binary_Op::Bit_Or:
      return 3;
    case ast::Binary_Op::Bit_Xor:
      return 4;
    case ast::Binary_Op::Bit_And:
      return 5;
    case ast::Binary_Op::Eq:
    case ast::Binary_Op::Ne:
      return 6;
    case ast::Binary_Op::Lt:
    case ast::Binary_Op::Gt:
    case ast::Binary_Op::Le:
    case ast::Binary_Op::Ge:
      return 7;
    case ast::Binary_Op::Shl:
    case ast::Binary_Op::Shr:
      return 8;
    case ast::Binary_Op::Add:
    case ast::Binary_Op::Sub:
      return 9;
    case ast::Binary_Op::Mul:
    case ast::Binary_Op::Div:
    case ast::Binary_Op::Mod:
      return 10;
  }
  unreachable();
}

// Sentinel value for end-of-file or non-existent character
constexpr char k_eof_char = '\0';

}  // namespace

// ============================================================================
// Parser::Impl - Private implementation (pimpl pattern)
// ============================================================================

struct Parser::Impl {
  std::size_t pos = 0;  // Current position in source
  Diagnostic_Engine* diagnostics = nullptr;

  // Lexical helpers
  char peek() const;
  char peek(std::size_t offset_) const;
  char advance(std::size_t count_ = 1);
  void skip_whitespace_and_comments();
  [[nodiscard]] bool is_at_end() const;
  [[nodiscard]] Source_Position current_position() const;
  [[nodiscard]] Source_Range make_range(Source_Position start_) const;

  // Error reporting
  void error(std::string message_, Source_Range range_) const;
  void error(std::string message_) const;

  // Token matching
  bool expect(char ch_);
  bool expect(std::string_view str_);
  bool match_keyword(std::string_view keyword_);
  bool match_operator(std::string_view op_);
  [[nodiscard]] bool lookahead(std::string_view str_) const;

  // Parsing helpers
  [[nodiscard]] std::optional<ast::Binary_Op> try_parse_binary_op();
  [[nodiscard]] std::optional<ast::Binary_Op> try_parse_binary_op_with_min_precedence(int min_precedence_);
  [[nodiscard]] std::optional<ast::Unary_Op> try_parse_unary_op();
  [[nodiscard]] std::optional<ast::Statement> try_parse_expr_as_statement(Parser* parser_);

  // Digit collection helpers (returns last char seen, appends to value_)
  template <typename Predicate>
  [[nodiscard]] char collect_digits(std::string& value_, Predicate is_valid_digit_);

  // Speculative parsing: try a parse operation, restore position if it returns nullopt
  template <typename F>
  auto try_parse(F&& parse_fn_) -> decltype(parse_fn_());
};

char Parser::Impl::peek() const {
  return peek(0UL);
}

char Parser::Impl::peek(std::size_t offset_) const {
  std::size_t const p = pos + offset_;
  if (p >= diagnostics->source().size()) {
    return k_eof_char;
  }
  return diagnostics->source()[p];
}

char Parser::Impl::advance(std::size_t count_) {
  if (count_ == 0) {
    return peek();
  }

  auto const new_pos = pos + count_;
  if (new_pos > diagnostics->source().size()) {
    return k_eof_char;
  }

  char const last_char = peek(count_ - 1);
  pos = new_pos;
  return last_char;
}

bool Parser::Impl::is_at_end() const {
  return pos >= diagnostics->source().size();
}

template <typename Predicate>
char Parser::Impl::collect_digits(std::string& value_, Predicate is_valid_digit_) {
  char last_char = peek();
  while (is_valid_digit_(peek()) || peek() == '_') {
    last_char = peek();
    char const ch = advance();
    if (ch != '_') {
      value_ += ch;
    }
  }
  return last_char;
}

template <typename F>
auto Parser::Impl::try_parse(F&& parse_fn_) -> decltype(parse_fn_()) {
  auto const saved_pos = pos;
  auto result = std::forward<F>(parse_fn_)();
  if (!result) {
    // Failed speculative parse - restore position
    pos = saved_pos;
  }
  return result;
}

void Parser::Impl::skip_whitespace_and_comments() {
  while (true) {
    char const current = peek();

    // Skip whitespace
    if (std::isspace(static_cast<unsigned char>(current)) != 0) {
      advance();
      continue;
    }

    // Skip line comments (//)
    if (current == '/' && peek(1) == '/') {
      advance(2);  // consume //
      // Skip until newline or EOF
      while (peek() != '\n' && peek() != k_eof_char) {
        advance();
      }
      continue;
    }

    // Skip block comments (/* ... */)
    if (current == '/' && peek(1) == '*') {
      advance(2);  // consume /*

      // Track nesting level for nested block comments
      int nesting = 1;
      while (nesting > 0 && peek() != k_eof_char) {
        if (peek() == '/' && peek(1) == '*') {
          advance(2);
          ++nesting;
        } else if (peek() == '*' && peek(1) == '/') {
          advance(2);
          --nesting;
        } else {
          advance();
        }
      }

      if (nesting > 0) {
        error("Unterminated block comment");
      }
      continue;
    }

    // No more whitespace or comments
    break;
  }
}

Source_Position Parser::Impl::current_position() const {
  return diagnostics->offset_to_position(pos);
}

Source_Range Parser::Impl::make_range(Source_Position start_) const {
  return Source_Range{.start = start_, .end = current_position()};
}

void Parser::Impl::error(std::string message_, Source_Range range_) const {
  diagnostics->add_error(range_, std::move(message_));
}

void Parser::Impl::error(std::string message_) const {
  auto const p = current_position();
  error(std::move(message_), Source_Range{.start = p, .end = p});
}

bool Parser::Impl::expect(char ch_) {
  skip_whitespace_and_comments();

  if (peek() != ch_) {
    error(std::format("Expected '{}', found '{}'", ch_, peek()));
    return false;
  }

  advance();
  return true;
}

bool Parser::Impl::expect(std::string_view str_) {
  skip_whitespace_and_comments();

  for (std::size_t i = 0; i < str_.size(); ++i) {
    if (peek(i) != str_[i]) {
      error(std::format("Expected '{}'", str_));
      return false;
    }
  }

  advance(str_.size());
  return true;
}

bool Parser::Impl::match_keyword(std::string_view keyword_) {
  skip_whitespace_and_comments();

  // Check if keyword matches
  if (!lookahead(keyword_)) {
    return false;
  }

  // Ensure it's followed by a non-identifier character
  char const next = peek(keyword_.size());
  if (is_identifier_continue(next)) {
    return false;
  }

  // Consume the keyword
  advance(keyword_.size());

  return true;
}

bool Parser::Impl::match_operator(std::string_view op_) {
  skip_whitespace_and_comments();

  // Check if operator matches
  if (!lookahead(op_)) {
    return false;
  }

  // Consume the operator
  advance(op_.size());

  return true;
}

bool Parser::Impl::lookahead(std::string_view str_) const {
  for (std::size_t i = 0; i < str_.size(); ++i) {
    if (peek(i) != str_[i]) {
      return false;
    }
  }
  return true;
}

std::optional<ast::Statement> Parser::Impl::try_parse_expr_as_statement(Parser* parser_) {
  auto expr_result = parser_->parse_expr();
  if (!expr_result) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Check if this is a while expression - can be used as statement without semicolon
  if (auto* while_fwd = std::get_if<std::shared_ptr<ast::While_Expr>>(&*expr_result)) {
    ast::While_Statement while_stmt;
    while_stmt.expr = *while_fwd;
    return ast::Statement{std::make_shared<ast::While_Statement>(std::move(while_stmt))};
  }

  // Check if this is a for expression - can be used as statement without semicolon
  if (auto* for_fwd = std::get_if<std::shared_ptr<ast::For_Expr>>(&*expr_result)) {
    ast::For_Statement for_stmt;
    for_stmt.expr = *for_fwd;
    return ast::Statement{std::make_shared<ast::For_Statement>(std::move(for_stmt))};
  }

  // Check if this is an if expression - can be used as statement without semicolon
  if (auto* if_fwd = std::get_if<std::shared_ptr<ast::If_Expr>>(&*expr_result)) {
    ast::If_Statement if_stmt;
    if_stmt.expr = *if_fwd;
    return ast::Statement{std::make_shared<ast::If_Statement>(std::move(if_stmt))};
  }

  // Other expressions require semicolon
  skip_whitespace_and_comments();
  if (peek() != ';') {
    // Semicolon missing - return nullopt to restore position (block parser can try as trailing expression)
    return std::nullopt;
  }

  advance();  // consume ';'

  // Check if this is a function call - use Func_Call_Statement
  if (auto* func_call_fwd = std::get_if<std::shared_ptr<ast::Func_Call_Expr>>(&*expr_result)) {
    ast::Func_Call_Statement func_call_stmt;
    func_call_stmt.expr = **func_call_fwd;  // Dereference shared_ptr then copy
    return ast::Statement{std::move(func_call_stmt)};
  }

  // Otherwise use generic Expr_Statement
  ast::Expr_Statement expr_stmt;
  expr_stmt.expr = std::make_shared<ast::Expr>(std::move(*expr_result));
  return ast::Statement{std::make_shared<ast::Expr_Statement>(std::move(expr_stmt))};
}

[[nodiscard]] std::optional<ast::Binary_Op> Parser::Impl::try_parse_binary_op() {
  skip_whitespace_and_comments();

  // Two-character operators first
  if (peek() == '=' && peek(1) == '=') {
    advance(2);
    return ast::Binary_Op::Eq;
  }
  if (peek() == '!' && peek(1) == '=') {
    advance(2);
    return ast::Binary_Op::Ne;
  }
  if (peek() == '<' && peek(1) == '=') {
    advance(2);
    return ast::Binary_Op::Le;
  }
  if (peek() == '>' && peek(1) == '=') {
    advance(2);
    return ast::Binary_Op::Ge;
  }
  if (peek() == '&' && peek(1) == '&') {
    advance(2);
    return ast::Binary_Op::And;
  }
  if (peek() == '|' && peek(1) == '|') {
    advance(2);
    return ast::Binary_Op::Or;
  }
  if (peek() == '<' && peek(1) == '<') {
    advance(2);
    return ast::Binary_Op::Shl;
  }
  if (peek() == '>' && peek(1) == '>') {
    advance(2);
    return ast::Binary_Op::Shr;
  }

  // Single-character operators
  switch (peek()) {
    case '+':
      advance();
      return ast::Binary_Op::Add;
    case '-':
      advance();
      return ast::Binary_Op::Sub;
    case '*':
      advance();
      return ast::Binary_Op::Mul;
    case '/':
      advance();
      return ast::Binary_Op::Div;
    case '%':
      advance();
      return ast::Binary_Op::Mod;
    case '<':
      advance();
      return ast::Binary_Op::Lt;
    case '>':
      advance();
      return ast::Binary_Op::Gt;
    case '&':
      advance();
      return ast::Binary_Op::Bit_And;
    case '|':
      advance();
      return ast::Binary_Op::Bit_Or;
    case '^':
      advance();
      return ast::Binary_Op::Bit_Xor;
    default:
      return std::nullopt;
  }
}

std::optional<ast::Binary_Op> Parser::Impl::try_parse_binary_op_with_min_precedence(int min_precedence_) {
  auto op = try_parse_binary_op();
  if (!op) {
    return std::nullopt;
  }
  // Check precedence - if too low, reject
  if (get_precedence(*op) < min_precedence_) {
    return std::nullopt;
  }
  return op;
}

[[nodiscard]] std::optional<ast::Unary_Op> Parser::Impl::try_parse_unary_op() {
  skip_whitespace_and_comments();

  switch (peek()) {
    case '-':
      advance();
      return ast::Unary_Op::Neg;
    case '+':
      advance();
      return ast::Unary_Op::Pos;
    case '!':
      advance();
      return ast::Unary_Op::Not;
    case '~':
      advance();
      return ast::Unary_Op::BitNot;
    default:
      return std::nullopt;
  }
}

namespace {
// Keywords that cannot be used as identifiers or pattern bindings
constexpr std::array<std::string_view, 17> k_keywords = {
    "fn",
    "struct",
    "enum",
    "trait",
    "impl",
    "type",
    "let",
    "return",
    "break",
    "continue",
    "if",
    "else",
    "while",
    "for",
    "match",
    "in",
    "as"
};
}  // namespace

Parser::Parser(Diagnostic_Engine& diagnostics_) : m_impl(std::make_unique<Impl>()) {
  m_impl->diagnostics = &diagnostics_;
}

Parser::~Parser() = default;

bool Parser::all_input_consumed() const {
  // Save position
  auto saved_pos = m_impl->pos;

  // Skip whitespace and comments (we need to do this without modifying state)
  while (saved_pos < m_impl->diagnostics->source().size()) {
    char const current = m_impl->diagnostics->source()[saved_pos];

    // Skip whitespace
    if (std::isspace(static_cast<unsigned char>(current)) != 0) {
      ++saved_pos;
      continue;
    }

    // Skip line comments (//)
    if (current == '/' && saved_pos + 1 < m_impl->diagnostics->source().size() &&
        m_impl->diagnostics->source()[saved_pos + 1] == '/') {
      saved_pos += 2;
      // Skip until newline or EOF
      while (saved_pos < m_impl->diagnostics->source().size() && m_impl->diagnostics->source()[saved_pos] != '\n') {
        ++saved_pos;
      }
      continue;
    }

    // Skip block comments (/* ... */)
    if (current == '/' && saved_pos + 1 < m_impl->diagnostics->source().size() &&
        m_impl->diagnostics->source()[saved_pos + 1] == '*') {
      saved_pos += 2;
      int nesting = 1;
      while (nesting > 0 && saved_pos < m_impl->diagnostics->source().size()) {
        if (saved_pos + 1 < m_impl->diagnostics->source().size() && m_impl->diagnostics->source()[saved_pos] == '/' &&
            m_impl->diagnostics->source()[saved_pos + 1] == '*') {
          saved_pos += 2;
          ++nesting;
        } else if (saved_pos + 1 < m_impl->diagnostics->source().size() &&
                   m_impl->diagnostics->source()[saved_pos] == '*' &&
                   m_impl->diagnostics->source()[saved_pos + 1] == '/') {
          saved_pos += 2;
          --nesting;
        } else {
          ++saved_pos;
        }
      }
      continue;
    }

    // Found non-whitespace, non-comment character
    return false;
  }

  // Reached EOF - all input consumed
  return true;
}

std::optional<ast::Module> Parser::parse_module() {
  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Import_Statement> imports;
  std::vector<ast::Item> items;

  // Parse import statements
  while (m_impl->pos < m_impl->diagnostics->source().size()) {
    m_impl->skip_whitespace_and_comments();

    if (m_impl->pos >= m_impl->diagnostics->source().size()) {
      break;
    }

    // Check if this is an import statement
    if (!m_impl->lookahead("import")) {
      break;  // No more imports, move to items phase
    }

    auto import_stmt = parse_import_statement();
    if (!import_stmt) {
      if (m_impl->diagnostics->has_errors()) {
        return std::nullopt;
      }
      m_impl->error("Failed to parse import statement", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    imports.push_back(std::move(*import_stmt));
  }

  // Parse items (with optional pub modifier)
  while (m_impl->pos < m_impl->diagnostics->source().size()) {
    m_impl->skip_whitespace_and_comments();

    if (m_impl->pos >= m_impl->diagnostics->source().size()) {
      break;
    }

    auto const start_pos = m_impl->current_position();

    // Check for optional 'pub' modifier
    bool is_pub = false;
    if (m_impl->match_keyword("pub")) {
      is_pub = true;
      m_impl->skip_whitespace_and_comments();
    }

    auto const start_char = m_impl->peek();

    // Module-level items must start with a keyword (fn, struct, enum, impl, trait, type)
    // Reject arbitrary expressions/statements at module level
    if (start_char != k_eof_char && !m_impl->lookahead("fn") && !m_impl->lookahead("struct") &&
        !m_impl->lookahead("enum") && !m_impl->lookahead("impl") && !m_impl->lookahead("trait") &&
        !m_impl->lookahead("type")) {
      m_impl->error(
          "Expected module-level item (fn, struct, enum, impl, trait, or type), found unexpected content",
          m_impl->make_range(start_pos)
      );
      return std::nullopt;
    }

    auto stmt = parse_statement();
    if (!stmt) {
      if (m_impl->diagnostics->has_errors()) {
        return std::nullopt;
      }
      m_impl->error(
          "Expected statement or declaration at module level",
          m_impl->make_range(m_impl->current_position())
      );
      return std::nullopt;
    }

    items.push_back(ast::make_item(is_pub, std::move(*stmt)));
    m_impl->skip_whitespace_and_comments();
  }

  if (m_impl->diagnostics->has_errors()) {
    return std::nullopt;
  }

  return ast::make_module(std::move(imports), std::move(items));
}

std::optional<ast::Import_Statement> Parser::parse_import_statement() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Expect 'import' keyword
  if (!m_impl->match_keyword("import")) {
    m_impl->error("Expected 'import' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Parse module path: Geometry.Shapes.Advanced or just Geometry
  std::vector<std::string> module_path;

  while (true) {
    // Parse type name (module names use Camel_Snake_Case)
    if (!is_identifier_start(m_impl->peek()) || (std::isupper(static_cast<unsigned char>(m_impl->peek())) == 0)) {
      m_impl->error(
          "Expected module name (must start with uppercase letter)",
          m_impl->make_range(m_impl->current_position())
      );
      return std::nullopt;
    }

    std::string segment;
    segment += m_impl->advance();
    while (is_identifier_continue(m_impl->peek())) {
      segment += m_impl->advance();
    }

    module_path.push_back(std::move(segment));

    m_impl->skip_whitespace_and_comments();

    // Check for '.' - either continues path or precedes '{'
    if (m_impl->peek() == '.') {
      m_impl->advance();  // consume '.'
      m_impl->skip_whitespace_and_comments();

      // If next char is '{', we're done with path
      if (m_impl->peek() == '{') {
        break;
      }

      // Otherwise, continue parsing path segments
      continue;
    }

    // No dot found - error, we need either another segment or .{items}
    m_impl->error("Expected '.' in import statement", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Expect '{'
  if (m_impl->peek() != '{') {
    m_impl->error("Expected '{' after module path in import statement", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }
  m_impl->advance();  // consume '{'

  m_impl->skip_whitespace_and_comments();

  // Parse import item list: {Item1, Item2 as Alias, item3}
  std::vector<ast::Import_Item> items;

  while (true) {
    if (m_impl->peek() == '}') {
      break;  // Empty list or end of list
    }

    // Parse identifier (can be type name or function name)
    if (!is_identifier_start(m_impl->peek())) {
      m_impl->error("Expected identifier in import list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    std::string item_name;
    item_name += m_impl->advance();
    while (is_identifier_continue(m_impl->peek())) {
      item_name += m_impl->advance();
    }

    m_impl->skip_whitespace_and_comments();

    // Check for optional 'as' alias
    std::optional<std::string> alias;
    if (m_impl->match_keyword("as")) {
      m_impl->skip_whitespace_and_comments();

      // Parse alias identifier
      if (!is_identifier_start(m_impl->peek())) {
        m_impl->error("Expected identifier after 'as'", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      std::string alias_name;
      alias_name += m_impl->advance();
      while (is_identifier_continue(m_impl->peek())) {
        alias_name += m_impl->advance();
      }

      alias = std::move(alias_name);
      m_impl->skip_whitespace_and_comments();
    }

    items.push_back(ast::make_import_item(std::move(item_name), std::move(alias)));

    m_impl->skip_whitespace_and_comments();

    // Check for ',' to continue or '}' to end
    if (m_impl->peek() == ',') {
      m_impl->advance();  // consume ','
      m_impl->skip_whitespace_and_comments();
      continue;
    }

    if (m_impl->peek() == '}') {
      break;
    }

    m_impl->error("Expected ',' or '}' in import list", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  // Expect '}'
  if (m_impl->peek() != '}') {
    m_impl->error("Expected '}' to close import list", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }
  m_impl->advance();  // consume '}'

  m_impl->skip_whitespace_and_comments();

  // Expect ';'
  if (m_impl->peek() != ';') {
    m_impl->error("Expected ';' after import statement", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }
  m_impl->advance();  // consume ';'

  return ast::make_import_statement(std::move(module_path), std::move(items));
}

std::optional<ast::Integer> Parser::parse_integer() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();
  std::string value;
  std::optional<std::string> suffix;

  // Check for hexadecimal literal (0x prefix)
  if (m_impl->peek() == '0' && (m_impl->peek(1) == 'x' || m_impl->peek(1) == 'X')) {
    m_impl->advance(2);  // consume '0x' or '0X'

    // Must have at least one hex digit after 0x
    if (std::isxdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
      m_impl->error("Invalid hexadecimal literal: expected hex digit after '0x'", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    // Collect hex digits and underscores
    value = "0x";
    char const last_char =
        m_impl->collect_digits(value, [](char ch_) { return std::isxdigit(static_cast<unsigned char>(ch_)) != 0; });

    // Check for trailing underscore
    if (last_char == '_') {
      m_impl->error("Invalid hexadecimal literal: trailing underscore not allowed", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }
  // Check for octal literal (0o prefix)
  else if (m_impl->peek() == '0' && (m_impl->peek(1) == 'o' || m_impl->peek(1) == 'O')) {
    m_impl->advance(2);  // consume '0o' or '0O'

    // Must have at least one octal digit after 0o
    if (m_impl->peek() < '0' || m_impl->peek() > '7') {
      m_impl->error("Invalid octal literal: expected octal digit after '0o'", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    // Collect octal digits and underscores
    value = "0o";
    char const last_char = m_impl->collect_digits(value, [](char ch_) { return ch_ >= '0' && ch_ <= '7'; });

    // Check for trailing underscore
    if (last_char == '_') {
      m_impl->error("Invalid octal literal: trailing underscore not allowed", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }
  // Check for binary literal (0b prefix)
  else if (m_impl->peek() == '0' && (m_impl->peek(1) == 'b' || m_impl->peek(1) == 'B')) {
    m_impl->advance(2);  // consume '0b' or '0B'

    // Must have at least one binary digit after 0b
    if (m_impl->peek() != '0' && m_impl->peek() != '1') {
      m_impl->error("Invalid binary literal: expected binary digit after '0b'", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    // Collect binary digits and underscores
    value = "0b";
    char const last_char = m_impl->collect_digits(value, [](char ch_) { return ch_ == '0' || ch_ == '1'; });

    // Check for trailing underscore
    if (last_char == '_') {
      m_impl->error("Invalid binary literal: trailing underscore not allowed", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }
  // Check for leading zero (only "0" is allowed, not "01", "02", etc.)
  else if (m_impl->peek() == '0') {
    if (std::isdigit(static_cast<unsigned char>(m_impl->peek(1))) != 0 || m_impl->peek(1) == '_') {
      m_impl->error("Invalid integer: leading zero not allowed (except standalone '0')", m_impl->make_range(start_pos));
      return std::nullopt;
    }
    value += m_impl->advance();
  } else if (m_impl->peek() >= '1' && m_impl->peek() <= '9') {
    // Non-zero start - collect all digits and underscores
    char const last_char =
        m_impl->collect_digits(value, [](char ch_) { return std::isdigit(static_cast<unsigned char>(ch_)) != 0; });
    // Check for trailing underscore
    if (last_char == '_') {
      m_impl->error("Invalid integer: trailing underscore not allowed", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  } else {
    m_impl->error("Expected integer literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Check for zero with trailing underscore (like "0_")
  if (value == "0" && m_impl->peek() == '_') {
    m_impl->advance();  // consume the '_'
    m_impl->error("Invalid integer: trailing underscore not allowed", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Check for optional type suffix (I8, I16, I32, I64, U8, U16, U32, U64)
  if (m_impl->peek() == 'I' || m_impl->peek() == 'U') {
    suffix = std::string(1, m_impl->advance());
    if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
      m_impl->error("Expected digit after type suffix", m_impl->make_range(start_pos));
      return std::nullopt;
    }
    while (std::isdigit(static_cast<unsigned char>(m_impl->peek())) != 0) {
      *suffix += m_impl->advance();
    }
  }

  // Create AST node
  ast::Integer result;
  result.value = std::move(value);
  if (suffix) {
    result.suffix = *suffix;
  }

  return result;
}

std::optional<ast::Float> Parser::parse_float() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();
  std::string value;
  std::optional<std::string> suffix;

  // Check for special float literals: nan, inf (case-insensitive)
  if (m_impl->lookahead("nan") || m_impl->lookahead("NaN") || m_impl->lookahead("NAN") || m_impl->lookahead("Nan")) {
    value = "nan";
    m_impl->advance(3);  // consume 'nan'

    // Check for optional type suffix (F32, F64)
    if (m_impl->peek() == 'F') {
      suffix = std::string(1, m_impl->advance());
      if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
        m_impl->error("Expected digit after type suffix", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      while (std::isdigit(static_cast<unsigned char>(m_impl->peek())) != 0) {
        *suffix += m_impl->advance();
      }
    }

    ast::Float result;
    result.value = std::move(value);
    if (suffix) {
      result.suffix = *suffix;
    }
    return result;
  }

  if (m_impl->lookahead("inf") || m_impl->lookahead("Inf") || m_impl->lookahead("INF")) {
    value = "inf";
    m_impl->advance(3);  // consume 'inf'

    // Check for optional type suffix (F32, F64)
    if (m_impl->peek() == 'F') {
      suffix = std::string(1, m_impl->advance());
      if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
        m_impl->error("Expected digit after type suffix", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      while (std::isdigit(static_cast<unsigned char>(m_impl->peek())) != 0) {
        *suffix += m_impl->advance();
      }
    }

    ast::Float result;
    result.value = std::move(value);
    if (suffix) {
      result.suffix = *suffix;
    }
    return result;
  }

  // Float requires digits before or after dot (or both)
  // Also supports scientific notation (e/E)

  // Collect digits before dot (if any)
  char const last_char_before_dot =
      m_impl->collect_digits(value, [](char ch_) { return std::isdigit(static_cast<unsigned char>(ch_)) != 0; });

  // Must have dot or exponent
  bool has_dot = false;
  bool has_exponent = false;

  if (m_impl->peek() == '.') {
    // Check for trailing underscore before dot
    if (last_char_before_dot == '_') {
      m_impl->error("Invalid float: underscore before decimal point", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    has_dot = true;
    value += m_impl->advance();  // consume '.'

    // Collect fractional digits
    char const last_char_after_dot =
        m_impl->collect_digits(value, [](char ch_) { return std::isdigit(static_cast<unsigned char>(ch_)) != 0; });
    // Check for trailing underscore after fractional part
    if (last_char_after_dot == '_') {
      m_impl->error("Invalid float: trailing underscore after decimal", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }

  // Check for exponent (e or E)
  if (m_impl->peek() == 'e' || m_impl->peek() == 'E') {
    // Check for trailing underscore before exponent (handled above)
    // (This check is redundant now but kept for clarity)

    has_exponent = true;
    value += m_impl->advance();  // consume 'e' or 'E'

    // Optional sign
    if (m_impl->peek() == '+' || m_impl->peek() == '-') {
      value += m_impl->advance();
    }

    // Check for leading underscore after e/E or sign
    if (m_impl->peek() == '_') {
      m_impl->error("Invalid float: underscore after exponent marker", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    // Collect exponent digits
    if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
      m_impl->error("Expected digits after exponent", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    char const last_char_in_exponent =
        m_impl->collect_digits(value, [](char ch_) { return std::isdigit(static_cast<unsigned char>(ch_)) != 0; });
    // Check for trailing underscore in exponent
    if (last_char_in_exponent == '_') {
      m_impl->error("Invalid float: trailing underscore in exponent", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }

  // Must have at least dot or exponent to be a float
  if (!has_dot && !has_exponent) {
    m_impl->error("Expected float literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Check for optional type suffix (F32, F64)
  if (m_impl->peek() == 'F') {
    suffix = std::string(1, m_impl->advance());
    if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
      m_impl->error("Expected digit after type suffix", m_impl->make_range(start_pos));
      return std::nullopt;
    }
    while (std::isdigit(static_cast<unsigned char>(m_impl->peek())) != 0) {
      *suffix += m_impl->advance();
    }
  }

  // Create AST node
  ast::Float result;
  result.value = std::move(value);
  if (suffix) {
    result.suffix = *suffix;
  }

  return result;
}

std::optional<ast::String> Parser::parse_string() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() != '"') {
    m_impl->error("Expected string literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::string value;
  value += m_impl->advance();  // consume opening quote

  while (m_impl->peek() != '"' && m_impl->peek() != k_eof_char) {
    if (m_impl->peek() == '\\') {
      // Escape sequence
      value += m_impl->advance();  // consume backslash
      if (m_impl->peek() == k_eof_char) {
        m_impl->error("Unterminated string literal", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      value += m_impl->advance();  // consume escaped character
    } else {
      value += m_impl->advance();
    }
  }

  if (m_impl->peek() != '"') {
    m_impl->error("Unterminated string literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  value += m_impl->advance();  // consume closing quote

  // Create AST node (stores with quotes)
  ast::String result;
  result.value = std::move(value);

  return result;
}

std::optional<ast::String_Interpolation> Parser::parse_string_interpolation() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() != '"') {
    m_impl->error("Expected string interpolation", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->advance();  // consume opening quote

  std::vector<ast::String_Interp_Part> parts;
  std::string current_literal;

  while (m_impl->peek() != '"' && m_impl->peek() != k_eof_char) {
    if (m_impl->peek() == '\\') {
      // Escape sequence
      current_literal += m_impl->advance();  // consume backslash
      if (m_impl->peek() == k_eof_char) {
        m_impl->error("Unterminated string interpolation", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      current_literal += m_impl->advance();  // consume escaped character
    } else if (m_impl->peek() == '{') {
      // Start of interpolated expression
      // Push any accumulated literal
      if (!current_literal.empty()) {
        parts.emplace_back(current_literal);
        current_literal.clear();
      }

      m_impl->advance();  // consume '{'
      m_impl->skip_whitespace_and_comments();

      // Parse expression inside {}
      auto expr = parse_expr();
      if (!expr) {
        m_impl->error("Expected expression in string interpolation", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      m_impl->skip_whitespace_and_comments();

      if (m_impl->peek() != '}') {
        m_impl->error("Expected '}' to close interpolated expression", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      m_impl->advance();  // consume '}'

      // Add expression to parts
      parts.emplace_back(std::make_shared<ast::Expr>(std::move(*expr)));
    } else {
      current_literal += m_impl->advance();
    }
  }

  if (m_impl->peek() != '"') {
    m_impl->error("Unterminated string interpolation", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->advance();  // consume closing quote

  // Push any remaining literal
  if (!current_literal.empty()) {
    parts.emplace_back(current_literal);
  }

  // Create AST node
  ast::String_Interpolation result;
  result.parts = std::move(parts);

  return result;
}

std::optional<ast::String> Parser::parse_raw_string() {
  m_impl->skip_whitespace_and_comments();
  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() != 'r') {
    return std::nullopt;
  }

  m_impl->advance();  // consume 'r'

  // Count delimiter '#' symbols
  size_t delimiter_count = 0;
  while (m_impl->peek() == '#') {
    ++delimiter_count;
    m_impl->advance();
  }

  if (m_impl->peek() != '"') {
    m_impl->error("Expected '\"' after raw string prefix", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->advance();  // consume opening quote

  // Build raw string content (no escape processing)
  std::string value = "r";
  for (size_t i = 0; i < delimiter_count; ++i) {
    value += '#';
  }
  value += '"';

  // Parse until we find closing delimiter
  while (!m_impl->is_at_end()) {
    if (m_impl->peek() == '"') {
      // Check if we have matching delimiter
      size_t matched_delimiters = 0;
      size_t look_ahead = 1;

      while (matched_delimiters < delimiter_count && m_impl->peek(look_ahead) == '#') {
        ++matched_delimiters;
        ++look_ahead;
      }

      if (matched_delimiters == delimiter_count) {
        // Found complete closing delimiter
        value += m_impl->advance();  // closing quote
        for (size_t i = 0; i < delimiter_count; ++i) {
          value += m_impl->advance();  // closing '#' symbols
        }

        ast::String result;
        result.value = std::move(value);
        return result;
      }
    }

    // Not the closing delimiter, just part of content
    value += m_impl->advance();
  }

  m_impl->error("Unterminated raw string literal", m_impl->make_range(start_pos));
  return std::nullopt;
}

std::optional<ast::Char> Parser::parse_char() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() != '\'') {
    m_impl->error("Expected character literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::string value;
  value += m_impl->advance();  // consume opening quote

  if (m_impl->peek() == '\'') {
    m_impl->error("Empty character literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  if (m_impl->peek() == '\\') {
    // Escape sequence
    value += m_impl->advance();  // consume backslash
    if (m_impl->peek() == k_eof_char) {
      m_impl->error("Unterminated character literal", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    char const escape_char = m_impl->peek();
    value += m_impl->advance();  // consume escape type char

    // Handle multi-character escapes
    if (escape_char == 'x') {
      // Hex escape: \xHH (exactly 2 hex digits)
      for (int i = 0; i < 2; i++) {
        if (m_impl->peek() == k_eof_char || std::isxdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
          m_impl->error("Invalid hex escape sequence (expected 2 hex digits)", m_impl->make_range(start_pos));
          return std::nullopt;
        }
        value += m_impl->advance();
      }
    } else if (escape_char == 'u') {
      // Unicode escape: \u{HHHHHH} (1-6 hex digits in braces)
      if (m_impl->peek() != '{') {
        m_impl->error("Invalid unicode escape (expected '{')", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      value += m_impl->advance();  // consume '{'

      int digit_count = 0;
      while (m_impl->peek() != '}' && digit_count < 6) {
        if (std::isxdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
          m_impl->error("Invalid unicode escape (expected hex digit or '}')", m_impl->make_range(start_pos));
          return std::nullopt;
        }
        value += m_impl->advance();
        digit_count++;
      }

      if (digit_count == 0) {
        m_impl->error("Invalid unicode escape (expected at least 1 hex digit)", m_impl->make_range(start_pos));
        return std::nullopt;
      }

      if (m_impl->peek() != '}') {
        m_impl->error("Invalid unicode escape (expected '}')", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      value += m_impl->advance();  // consume '}'
    }
    // For simple escapes like \n, \t, \', \", \\, we've already consumed the char
  } else {
    // Regular character (may be multi-byte UTF-8)
    char const first_byte = m_impl->peek();
    value += m_impl->advance();

    // UTF-8 continuation bytes start with 10xxxxxx (0x80-0xBF)
    // Detect multi-byte UTF-8 by checking if first byte has high bit set
    if ((static_cast<unsigned char>(first_byte) & 0x80U) != 0) {
      // This is a multi-byte UTF-8 sequence
      // Count continuation bytes based on leading byte
      int continuation_bytes = 0;
      if ((static_cast<unsigned char>(first_byte) & 0xE0U) == 0xC0U) {
        continuation_bytes = 1;  // 2-byte sequence (110xxxxx)
      } else if ((static_cast<unsigned char>(first_byte) & 0xF0U) == 0xE0U) {
        continuation_bytes = 2;  // 3-byte sequence (1110xxxx)
      } else if ((static_cast<unsigned char>(first_byte) & 0xF8U) == 0xF0U) {
        continuation_bytes = 3;  // 4-byte sequence (11110xxx)
      }

      for (int i = 0; i < continuation_bytes; i++) {
        if (m_impl->peek() == k_eof_char) {
          m_impl->error("Invalid UTF-8 sequence in character literal", m_impl->make_range(start_pos));
          return std::nullopt;
        }
        value += m_impl->advance();
      }
    }
  }

  if (m_impl->peek() != '\'') {
    m_impl->error("Unterminated character literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  value += m_impl->advance();  // consume closing quote

  // Create AST node (stores with quotes)
  ast::Char result;
  result.value = std::move(value);

  return result;
}

std::optional<ast::Bool_Literal> Parser::parse_bool_literal() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Try to match "true"
  if (m_impl->lookahead("true")) {
    // Make sure it's not part of a longer identifier (e.g., "true_value")
    if (is_identifier_continue(m_impl->peek(4))) {
      m_impl->error("Expected boolean literal 'true' or 'false'", m_impl->make_range(start_pos));
      return std::nullopt;
    }
    m_impl->advance(4);  // Consume 'true'
    return ast::Bool_Literal{true};
  }

  // Try to match "false"
  if (m_impl->lookahead("false")) {
    // Make sure it's not part of a longer identifier (e.g., "false_value")
    if (is_identifier_continue(m_impl->peek(5))) {
      m_impl->error("Expected boolean literal 'true' or 'false'", m_impl->make_range(start_pos));
      return std::nullopt;
    }
    m_impl->advance(5);  // Consume 'false'
    return ast::Bool_Literal{false};
  }

  m_impl->error("Expected boolean literal 'true' or 'false'", m_impl->make_range(start_pos));
  return std::nullopt;
}

std::optional<ast::Unit_Literal> Parser::parse_unit_literal() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() != '(' || m_impl->peek(1) != ')') {
    m_impl->error("Expected unit literal '()'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->advance(2);  // consume '()'

  return ast::Unit_Literal{};
}

std::optional<ast::Struct_Literal> Parser::parse_struct_literal() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Parse type name (must start with uppercase)
  if (!is_identifier_start(m_impl->peek()) || std::isupper(static_cast<unsigned char>(m_impl->peek())) == 0) {
    m_impl->error("Expected type name for struct literal", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::string type_name;
  type_name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    type_name += m_impl->advance();
  }

  m_impl->skip_whitespace_and_comments();

  // Expect '{'
  if (!m_impl->expect('{')) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Field_Initializer> fields;

  // Parse field initializers
  // Empty struct literal is valid: Point {}
  if (m_impl->peek() != '}') {
    while (true) {
      m_impl->skip_whitespace_and_comments();

      // Parse field name
      if (!is_identifier_start(m_impl->peek())) {
        m_impl->error("Expected field name", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      std::string field_name;
      field_name += m_impl->advance();
      while (is_identifier_continue(m_impl->peek())) {
        field_name += m_impl->advance();
      }

      m_impl->skip_whitespace_and_comments();

      // Expect ':'
      if (!m_impl->expect(':')) {
        return std::nullopt;
      }

      m_impl->skip_whitespace_and_comments();

      // Parse field value expression
      auto value = parse_expr();
      if (!value) {
        m_impl->error("Expected expression for field value", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      ast::Field_Initializer field;
      field.name = std::move(field_name);
      field.value = std::make_shared<ast::Expr>(std::move(*value));
      fields.push_back(std::move(field));

      m_impl->skip_whitespace_and_comments();

      // Check for comma or end
      if (m_impl->peek() == ',') {
        m_impl->advance();  // consume ','
        m_impl->skip_whitespace_and_comments();
        // Allow trailing comma
        if (m_impl->peek() == '}') {
          break;
        }
        continue;
      }

      // No comma, must be end of fields
      break;
    }
  }

  m_impl->skip_whitespace_and_comments();

  // Expect '}'
  if (!m_impl->expect('}')) {
    return std::nullopt;
  }

  ast::Struct_Literal result;
  result.type_name = std::move(type_name);
  result.fields = std::move(fields);

  return result;
}

std::optional<ast::Array_Literal> Parser::parse_array_literal() {
  m_impl->skip_whitespace_and_comments();

  // Expect '['
  if (!m_impl->expect('[')) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Expr> elements;

  // Parse array elements
  // Empty array literal is valid: []
  if (m_impl->peek() != ']') {
    while (true) {
      m_impl->skip_whitespace_and_comments();

      // Parse element expression
      auto element = parse_expr();
      if (!element) {
        m_impl->error("Expected expression in array literal", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      elements.push_back(std::move(*element));

      m_impl->skip_whitespace_and_comments();

      // Check for comma or end
      if (m_impl->peek() == ',') {
        m_impl->advance();  // consume ','
        m_impl->skip_whitespace_and_comments();
        // Allow trailing comma
        if (m_impl->peek() == ']') {
          break;
        }
        continue;
      }

      // No comma, must be end of elements
      break;
    }
  }

  m_impl->skip_whitespace_and_comments();

  // Expect ']'
  if (!m_impl->expect(']')) {
    return std::nullopt;
  }

  ast::Array_Literal result;
  result.elements = std::move(elements);

  return result;
}

std::optional<ast::Var_Name> Parser::parse_variable_name() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();
  std::vector<ast::Var_Name_Segment> segments;

  // Parse first segment
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected identifier", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::string name;
  name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    name += m_impl->advance();
  }

  // Check if it's a keyword (keywords can't be used as variable names)
  for (auto const& kw: k_keywords) {
    if (name == kw) {
      m_impl->error(std::format("Cannot use keyword '{}' as variable name", name), m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }

  // For variable names in expressions, NO type parameters
  // Type parameters only appear in qualified names (function calls)
  ast::Var_Name_Segment segment;
  segment.value = std::move(name);
  // segment.type_params is empty
  segments.push_back(std::move(segment));

  // For variable names in expressions, parse only single segment
  // Multi-segment paths are handled by parse_qualified_variable_name() for function calls
  // Field access (obj.field) is handled by parse_postfix_expr()

  ast::Var_Name var_name;
  var_name.segments = std::move(segments);
  return var_name;
}

[[nodiscard]] std::optional<ast::Var_Name> Parser::parse_qualified_variable_name() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();
  std::vector<ast::Var_Name_Segment> segments;

  // Parse first segment (same as parse_variable_name, but then continue for more segments)
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected identifier", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::string name;
  name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    name += m_impl->advance();
  }

  // Check for type parameters after first segment
  std::vector<ast::Type_Name> type_params;
  m_impl->skip_whitespace_and_comments();
  if (m_impl->peek() == '<') {
    m_impl->advance();  // consume '<'

    // Parse type parameters
    while (true) {
      m_impl->skip_whitespace_and_comments();
      auto type_param = parse_type_name();
      if (!type_param) {
        m_impl->error("Expected type parameter", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      type_params.push_back(std::move(*type_param));

      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == '>') {
        m_impl->advance();  // consume '>'
        break;
      }
      if (m_impl->peek() == ',') {
        m_impl->advance();  // consume ','
        continue;
      }
      m_impl->error("Expected ',' or '>' in type parameters", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }

  ast::Var_Name_Segment segment;
  segment.value = std::move(name);
  segment.type_params = std::move(type_params);
  segments.push_back(std::move(segment));

  // Parse additional path segments (Std.IO.println)
  while (true) {
    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() != '.' || m_impl->peek(1) == '.') {  // Stop at '..' (range operator)
      break;
    }
    m_impl->advance();  // consume '.'

    m_impl->skip_whitespace_and_comments();
    if (!is_identifier_start(m_impl->peek())) {
      m_impl->error("Expected identifier after '.'", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    std::string segment_name;
    segment_name += m_impl->advance();
    while (is_identifier_continue(m_impl->peek())) {
      segment_name += m_impl->advance();
    }

    // Type parameters for path segments
    std::vector<ast::Type_Name> segment_type_params;
    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == '<') {
      m_impl->advance();  // consume '<'

      while (true) {
        m_impl->skip_whitespace_and_comments();
        auto type_param = parse_type_name();
        if (!type_param) {
          m_impl->error("Expected type parameter", m_impl->make_range(start_pos));
          return std::nullopt;
        }
        segment_type_params.push_back(std::move(*type_param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == '>') {
          m_impl->advance();  // consume '>'
          break;
        }
        if (m_impl->peek() == ',') {
          m_impl->advance();  // consume ','
          continue;
        }
        m_impl->error("Expected ',' or '>' in type parameters", m_impl->make_range(start_pos));
        return std::nullopt;
      }
    }

    ast::Var_Name_Segment path_segment;
    path_segment.value = std::move(segment_name);
    path_segment.type_params = std::move(segment_type_params);
    segments.push_back(std::move(path_segment));
  }

  ast::Var_Name result;
  result.segments = std::move(segments);
  return result;
}

std::optional<ast::Type_Name> Parser::parse_type_name() {
  m_impl->skip_whitespace_and_comments();

  // Type_Name is a variant of Path_Type, Function_Type, Array_Type, and Tuple_Type
  // Try array type first (starts with "[")
  if (m_impl->peek() == '[') {
    auto array_type = parse_array_type();
    if (array_type) {
      return ast::Type_Name{std::move(*array_type)};
    }
    return std::nullopt;
  }

  // Try tuple type or unit type (starts with "(")
  // Need to distinguish: () = unit type, (T) = parenthesized type, (T,) or (T, U) = tuple type
  if (m_impl->peek() == '(') {
    auto const start_pos = m_impl->current_position();

    // Check for unit type () first
    if (m_impl->peek(1) == ')') {
      // Parse as unit type (handled by parse_path_type)
      auto path_type = parse_path_type();
      if (path_type) {
        return ast::Type_Name{std::move(*path_type)};
      }
      return std::nullopt;
    }

    // Could be tuple type or parenthesized type
    m_impl->advance();  // consume '('

    // Parse first type
    auto first_type = parse_type_name();
    if (!first_type) {
      m_impl->error("Expected type in parentheses", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    m_impl->skip_whitespace_and_comments();

    // Check what follows
    if (m_impl->peek() == ',') {
      // Tuple type: (T, ...)
      std::vector<ast::Type_Name> element_types;
      element_types.push_back(std::move(*first_type));

      while (m_impl->peek() == ',') {
        m_impl->advance();  // consume ','
        m_impl->skip_whitespace_and_comments();

        // Allow trailing comma before ')'
        if (m_impl->peek() == ')') {
          break;
        }

        auto elem_type = parse_type_name();
        if (!elem_type) {
          m_impl->error("Expected type in tuple type", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        element_types.push_back(std::move(*elem_type));
        m_impl->skip_whitespace_and_comments();
      }

      if (!m_impl->expect(')')) {
        return std::nullopt;
      }

      return ast::Type_Name{ast::make_tuple_type(std::move(element_types))};
    }
    if (m_impl->peek() == ')') {
      // Parenthesized type: (T) - just return T
      m_impl->advance();  // consume ')'
      return first_type;
    }
    m_impl->error("Expected ',' or ')' after type in parentheses", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  // Could be function type "fn(...)" or path type starting with something like "Fn"
  // Try parsing as function type
  if (auto func_type = m_impl->try_parse([this] { return parse_function_type(); }); func_type) {
    return ast::Type_Name{std::move(*func_type)};
  }

  // Parse as path type
  if (auto path_type = parse_path_type(); path_type) {
    return ast::Type_Name{std::move(*path_type)};
  }

  return std::nullopt;
}

std::optional<ast::Path_Type> Parser::parse_path_type() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Special case: unit type ()
  if (m_impl->peek() == '(' && m_impl->peek(1) == ')') {
    m_impl->advance(2);  // consume '()'

    ast::Type_Name_Segment segment;
    segment.value = "()";

    ast::Path_Type result;
    result.segments.push_back(std::move(segment));
    return result;
  }

  // Parse first segment
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected type name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::vector<ast::Type_Name_Segment> segments;

  // Parse segments separated by '.'
  while (true) {
    m_impl->skip_whitespace_and_comments();

    if (!is_identifier_start(m_impl->peek())) {
      if (segments.empty()) {
        m_impl->error("Expected type name", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      break;
    }

    std::string name;
    name += m_impl->advance();
    while (is_identifier_continue(m_impl->peek())) {
      name += m_impl->advance();
    }

    // Parse optional type parameters <T, U>
    std::vector<ast::Type_Name> type_params;
    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == '<') {
      m_impl->advance();  // consume '<'

      while (true) {
        m_impl->skip_whitespace_and_comments();
        auto type_param = parse_type_name();
        if (!type_param) {
          m_impl->error("Expected type parameter", m_impl->make_range(start_pos));
          return std::nullopt;
        }
        type_params.push_back(std::move(*type_param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == '>') {
          m_impl->advance();  // consume '>'
          break;
        }
        if (m_impl->peek() == ',') {
          m_impl->advance();  // consume ','
          continue;
        }
        m_impl->error("Expected ',' or '>' in type parameters", m_impl->make_range(start_pos));
        return std::nullopt;
      }
    }

    ast::Type_Name_Segment segment;
    segment.value = std::move(name);
    segment.type_params = std::move(type_params);
    segments.push_back(std::move(segment));

    // Check for '.' to continue path
    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == '.' && m_impl->peek(1) != '.') {
      m_impl->advance();  // consume '.'
      continue;
    }
    break;
  }

  if (segments.empty()) {
    m_impl->error("Expected type name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  ast::Path_Type result;
  result.segments = std::move(segments);
  return result;
}

std::optional<ast::Function_Type> Parser::parse_function_type() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("fn")) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect('(')) {
    return std::nullopt;
  }

  // Parse parameter types
  std::vector<ast::Type_Name> param_types;
  m_impl->skip_whitespace_and_comments();
  if (m_impl->peek() != ')') {
    while (true) {
      m_impl->skip_whitespace_and_comments();
      auto param_type = parse_type_name();
      if (!param_type) {
        m_impl->error("Expected parameter type", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      param_types.push_back(std::move(*param_type));

      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == ')') {
        break;
      }
      if (m_impl->peek() == ',') {
        m_impl->advance();  // consume ','
        continue;
      }
      m_impl->error("Expected ',' or ')' in function type", m_impl->make_range(start_pos));
      return std::nullopt;
    }
  }

  if (!m_impl->expect(')')) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(':')) {
    return std::nullopt;
  }

  // Parse return type
  m_impl->skip_whitespace_and_comments();
  auto return_type = parse_type_name();
  if (!return_type) {
    m_impl->error("Expected return type", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  ast::Function_Type result;

  for (auto& param: param_types) {
    result.param_types.push_back(std::make_shared<ast::Type_Name>(std::move(param)));
  }

  result.return_type = std::make_shared<ast::Type_Name>(std::move(*return_type));

  return result;
}

std::optional<ast::Array_Type> Parser::parse_array_type() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->expect('[')) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Parse element type
  auto element_type = parse_type_name();
  if (!element_type) {
    m_impl->error("Expected element type in array type", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Check for optional size: [T; N] vs [T]
  std::optional<std::string> size;
  if (m_impl->expect(';')) {
    // Sized array: [T; N]
    m_impl->skip_whitespace_and_comments();

    // Parse array size (integer literal)
    if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0) {
      m_impl->error("Expected integer literal for array size", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    std::string size_str;
    while (std::isdigit(static_cast<unsigned char>(m_impl->peek())) != 0) {
      size_str += m_impl->advance();
    }
    size = std::move(size_str);
  }
  // else: unsized array [T]

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(']')) {
    m_impl->error("Expected ']' after array type", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  ast::Array_Type result;
  result.element_type = std::make_shared<ast::Type_Name>(std::move(*element_type));
  result.size = std::move(size);

  return result;
}

std::optional<std::vector<ast::Trait_Bound>> Parser::parse_trait_bounds() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() != ':') {
    return std::vector<ast::Trait_Bound>();  // No bounds
  }

  m_impl->advance();  // consume ':'

  std::vector<ast::Trait_Bound> bounds;

  while (true) {
    m_impl->skip_whitespace_and_comments();
    auto trait_name = parse_type_name();
    if (!trait_name) {
      m_impl->error("Expected trait name", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    ast::Trait_Bound bound;
    bound.trait_name = std::move(*trait_name);
    bounds.push_back(std::move(bound));

    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == '+') {
      m_impl->advance();  // consume '+'
      continue;
    }
    break;
  }

  return bounds;
}

std::optional<ast::Type_Param> Parser::parse_type_param() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Parse type parameter name (simple identifier, not a path)
  auto name = parse_type_name();
  if (!name) {
    m_impl->error("Expected type parameter name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional trait bounds (: Trait1 + Trait2)
  auto bounds = parse_trait_bounds();
  if (!bounds) {
    return std::nullopt;
  }

  ast::Type_Param result;
  result.name = std::move(*name);
  result.bounds = std::move(*bounds);

  return result;
}

std::optional<ast::Where_Clause> Parser::parse_where_clause() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Note: Caller should have already matched 'where' keyword
  // This function parses the predicates only

  std::vector<ast::Where_Predicate> predicates;

  while (true) {
    m_impl->skip_whitespace_and_comments();

    // Parse type being constrained
    auto type_name = parse_type_name();
    if (!type_name) {
      m_impl->error("Expected type name in where clause", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    // Parse trait bounds (: Trait1 + Trait2)
    auto bounds = parse_trait_bounds();
    if (!bounds) {
      return std::nullopt;
    }

    ast::Where_Predicate predicate;
    predicate.type_name = std::move(*type_name);
    predicate.bounds = std::move(*bounds);
    predicates.push_back(std::move(predicate));

    // Check for more predicates
    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == ',') {
      m_impl->advance();  // consume ','
      continue;
    }
    break;
  }

  ast::Where_Clause result;
  result.predicates = std::move(predicates);

  return result;
}

std::optional<ast::Expr> Parser::parse_primary_expr() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Try control flow expressions first (they start with keywords)
  if (m_impl->lookahead("if")) {
    if (auto if_expr = parse_if_expr()) {
      return ast::Expr{std::make_shared<ast::If_Expr>(std::move(*if_expr))};
    }
  }

  if (m_impl->lookahead("while")) {
    if (auto while_expr = parse_while_expr()) {
      return ast::Expr{std::make_shared<ast::While_Expr>(std::move(*while_expr))};
    }
  }

  if (m_impl->lookahead("for")) {
    if (auto for_expr = parse_for_expr()) {
      return ast::Expr{std::make_shared<ast::For_Expr>(std::move(*for_expr))};
    }
  }

  if (m_impl->lookahead("match")) {
    if (auto match_expr = parse_match_expr()) {
      return ast::Expr{std::make_shared<ast::Match_Expr>(std::move(*match_expr))};
    }
  }

  // Try block
  if (m_impl->peek() == '{') {
    if (auto block = parse_block()) {
      return ast::Expr{std::make_shared<ast::Block>(std::move(*block))};
    }
  }

  // Try integer or float
  if (std::isdigit(static_cast<unsigned char>(m_impl->peek())) != 0) {
    // Need to distinguish between integer and float
    // Look ahead for decimal point or exponent
    bool is_float = false;
    for (std::size_t i = 0; m_impl->peek(i) != k_eof_char; ++i) {
      char const ch = m_impl->peek(i);
      if (ch == '.' && m_impl->peek(i + 1) != '.') {
        is_float = true;
        break;
      }
      if (ch == 'e' || ch == 'E') {
        is_float = true;
        break;
      }
      if (std::isdigit(static_cast<unsigned char>(ch)) == 0 && ch != '_') {
        break;
      }
    }

    if (is_float) {
      if (auto float_lit = parse_float()) {
        return ast::Expr{std::move(*float_lit)};
      }
    } else {
      if (auto integer = parse_integer()) {
        return ast::Expr{std::move(*integer)};
      }
    }
  }

  // Try raw string (r"..." or r#"..."#)
  if (m_impl->peek() == 'r' && (m_impl->peek(1) == '"' || m_impl->peek(1) == '#')) {
    if (auto raw_string = parse_raw_string()) {
      return ast::Expr{std::move(*raw_string)};
    }
  }

  // Try string or string interpolation
  if (m_impl->peek() == '"') {
    // Peek ahead to see if there's interpolation: '{' followed by non-'}' before closing '"'
    bool has_interpolation = false;
    std::size_t look_ahead = 1;
    while (m_impl->peek(look_ahead) != k_eof_char) {
      char const ch = m_impl->peek(look_ahead);
      if (ch == '"') {
        break;  // End of string, no interpolation
      }
      if (ch == '\\') {
        look_ahead += 2;  // Skip escape sequence
        continue;
      }
      if (ch == '{' && m_impl->peek(look_ahead + 1) != '}') {
        // Only treat as interpolation if there's something inside {}
        has_interpolation = true;
        break;
      }
      look_ahead++;
    }

    if (has_interpolation) {
      if (auto interp = parse_string_interpolation()) {
        return ast::Expr{std::move(*interp)};
      }
    } else {
      if (auto string = parse_string()) {
        return ast::Expr{std::move(*string)};
      }
    }
  }

  // Try char
  if (m_impl->peek() == '\'') {
    if (auto char_lit = parse_char()) {
      return ast::Expr{std::move(*char_lit)};
    }
  }

  // Try array literal [expr, ...]
  if (m_impl->peek() == '[') {
    if (auto array_lit = parse_array_literal()) {
      return ast::Expr{std::move(*array_lit)};
    }
  }

  // Try boolean literal (true or false)
  // Must check before identifiers to prevent treating them as variable names
  if (m_impl->lookahead("true") && !is_identifier_continue(m_impl->peek(4))) {
    if (auto bool_lit = parse_bool_literal()) {
      return ast::Expr{*bool_lit};
    }
  }
  if (m_impl->lookahead("false") && !is_identifier_continue(m_impl->peek(5))) {
    if (auto bool_lit = parse_bool_literal()) {
      return ast::Expr{*bool_lit};
    }
  }

  // Try special float literals (nan, inf) - must check before identifiers
  // Case-insensitive matching
  // Allow F suffix for type annotation (nanF32, infF64)
  if (m_impl->lookahead("nan") || m_impl->lookahead("NaN") || m_impl->lookahead("NAN") || m_impl->lookahead("Nan")) {
    char const after_nan = m_impl->peek(3);
    // Valid if followed by EOF, whitespace, non-identifier, or 'F' (for suffix)
    if (after_nan == k_eof_char || !is_identifier_continue(after_nan) || after_nan == 'F') {
      if (auto float_lit = parse_float()) {
        return ast::Expr{std::move(*float_lit)};
      }
    }
  }
  if (m_impl->lookahead("inf") || m_impl->lookahead("Inf") || m_impl->lookahead("INF")) {
    char const after_inf = m_impl->peek(3);
    // Valid if followed by EOF, whitespace, non-identifier, or 'F' (for suffix)
    if (after_inf == k_eof_char || !is_identifier_continue(after_inf) || after_inf == 'F') {
      if (auto float_lit = parse_float()) {
        return ast::Expr{std::move(*float_lit)};
      }
    }
  }

  // Try unit literal (), tuple literal (expr, ...), or parenthesized expression (expr)
  if (m_impl->peek() == '(') {
    if (m_impl->peek(1) == ')') {
      // Unit literal: ()
      if (auto unit = parse_unit_literal()) {
        return ast::Expr{*unit};
      }
    } else {
      // Could be tuple literal or parenthesized expression
      m_impl->advance();  // consume '('

      // Parse first expression
      auto first_expr = parse_expr();
      if (!first_expr) {
        m_impl->error("Expected expression", m_impl->make_range(start_pos));
        return std::nullopt;
      }

      m_impl->skip_whitespace_and_comments();

      // Check what follows
      if (m_impl->peek() == ',') {
        // Tuple literal: (expr, ...)
        std::vector<ast::Expr> elements;
        elements.push_back(std::move(*first_expr));

        while (m_impl->peek() == ',') {
          m_impl->advance();  // consume ','
          m_impl->skip_whitespace_and_comments();

          // Allow trailing comma before ')'
          if (m_impl->peek() == ')') {
            break;
          }

          auto elem = parse_expr();
          if (!elem) {
            m_impl->error("Expected expression in tuple literal", m_impl->make_range(m_impl->current_position()));
            return std::nullopt;
          }
          elements.push_back(std::move(*elem));
          m_impl->skip_whitespace_and_comments();
        }

        if (!m_impl->expect(')')) {
          return std::nullopt;
        }

        return ast::Expr{ast::make_tuple_literal(std::move(elements))};
      }
      if (m_impl->peek() == ')') {
        // Parenthesized expression: (expr)
        m_impl->advance();  // consume ')'
        return first_expr;
      }
      m_impl->error(
          "Expected ',' or ')' after expression in parentheses",
          m_impl->make_range(m_impl->current_position())
      );
      return std::nullopt;
    }
  }

  // Try struct literal or variable name / qualified function call
  if (is_identifier_start(m_impl->peek())) {
    // Look ahead to determine:
    //   - TypeName { ... }  → struct literal (uppercase start + brace)
    //   - a.b.c()  → qualified function call (dotted.path(...))
    //   - a.b.c    → field access (parse single segment, postfix handles dots)
    //   - a()      → simple function call (parse single segment)

    // Check if this is a struct literal (uppercase identifier followed by '{')
    bool is_struct_literal = false;
    if (std::isupper(static_cast<unsigned char>(m_impl->peek())) != 0) {
      std::size_t look_ahead = 0;
      while (is_identifier_continue(m_impl->peek(look_ahead))) {
        look_ahead++;
      }

      // Skip whitespace
      while (m_impl->peek(look_ahead) == ' ' || m_impl->peek(look_ahead) == '\t' || m_impl->peek(look_ahead) == '\n' ||
             m_impl->peek(look_ahead) == '\r') {
        look_ahead++;
      }

      if (m_impl->peek(look_ahead) == '{') {
        is_struct_literal = true;
      }
    }

    if (is_struct_literal) {
      if (auto struct_lit = parse_struct_literal()) {
        return ast::Expr{std::move(*struct_lit)};
      }
    }

    bool is_qualified_call = false;
    std::size_t look_ahead = 0;

    // Skip first identifier
    while (is_identifier_continue(m_impl->peek(look_ahead))) {
      look_ahead++;
    }

    // Skip optional type parameters
    if (m_impl->peek(look_ahead) == '<') {
      int depth = 1;
      look_ahead++;
      while (depth > 0 && m_impl->peek(look_ahead) != k_eof_char) {
        if (m_impl->peek(look_ahead) == '<') {
          depth++;
        }
        if (m_impl->peek(look_ahead) == '>') {
          depth--;
        }
        look_ahead++;
      }
    }

    // Check for dotted path pattern
    while (m_impl->peek(look_ahead) == '.' && m_impl->peek(look_ahead + 1) != '.') {
      look_ahead++;  // skip '.'

      // Skip identifier
      if (!is_identifier_start(m_impl->peek(look_ahead))) {
        break;
      }
      while (is_identifier_continue(m_impl->peek(look_ahead))) {
        look_ahead++;
      }

      // Skip optional type parameters
      if (m_impl->peek(look_ahead) == '<') {
        int depth = 1;
        look_ahead++;
        while (depth > 0 && m_impl->peek(look_ahead) != k_eof_char) {
          if (m_impl->peek(look_ahead) == '<') {
            depth++;
          }
          if (m_impl->peek(look_ahead) == '>') {
            depth--;
          }
          look_ahead++;
        }
      }
    }

    // If we found a dotted path and it's followed by '(', it's a qualified call
    if (m_impl->peek(look_ahead) == '(') {
      // Count how many segments we saw
      std::size_t segment_count = 1;  // at least one segment
      for (std::size_t i = 0; i < look_ahead; i++) {
        if (m_impl->peek(i) == '.' && m_impl->peek(i + 1) != '.') {
          segment_count++;
        }
      }
      is_qualified_call = segment_count > 1;
    }

    if (is_qualified_call) {
      if (auto var_name = parse_qualified_variable_name()) {
        return ast::Expr{std::move(*var_name)};
      }
    } else {
      if (auto var_name = parse_variable_name()) {
        return ast::Expr{std::move(*var_name)};
      }
    }
  }

  m_impl->error("Expected expression", m_impl->make_range(start_pos));
  return std::nullopt;
}

[[nodiscard]] std::optional<ast::Expr> Parser::parse_expr() {
  return parse_binary_expr(0);
}

[[nodiscard]] std::optional<ast::Expr> Parser::parse_unary_expr() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Check for unbounded start range (.., ..=)
  if (m_impl->peek() == '.' && m_impl->peek(1) == '.') {
    m_impl->advance(2);  // consume '..'

    bool inclusive = false;
    if (m_impl->peek() == '=') {
      m_impl->advance();  // consume '='
      inclusive = true;
    }

    m_impl->skip_whitespace_and_comments();

    // Try to parse end expression
    // Exclude blocks to avoid ambiguity: `for i in ..{ }` should parse as unbounded range
    // followed by loop body, not as range with block expression endpoint.
    // Block expressions as range endpoints require explicit parentheses: `..({})`
    std::optional<ast::Expr> end_expr;
    if (m_impl->peek() != '{') {
      end_expr = parse_binary_expr(1);  // Precedence 1 to avoid consuming outer operators
    }

    ast::Range_Expr range;
    range.start = std::nullopt;  // Unbounded start
    range.end = end_expr ? std::make_optional(std::make_shared<ast::Expr>(std::move(*end_expr))) : std::nullopt;
    range.inclusive = inclusive;

    return ast::Expr{std::make_shared<ast::Range_Expr>(std::move(range))};
  }

  // Try unary operator
  if (auto unary_op = m_impl->try_parse_unary_op()) {
    auto operand = parse_unary_expr();  // Right associative
    if (!operand) {
      m_impl->error("Expected expression after unary operator", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    ast::Unary_Expr unary;
    unary.op = *unary_op;
    unary.operand = std::make_shared<ast::Expr>(std::move(*operand));
    return ast::Expr{std::make_shared<ast::Unary_Expr>(std::move(unary))};
  }

  // No unary operator, parse postfix expression (field access, function calls)
  return parse_postfix_expr();
}

[[nodiscard]] std::optional<ast::Expr> Parser::parse_binary_expr(int min_precedence_) {
  // Precedence climbing algorithm
  auto lhs = parse_unary_expr();
  if (!lhs) {
    return std::nullopt;
  }

  while (true) {
    m_impl->skip_whitespace_and_comments();

    // Check for 'as' cast operator before other operators
    // Cast has precedence 11 (just below postfix operators like ., (), [])
    // Grammar: x + y as I64 * z => x + ((y as I64) * z)
    if (m_impl->lookahead("as") &&
        (m_impl->peek(2) == ' ' || m_impl->peek(2) == '\t' || m_impl->peek(2) == '\n' || m_impl->peek(2) == '\r' ||
         (std::isupper(static_cast<unsigned char>(m_impl->peek(2))) != 0))) {
      int const cast_precedence = 11;  // Just below postfix (highest), above unary and multiplicative
      if (cast_precedence < min_precedence_) {
        break;
      }

      m_impl->advance(2);  // consume 'as'
      m_impl->skip_whitespace_and_comments();

      // Parse the target type
      auto target_type = parse_type_name();
      if (!target_type) {
        m_impl->error("Expected type name after 'as'");
        return std::nullopt;
      }

      // Build cast expression
      lhs = ast::Expr{std::make_shared<ast::Cast_Expr>(ast::make_cast_expr(std::move(*lhs), std::move(*target_type)))};
      continue;
    }

    // Check for range operators (.., ..=) before binary operators
    // Ranges have low precedence (between logical OR and assignment)
    if (m_impl->peek() == '.' && m_impl->peek(1) == '.') {
      int const range_precedence = 0;  // Lower than all binary ops
      if (range_precedence < min_precedence_) {
        break;
      }

      m_impl->advance(2);  // consume '..'

      bool inclusive = false;
      if (m_impl->peek() == '=') {
        m_impl->advance();  // consume '='
        inclusive = true;
      }

      // Parse right-hand side (end of range)
      m_impl->skip_whitespace_and_comments();

      // Try to parse end expression - if we can't, it's unbounded end (a..)
      // Don't parse blocks as range end - blocks are statements, not valid range bounds
      std::optional<ast::Expr> rhs;
      if (m_impl->peek() != '{') {
        rhs = parse_binary_expr(range_precedence + 1);
      }

      // Build range expression
      ast::Range_Expr range;
      range.start = std::make_optional(std::make_shared<ast::Expr>(std::move(*lhs)));
      range.end = rhs ? std::make_optional(std::make_shared<ast::Expr>(std::move(*rhs))) : std::nullopt;
      range.inclusive = inclusive;

      lhs = ast::Expr{std::make_shared<ast::Range_Expr>(std::move(range))};
      continue;
    }

    // Try to parse binary operator
    auto op = m_impl->try_parse([this, min_precedence_] {
      return m_impl->try_parse_binary_op_with_min_precedence(min_precedence_);
    });

    if (!op) {
      break;
    }

    int const precedence = get_precedence(*op);

    // Parse right-hand side with higher precedence (left associative)
    auto rhs = parse_binary_expr(precedence + 1);
    if (!rhs) {
      m_impl->error("Expected expression after binary operator");
      return std::nullopt;
    }

    // Build binary expression
    ast::Binary_Expr binary;
    binary.lhs = std::make_shared<ast::Expr>(std::move(*lhs));
    binary.op = *op;
    binary.rhs = std::make_shared<ast::Expr>(std::move(*rhs));

    lhs = ast::Expr{std::make_shared<ast::Binary_Expr>(std::move(binary))};
  }

  return lhs;
}

[[nodiscard]] std::optional<ast::Expr> Parser::parse_postfix_expr() {
  auto expr = parse_primary_expr();
  if (!expr) {
    return std::nullopt;
  }

  // Handle postfix operations: field access (.) and function calls ()
  while (true) {
    m_impl->skip_whitespace_and_comments();

    // Field access: expr.field
    if (m_impl->peek() == '.' && m_impl->peek(1) != '.') {
      m_impl->advance();  // consume '.'
      m_impl->skip_whitespace_and_comments();

      // Allow both identifier field names and numeric field names (for tuple access like pair.0)
      if (!is_identifier_start(m_impl->peek()) && (std::isdigit(static_cast<unsigned char>(m_impl->peek())) == 0)) {
        m_impl->error("Expected field name after '.'");
        return std::nullopt;
      }

      std::string field_name;
      field_name += m_impl->advance();
      while (is_identifier_continue(m_impl->peek())) {
        field_name += m_impl->advance();
      }

      ast::Field_Access_Expr field_access;
      field_access.object = std::make_shared<ast::Expr>(std::move(*expr));
      field_access.field_name = std::move(field_name);

      expr = ast::Expr{std::make_shared<ast::Field_Access_Expr>(std::move(field_access))};
      continue;
    }

    // Function call: expr(args)
    if (m_impl->peek() == '(') {
      m_impl->advance();  // consume '('

      std::vector<ast::Expr> params;
      m_impl->skip_whitespace_and_comments();

      if (m_impl->peek() != ')') {
        while (true) {
          auto param = parse_expr();
          if (!param) {
            m_impl->error("Expected expression in function call");
            return std::nullopt;
          }
          params.push_back(std::move(*param));

          m_impl->skip_whitespace_and_comments();
          if (m_impl->peek() == ')') {
            break;
          }
          if (m_impl->peek() == ',') {
            m_impl->advance();  // consume ','
            continue;
          }
          m_impl->error("Expected ',' or ')' in function call");
          return std::nullopt;
        }
      }

      if (!m_impl->expect(')')) {
        return std::nullopt;
      }

      // Handle function call - can be on Var_Name or Field_Access for method calls
      if (auto* var_name = std::get_if<ast::Var_Name>(&*expr)) {
        ast::Func_Call_Expr func_call;
        func_call.name = std::move(*var_name);
        func_call.params = std::move(params);

        expr = ast::Expr{std::make_shared<ast::Func_Call_Expr>(std::move(func_call))};
        continue;
      }

      // Method call: obj.method() - the target is a field access expression
      if (auto* field_access = std::get_if<std::shared_ptr<ast::Field_Access_Expr>>(&*expr)) {
        // Extract the method name from field access
        auto field_name = (*field_access)->field_name;
        auto object = std::move((*field_access)->object);

        // Create a method call: convert field access into qualified name with object as first param
        ast::Var_Name method_name;
        ast::Var_Name_Segment segment;
        segment.value = std::move(field_name);
        method_name.segments.push_back(std::move(segment));

        ast::Func_Call_Expr func_call;
        func_call.name = std::move(method_name);
        func_call.params = std::move(params);

        // Prepend the object as the first parameter (self)
        func_call.params.insert(func_call.params.begin(), std::move(*object));

        expr = ast::Expr{std::make_shared<ast::Func_Call_Expr>(std::move(func_call))};
        continue;
      }

      m_impl->error("Function call target must be a variable name or field access");
      return std::nullopt;
    }

    // Index expression: expr[index]
    if (m_impl->peek() == '[') {
      m_impl->advance();  // consume '['
      m_impl->skip_whitespace_and_comments();

      auto index = parse_expr();
      if (!index) {
        m_impl->error("Expected expression in array index");
        return std::nullopt;
      }

      m_impl->skip_whitespace_and_comments();
      if (!m_impl->expect(']')) {
        return std::nullopt;
      }

      ast::Index_Expr index_expr;
      index_expr.object = std::make_shared<ast::Expr>(std::move(*expr));
      index_expr.index = std::make_shared<ast::Expr>(std::move(*index));

      expr = ast::Expr{std::make_shared<ast::Index_Expr>(std::move(index_expr))};
      continue;
    }

    // No more postfix operations
    break;
  }

  return expr;
}

[[nodiscard]] std::optional<ast::If_Expr> Parser::parse_if_expr() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("if")) {
    m_impl->error("Expected 'if' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse condition (no parentheses)
  m_impl->skip_whitespace_and_comments();
  auto condition = parse_expr();
  if (!condition) {
    m_impl->error("Expected condition after 'if'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse then block
  m_impl->skip_whitespace_and_comments();
  auto then_block = parse_block();
  if (!then_block) {
    m_impl->error("Expected block after if condition", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional else-if and else clauses
  std::vector<ast::Else_If_Clause> else_ifs;
  std::optional<std::shared_ptr<ast::Block>> else_block;

  while (true) {
    m_impl->skip_whitespace_and_comments();

    if (!m_impl->match_keyword("else")) {
      break;
    }

    m_impl->skip_whitespace_and_comments();

    // Check for 'else if'
    if (m_impl->match_keyword("if")) {
      m_impl->skip_whitespace_and_comments();
      auto else_if_condition = parse_expr();
      if (!else_if_condition) {
        m_impl->error("Expected condition after 'else if'", m_impl->make_range(start_pos));
        return std::nullopt;
      }

      m_impl->skip_whitespace_and_comments();
      auto else_if_block = parse_block();
      if (!else_if_block) {
        m_impl->error("Expected block after else-if condition", m_impl->make_range(start_pos));
        return std::nullopt;
      }

      ast::Else_If_Clause else_if;
      else_if.condition = std::make_shared<ast::Expr>(std::move(*else_if_condition));
      else_if.then_block = std::make_shared<ast::Block>(std::move(*else_if_block));
      else_ifs.push_back(std::move(else_if));
      continue;
    }

    // Just 'else'
    m_impl->skip_whitespace_and_comments();
    auto final_else_block = parse_block();
    if (!final_else_block) {
      m_impl->error("Expected block after 'else'", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    else_block = std::make_shared<ast::Block>(std::move(*final_else_block));
    break;
  }

  ast::If_Expr result;
  result.condition = std::make_shared<ast::Expr>(std::move(*condition));
  result.then_block = std::make_shared<ast::Block>(std::move(*then_block));
  result.else_ifs = std::move(else_ifs);
  result.else_block = std::move(else_block);

  return result;
}

[[nodiscard]] std::optional<ast::Block> Parser::parse_block() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->expect('{')) {
    return std::nullopt;
  }

  std::vector<ast::Statement> statements;
  std::optional<std::shared_ptr<ast::Expr>> trailing_expr;

  while (true) {
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() == '}') {
      break;
    }

    // Try to parse statement
    auto stmt = parse_statement();
    if (stmt) {
      statements.push_back(std::move(*stmt));
      continue;
    }

    // If no statement, try trailing expression (no semicolon)
    auto expr = parse_expr();
    if (expr) {
      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == '}') {
        // This is the trailing expression
        trailing_expr = std::make_shared<ast::Expr>(std::move(*expr));
        break;
      }
      m_impl->error("Expected ';' or '}' after expression", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    m_impl->error("Expected statement or expression in block", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  if (!m_impl->expect('}')) {
    return std::nullopt;
  }

  ast::Block result;
  result.statements = std::move(statements);
  if (trailing_expr) {
    result.trailing_expr = std::move(*trailing_expr);
  }

  return result;
}

[[nodiscard]] std::optional<ast::While_Expr> Parser::parse_while_expr() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("while")) {
    m_impl->error("Expected 'while' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse condition (no parentheses)
  m_impl->skip_whitespace_and_comments();
  auto condition = parse_expr();
  if (!condition) {
    m_impl->error("Expected condition after 'while'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse body block
  m_impl->skip_whitespace_and_comments();
  auto body = parse_block();
  if (!body) {
    m_impl->error("Expected block after while condition", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  ast::While_Expr result;
  result.condition = std::make_shared<ast::Expr>(std::move(*condition));
  result.body = std::make_shared<ast::Block>(std::move(*body));

  return result;
}

[[nodiscard]] std::optional<ast::For_Expr> Parser::parse_for_expr() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("for")) {
    m_impl->error("Expected 'for' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse pattern (supports simple, tuple, struct patterns)
  m_impl->skip_whitespace_and_comments();
  auto pattern = parse_pattern();
  if (!pattern) {
    m_impl->error("Expected pattern after 'for'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Validate that simple patterns are not keywords
  if (auto* simple = std::get_if<ast::Simple_Pattern>(&*pattern)) {
    for (auto const& kw: k_keywords) {
      if (simple->name == kw) {
        m_impl->error("Cannot use keyword '" + simple->name + "' as pattern binding", m_impl->make_range(start_pos));
        return std::nullopt;
      }
    }
  }

  // Parse 'in' keyword
  m_impl->skip_whitespace_and_comments();
  if (!m_impl->match_keyword("in")) {
    m_impl->error("Expected 'in' keyword after for pattern", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse iterator expression
  m_impl->skip_whitespace_and_comments();
  auto iterator = parse_expr();
  if (!iterator) {
    m_impl->error("Expected expression after 'in'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse body block
  m_impl->skip_whitespace_and_comments();
  auto body = parse_block();
  if (!body) {
    m_impl->error("Expected block after for iterator", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  ast::For_Expr result;
  result.pattern = std::move(*pattern);
  result.iterator = std::make_shared<ast::Expr>(std::move(*iterator));
  result.body = std::make_shared<ast::Block>(std::move(*body));

  return result;
}

[[nodiscard]] std::optional<ast::Match_Expr> Parser::parse_match_expr() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("match")) {
    m_impl->error("Expected 'match' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse scrutinee expression
  m_impl->skip_whitespace_and_comments();
  auto scrutinee = parse_expr();
  if (!scrutinee) {
    m_impl->error("Expected expression after 'match'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse match arms in braces
  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect('{')) {
    return std::nullopt;
  }

  std::vector<ast::Match_Arm> arms;

  while (true) {
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() == '}') {
      break;
    }

    // Parse pattern using full pattern parser
    auto pattern = parse_pattern();
    if (!pattern) {
      m_impl->error("Expected pattern in match arm", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    // Optional guard (if condition)
    std::optional<std::shared_ptr<ast::Expr>> guard;
    m_impl->skip_whitespace_and_comments();
    if (m_impl->match_keyword("if")) {
      m_impl->skip_whitespace_and_comments();
      auto guard_expr = parse_expr();
      if (!guard_expr) {
        m_impl->error("Expected expression after 'if' in match guard", m_impl->make_range(start_pos));
        return std::nullopt;
      }
      guard = std::make_shared<ast::Expr>(std::move(*guard_expr));
    }

    // Parse => arrow
    m_impl->skip_whitespace_and_comments();
    if (!m_impl->expect('=')) {
      return std::nullopt;
    }
    if (!m_impl->expect('>')) {
      return std::nullopt;
    }

    // Parse result expression
    m_impl->skip_whitespace_and_comments();
    auto result = parse_expr();
    if (!result) {
      m_impl->error("Expected expression after '=>' in match arm", m_impl->make_range(start_pos));
      return std::nullopt;
    }

    ast::Match_Arm arm;
    arm.pattern = std::move(*pattern);
    arm.guard = std::move(guard);
    arm.result = std::make_shared<ast::Expr>(std::move(*result));
    arms.push_back(std::move(arm));

    // Check for comma or closing brace
    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == ',') {
      m_impl->advance();
      continue;
    }
    if (m_impl->peek() == '}') {
      break;
    }
    m_impl->error("Expected ',' or '}' in match expression", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  if (!m_impl->expect('}')) {
    return std::nullopt;
  }

  ast::Match_Expr result;
  result.scrutinee = std::make_shared<ast::Expr>(std::move(*scrutinee));
  result.arms = std::move(arms);

  return result;
}

// Statements

[[nodiscard]] std::optional<ast::Statement> Parser::parse_statement() {
  m_impl->skip_whitespace_and_comments();

  // Try function definition
  if (m_impl->lookahead("fn")) {
    auto func_def = m_impl->try_parse([this] { return parse_func_def(); });
    if (func_def) {
      return ast::Statement{std::make_shared<ast::Func_Def>(std::move(*func_def))};
    }
  }

  // Try struct definition
  if (m_impl->lookahead("struct")) {
    auto struct_def = m_impl->try_parse([this] { return parse_struct_def(); });
    if (struct_def) {
      return ast::Statement{std::make_shared<ast::Struct_Def>(std::move(*struct_def))};
    }
  }

  // Try enum definition
  if (m_impl->lookahead("enum")) {
    auto enum_def = m_impl->try_parse([this] { return parse_enum_def(); });
    if (enum_def) {
      return ast::Statement{std::make_shared<ast::Enum_Def>(std::move(*enum_def))};
    }
  }

  // Try trait definition or impl blocks (both start with keywords)
  if (m_impl->lookahead("trait")) {
    auto trait_def = m_impl->try_parse([this] { return parse_trait_def(); });
    if (trait_def) {
      return ast::Statement{std::make_shared<ast::Trait_Def>(std::move(*trait_def))};
    }
  }

  // Try impl blocks - need to distinguish between trait impl and regular impl
  if (m_impl->lookahead("impl")) {
    auto trait_impl = m_impl->try_parse([this] { return parse_trait_impl(); });
    if (trait_impl) {
      return ast::Statement{std::make_shared<ast::Trait_Impl>(std::move(*trait_impl))};
    }

    auto impl_block = m_impl->try_parse([this] { return parse_impl_block(); });
    if (impl_block) {
      return ast::Statement{std::make_shared<ast::Impl_Block>(std::move(*impl_block))};
    }
  }

  // Try type alias
  if (m_impl->lookahead("type")) {
    auto type_alias = m_impl->try_parse([this] { return parse_type_alias(); });
    if (type_alias) {
      return ast::Statement{std::make_shared<ast::Type_Alias>(std::move(*type_alias))};
    }
  }

  // Try let statement
  if (m_impl->lookahead("let")) {
    auto let_stmt = m_impl->try_parse([this] { return parse_let_statement(); });
    if (let_stmt) {
      return ast::Statement{std::make_shared<ast::Let_Statement>(std::move(*let_stmt))};
    }
  }

  // Try return statement
  if (m_impl->lookahead("return")) {
    auto return_stmt = m_impl->try_parse([this] { return parse_return_statement(); });
    if (return_stmt) {
      return ast::Statement{std::move(*return_stmt)};
    }
  }

  // Try break statement
  if (m_impl->lookahead("break")) {
    auto break_stmt = m_impl->try_parse([this] { return parse_break_statement(); });
    if (break_stmt) {
      return ast::Statement{std::move(*break_stmt)};
    }
  }

  // Try continue statement
  if (m_impl->lookahead("continue")) {
    auto continue_stmt = m_impl->try_parse([this] { return parse_continue_statement(); });
    if (continue_stmt) {
      return ast::Statement{*continue_stmt};
    }
  }

  // Try block statement (nested blocks)
  if (m_impl->peek() == '{') {
    auto block = m_impl->try_parse([this] { return parse_block(); });
    if (block) {
      m_impl->skip_whitespace_and_comments();
      // Block as statement doesn't need semicolon
      return ast::Statement{std::make_shared<ast::Block>(std::move(*block))};
    }
  }

  // Try assignment statement
  auto assignment_stmt = m_impl->try_parse([this] { return parse_assignment_statement(); });
  if (assignment_stmt) {
    m_impl->skip_whitespace_and_comments();
    if (!m_impl->expect(';')) {
      m_impl->error("Expected ';' after assignment");
      return std::nullopt;
    }
    return ast::Statement{std::make_shared<ast::Assignment_Statement>(std::move(*assignment_stmt))};
  }

  // Try expression - some expressions can be statements without semicolons
  auto expr_stmt = m_impl->try_parse([this] { return m_impl->try_parse_expr_as_statement(this); });
  if (expr_stmt) {
    return expr_stmt;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<ast::Return_Statement> Parser::parse_return_statement() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("return")) {
    m_impl->error("Expected 'return' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional return expression
  m_impl->skip_whitespace_and_comments();
  auto expr = parse_expr();
  if (!expr) {
    m_impl->error("Expected expression after 'return'", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    return std::nullopt;
  }

  ast::Return_Statement result;
  result.expr = std::move(*expr);

  return result;
}

[[nodiscard]] std::optional<ast::Break_Statement> Parser::parse_break_statement() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("break")) {
    m_impl->error("Expected 'break' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional break value
  std::optional<ast::Expr> value;
  m_impl->skip_whitespace_and_comments();

  if (m_impl->peek() != ';') {
    if (auto expr = parse_expr()) {
      value = std::move(*expr);
    }
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    return std::nullopt;
  }

  ast::Break_Statement result;
  result.value = std::move(value);

  return result;
}

[[nodiscard]] std::optional<ast::Continue_Statement> Parser::parse_continue_statement() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("continue")) {
    m_impl->error("Expected 'continue' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    return std::nullopt;
  }

  return ast::Continue_Statement{};
}

[[nodiscard]] std::optional<ast::Func_Param> Parser::parse_func_param() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  bool is_mut = false;
  if (m_impl->match_keyword("mut")) {
    is_mut = true;
    m_impl->skip_whitespace_and_comments();
  }

  auto name_result = parse_variable_name();
  if (!name_result) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::optional<ast::Type_Name> type;
  // Check for optional type annotation
  if (m_impl->peek() == ':') {
    m_impl->advance();  // consume ':'
    m_impl->skip_whitespace_and_comments();
    auto type_result = parse_type_name();
    if (!type_result) {
      m_impl->error("Expected type annotation after ':' in parameter", m_impl->make_range(start_pos));
      return std::nullopt;
    }
    type = std::move(*type_result);
  }

  ast::Func_Param result;
  result.is_mut = is_mut;
  result.name = std::move(name_result->segments[0].value);
  result.type = std::move(type);

  return result;
}

[[nodiscard]] std::optional<ast::Func_Decl> Parser::parse_func_decl() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("fn")) {
    m_impl->error("Expected 'fn' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto name_result = parse_variable_name();
  if (!name_result) {
    m_impl->error("Expected function name after 'fn'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error(
              "Expected type parameter in function declaration",
              m_impl->make_range(m_impl->current_position())
          );
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('(')) {
    m_impl->error("Expected '(' to start parameter list", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Func_Param> func_params;
  if (m_impl->peek() != ')') {
    while (true) {
      auto param = parse_func_param();
      if (!param) {
        m_impl->error("Expected function parameter", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
      func_params.push_back(std::move(*param));

      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == ',') {
        m_impl->advance();
        m_impl->skip_whitespace_and_comments();
      } else {
        break;
      }
    }
  }

  if (!m_impl->expect(')')) {
    m_impl->error("Expected ')' to close parameter list", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  if (!m_impl->expect(':')) {
    m_impl->error("Expected ':' before return type", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto return_type = parse_type_name();
  if (!return_type) {
    m_impl->error("Expected return type after ':'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::optional<ast::Where_Clause> where_clause;
  if (m_impl->match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      m_impl->error("Expected where clause after 'where'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
  }

  ast::Func_Decl result;
  result.name = std::move(name_result->segments[0].value);
  result.type_params = std::move(type_params);
  result.func_params = std::move(func_params);
  result.return_type = std::move(*return_type);
  result.where_clause = std::move(where_clause);

  return result;
}

[[nodiscard]] std::optional<ast::Func_Def> Parser::parse_func_def() {
  m_impl->skip_whitespace_and_comments();

  auto decl = parse_func_decl();
  if (!decl) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto body = parse_block();
  if (!body) {
    m_impl->error("Expected function body block", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Func_Def result;
  result.declaration = std::move(*decl);
  result.body = std::move(*body);

  return result;
}

[[nodiscard]] std::optional<ast::Struct_Field> Parser::parse_struct_field() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Check for optional 'pub' keyword
  bool const is_pub = m_impl->match_keyword("pub");
  if (is_pub) {
    m_impl->skip_whitespace_and_comments();
  }

  auto name = parse_variable_name();
  if (!name) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(':')) {
    m_impl->error("Expected ':' after field name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto type = parse_type_name();
  if (!type) {
    m_impl->error("Expected type after ':' in field declaration", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Struct_Field result;
  result.is_pub = is_pub;
  result.name = std::move(name->segments[0].value);
  result.type = std::move(*type);

  return result;
}

[[nodiscard]] std::optional<ast::Struct_Def> Parser::parse_struct_def() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("struct")) {
    m_impl->error("Expected 'struct' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Parse struct name (just the identifier, not a full type name with type args)
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected struct name after 'struct'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  std::string struct_name;
  struct_name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    struct_name += m_impl->advance();
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error("Expected type parameter in struct definition", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  std::optional<ast::Where_Clause> where_clause;
  if (m_impl->match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      m_impl->error("Expected where clause after 'where'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('{')) {
    m_impl->error("Expected '{' to start struct body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Struct_Field> fields;
  while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
    auto field = parse_struct_field();
    if (!field) {
      m_impl->error("Expected struct field", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    fields.push_back(std::move(*field));

    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == ',') {
      m_impl->advance();
      m_impl->skip_whitespace_and_comments();
    } else if (m_impl->peek() != '}') {
      m_impl->error("Expected ',' or '}' after struct field", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
  }

  if (!m_impl->expect('}')) {
    m_impl->error("Expected '}' to close struct body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Struct_Def result;
  result.name = std::move(struct_name);
  result.type_params = std::move(type_params);
  result.fields = std::move(fields);
  result.where_clause = std::move(where_clause);

  return result;
}

[[nodiscard]] std::optional<ast::Enum_Variant> Parser::parse_enum_variant() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  auto name = parse_type_name();
  if (!name) {
    return std::nullopt;
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    m_impl->error("Enum variant name must be a simple type name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    m_impl->error("Enum variant name must be a simple identifier", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  std::string variant_name = path.segments[0].value;

  m_impl->skip_whitespace_and_comments();

  if (m_impl->peek() == '(') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    std::vector<ast::Type_Name> tuple_fields;
    if (m_impl->peek() != ')') {
      while (true) {
        auto field_type = parse_type_name();
        if (!field_type) {
          m_impl->error("Expected type in tuple variant", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        tuple_fields.push_back(std::move(*field_type));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
          // Allow trailing comma before ')'
          if (m_impl->peek() == ')') {
            break;
          }
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect(')')) {
      m_impl->error("Expected ')' to close tuple variant", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    ast::Tuple_Variant tuple_var;
    tuple_var.name = std::move(variant_name);
    tuple_var.tuple_fields = std::move(tuple_fields);

    return ast::Enum_Variant{std::move(tuple_var)};
  }
  if (m_impl->peek() == '{') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    std::vector<ast::Struct_Field> struct_fields;
    while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
      auto field = parse_struct_field();
      if (!field) {
        m_impl->error("Expected struct field in variant", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
      struct_fields.push_back(std::move(*field));

      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == ',') {
        m_impl->advance();
        m_impl->skip_whitespace_and_comments();
      } else if (m_impl->peek() != '}') {
        m_impl->error("Expected ',' or '}' after struct field", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
    }

    if (!m_impl->expect('}')) {
      m_impl->error("Expected '}' to close struct variant", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    ast::Struct_Variant struct_var;
    struct_var.name = std::move(variant_name);
    struct_var.struct_fields = std::move(struct_fields);

    return ast::Enum_Variant{std::move(struct_var)};
  }
  ast::Unit_Variant unit_var;
  unit_var.name = std::move(variant_name);
  return ast::Enum_Variant{std::move(unit_var)};
}

[[nodiscard]] std::optional<ast::Enum_Def> Parser::parse_enum_def() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("enum")) {
    m_impl->error("Expected 'enum' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Parse enum name (just the identifier, not a full type name with type args)
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected enum name after 'enum'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  std::string enum_name;
  enum_name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    enum_name += m_impl->advance();
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error("Expected type parameter in enum definition", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  std::optional<ast::Where_Clause> where_clause;
  if (m_impl->match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      m_impl->error("Expected where clause after 'where'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('{')) {
    m_impl->error("Expected '{' to start enum body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Enum_Variant> variants;
  while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
    auto variant = parse_enum_variant();
    if (!variant) {
      m_impl->error("Expected enum variant", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    variants.push_back(std::move(*variant));

    m_impl->skip_whitespace_and_comments();
    if (m_impl->peek() == ',') {
      m_impl->advance();
      m_impl->skip_whitespace_and_comments();
    } else if (m_impl->peek() != '}') {
      m_impl->error("Expected ',' or '}' after enum variant", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
  }

  if (!m_impl->expect('}')) {
    m_impl->error("Expected '}' to close enum body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Enum_Def result;
  result.name = std::move(enum_name);
  result.type_params = std::move(type_params);
  result.variants = std::move(variants);
  result.where_clause = std::move(where_clause);

  return result;
}

[[nodiscard]] std::optional<ast::Assoc_Type_Decl> Parser::parse_assoc_type_decl() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("type")) {
    m_impl->error("Expected 'type' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto name = parse_type_name();
  if (!name) {
    m_impl->error("Expected associated type name after 'type'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    m_impl->error("Associated type name must be a simple type name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    m_impl->error("Associated type name must be a simple identifier", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Trait_Bound> bounds;
  if (m_impl->peek() == ':') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    while (true) {
      auto bound_type = parse_type_name();
      if (!bound_type) {
        m_impl->error("Expected trait bound after ':'", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      if (std::get_if<ast::Path_Type>(&*bound_type) == nullptr) {
        m_impl->error("Trait bound must be a path type", m_impl->make_range(start_pos));
        return std::nullopt;
      }

      ast::Trait_Bound bound;
      bound.trait_name = std::get<ast::Path_Type>(std::move(*bound_type));
      bounds.push_back(std::move(bound));

      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == '+') {
        m_impl->advance();
        m_impl->skip_whitespace_and_comments();
      } else {
        break;
      }
    }
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    m_impl->error("Expected ';' after associated type declaration", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Assoc_Type_Decl result;
  result.name = path.segments[0].value;
  result.bounds = std::move(bounds);

  return result;
}

[[nodiscard]] std::optional<ast::Trait_Def> Parser::parse_trait_def() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("trait")) {
    m_impl->error("Expected 'trait' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Parse trait name (just the identifier, not type parameters)
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected trait name after 'trait'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  std::string trait_name;
  trait_name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    trait_name += m_impl->advance();
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error("Expected type parameter in trait definition", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  std::optional<ast::Where_Clause> where_clause;
  if (m_impl->match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      m_impl->error("Expected where clause after 'where'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('{')) {
    m_impl->error("Expected '{' to start trait body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Assoc_Type_Decl> assoc_types;
  std::vector<ast::Func_Decl> methods;

  while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
    auto const item_start = m_impl->current_position();

    if (m_impl->lookahead("type")) {
      auto assoc_type = parse_assoc_type_decl();
      if (!assoc_type) {
        m_impl->error("Expected associated type declaration", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
      assoc_types.push_back(std::move(*assoc_type));
    } else if (m_impl->lookahead("fn")) {
      auto method = parse_func_decl();
      if (!method) {
        m_impl->error("Expected method declaration", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      m_impl->skip_whitespace_and_comments();
      if (!m_impl->expect(';')) {
        m_impl->error("Expected ';' after method declaration in trait", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      methods.push_back(std::move(*method));
    } else {
      m_impl->error("Expected 'type' or 'fn' in trait body", m_impl->make_range(item_start));
      return std::nullopt;
    }

    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('}')) {
    m_impl->error("Expected '}' to close trait body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Trait_Def result;
  result.name = std::move(trait_name);
  result.type_params = std::move(type_params);
  result.assoc_types = std::move(assoc_types);
  result.methods = std::move(methods);
  result.where_clause = std::move(where_clause);

  return result;
}

[[nodiscard]] std::optional<ast::Type_Alias> Parser::parse_type_alias() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("type")) {
    m_impl->error("Expected 'type' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Parse type alias name (just the identifier)
  if (!is_identifier_start(m_impl->peek())) {
    m_impl->error("Expected type alias name after 'type'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  std::string alias_name;
  alias_name += m_impl->advance();
  while (is_identifier_continue(m_impl->peek())) {
    alias_name += m_impl->advance();
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error("Expected type parameter in type alias", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('=')) {
    m_impl->error("Expected '=' in type alias definition", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto aliased_type = parse_type_name();
  if (!aliased_type) {
    m_impl->error("Expected type after '=' in type alias", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    m_impl->error("Expected ';' after type alias definition", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Type_Alias result;
  result.name = std::move(alias_name);
  result.type_params = std::move(type_params);
  result.aliased_type = std::move(*aliased_type);

  return result;
}

[[nodiscard]] std::optional<ast::Impl_Block> Parser::parse_impl_block() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("impl")) {
    m_impl->error("Expected 'impl' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error("Expected type parameter in impl block", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  auto type_name = parse_type_name();
  if (!type_name) {
    m_impl->error("Expected type name in impl block", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::optional<ast::Where_Clause> where_clause;
  if (m_impl->match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      m_impl->error("Expected where clause after 'where'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('{')) {
    m_impl->error("Expected '{' to start impl block body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Func_Def> methods;
  while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
    // Check for optional 'pub' keyword for methods
    bool const is_pub = m_impl->match_keyword("pub");
    if (is_pub) {
      m_impl->skip_whitespace_and_comments();
    }

    auto method = parse_func_def();
    if (!method) {
      m_impl->error("Expected method definition in impl block", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    method->is_pub = is_pub;
    methods.push_back(std::move(*method));
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('}')) {
    m_impl->error("Expected '}' to close impl block body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Impl_Block result;
  result.type_name = std::move(*type_name);
  result.type_params = std::move(type_params);
  result.methods = std::move(methods);
  result.where_clause = std::move(where_clause);

  return result;
}

[[nodiscard]] std::optional<ast::Assoc_Type_Impl> Parser::parse_assoc_type_impl() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("type")) {
    m_impl->error("Expected 'type' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto name = parse_type_name();
  if (!name) {
    m_impl->error("Expected associated type name after 'type'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    m_impl->error("Associated type name must be a simple type name", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    m_impl->error("Associated type name must be a simple identifier", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect('=')) {
    m_impl->error("Expected '=' in associated type implementation", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto type_value = parse_type_name();
  if (!type_value) {
    m_impl->error(
        "Expected type after '=' in associated type implementation",
        m_impl->make_range(m_impl->current_position())
    );
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    m_impl->error("Expected ';' after associated type implementation", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Assoc_Type_Impl result;
  result.name = path.segments[0].value;
  result.type_value = std::move(*type_value);

  return result;
}

[[nodiscard]] std::optional<ast::Trait_Impl> Parser::parse_trait_impl() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("impl")) {
    m_impl->error("Expected 'impl' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (m_impl->peek() == '<') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    if (m_impl->peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          m_impl->error("Expected type parameter in trait impl", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect('>')) {
      m_impl->error("Expected '>' to close type parameter list", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    m_impl->skip_whitespace_and_comments();
  }

  auto trait_name = parse_type_name();
  if (!trait_name) {
    m_impl->error("Expected trait name in trait impl", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  // Check for 'for' keyword - if not present, this is likely an impl block, not trait impl
  if (!m_impl->lookahead("for")) {
    return std::nullopt;  // Not a trait impl, backtrack
  }

  m_impl->advance(3);  // consume 'for'

  m_impl->skip_whitespace_and_comments();
  auto type_name = parse_type_name();
  if (!type_name) {
    m_impl->error("Expected type name after 'for' in trait impl", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::optional<ast::Where_Clause> where_clause;
  if (m_impl->match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      m_impl->error("Expected where clause after 'where'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('{')) {
    m_impl->error("Expected '{' to start trait impl body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::vector<ast::Assoc_Type_Impl> assoc_type_impls;
  std::vector<ast::Func_Def> methods;

  while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
    auto const item_start = m_impl->current_position();

    if (m_impl->lookahead("type")) {
      auto assoc_type = parse_assoc_type_impl();
      if (!assoc_type) {
        m_impl->error("Expected associated type implementation", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
      assoc_type_impls.push_back(std::move(*assoc_type));
    } else if (m_impl->lookahead("fn")) {
      auto method = parse_func_def();
      if (!method) {
        m_impl->error("Expected method definition", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
      methods.push_back(std::move(*method));
    } else {
      m_impl->error("Expected 'type' or 'fn' in trait impl body", m_impl->make_range(item_start));
      return std::nullopt;
    }

    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('}')) {
    m_impl->error("Expected '}' to close trait impl body", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Trait_Impl result;
  result.trait_name = std::move(*trait_name);
  result.type_name = std::move(*type_name);
  result.type_params = std::move(type_params);
  result.assoc_type_impls = std::move(assoc_type_impls);
  result.methods = std::move(methods);
  result.where_clause = std::move(where_clause);

  return result;
}

[[nodiscard]] std::optional<ast::Pattern> Parser::parse_single_pattern() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (m_impl->peek() == '_') {
    m_impl->advance();
    return ast::Pattern{ast::Wildcard_Pattern{}};
  }

  if (m_impl->peek() == '(') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    std::vector<std::shared_ptr<ast::Pattern>> elements;
    if (m_impl->peek() != ')') {
      while (true) {
        auto element = parse_pattern();
        if (!element) {
          m_impl->error("Expected pattern in tuple", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        elements.push_back(std::make_shared<ast::Pattern>(std::move(*element)));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect(')')) {
      m_impl->error("Expected ')' to close tuple pattern", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    ast::Tuple_Pattern tuple_pat;
    tuple_pat.elements = std::move(elements);
    return ast::Pattern{std::move(tuple_pat)};
  }

  if (m_impl->peek() == '"' || std::isdigit(m_impl->peek()) != 0 ||
      (m_impl->peek() == '-' && std::isdigit(m_impl->peek(1)) != 0) ||
      (m_impl->lookahead("true") && !is_identifier_continue(m_impl->peek(4))) ||
      (m_impl->lookahead("false") && !is_identifier_continue(m_impl->peek(5)))) {
    auto expr = parse_primary_expr();
    if (!expr) {
      m_impl->error("Expected literal in pattern", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    ast::Literal_Pattern lit_pat;
    lit_pat.value = std::make_shared<ast::Expr>(std::move(*expr));
    return ast::Pattern{std::move(lit_pat)};
  }

  auto name = parse_type_name();
  if (!name) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  if (m_impl->peek() == '(') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    std::vector<std::shared_ptr<ast::Pattern>> patterns;
    if (m_impl->peek() != ')') {
      while (true) {
        auto pattern = parse_pattern();
        if (!pattern) {
          m_impl->error("Expected pattern in enum variant", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        patterns.push_back(std::make_shared<ast::Pattern>(std::move(*pattern)));

        m_impl->skip_whitespace_and_comments();
        if (m_impl->peek() == ',') {
          m_impl->advance();
          m_impl->skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!m_impl->expect(')')) {
      m_impl->error("Expected ')' to close enum pattern", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    ast::Enum_Pattern enum_pat;
    enum_pat.type_name = std::move(*name);
    enum_pat.patterns = std::move(patterns);
    return ast::Pattern{std::move(enum_pat)};
  }
  if (m_impl->peek() == '{') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();

    std::vector<ast::Field_Pattern> fields;
    bool has_rest = false;

    // Check for empty struct pattern or immediate ..
    if (m_impl->peek() == '.' && m_impl->peek(1) == '.') {
      m_impl->advance();  // first .
      m_impl->advance();  // second .
      has_rest = true;
      m_impl->skip_whitespace_and_comments();
      if (!m_impl->expect('}')) {
        m_impl->error("Expected '}' after '..' in struct pattern", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
      ast::Struct_Pattern struct_pat;
      struct_pat.type_name = std::move(*name);
      struct_pat.fields = std::move(fields);
      struct_pat.has_rest = has_rest;
      return ast::Pattern{std::move(struct_pat)};
    }

    while (m_impl->peek() != '}' && m_impl->pos < m_impl->diagnostics->source().size()) {
      // Check for .. rest pattern
      if (m_impl->peek() == '.' && m_impl->peek(1) == '.') {
        m_impl->advance();  // first .
        m_impl->advance();  // second .
        has_rest = true;
        m_impl->skip_whitespace_and_comments();

        // .. must be last element
        if (m_impl->peek() == ',') {
          m_impl->error(
              "Rest pattern '..' must be the last element in struct pattern",
              m_impl->make_range(m_impl->current_position())
          );
          return std::nullopt;
        }
        break;
      }

      // Parse field name
      if (!is_identifier_start(m_impl->peek())) {
        m_impl->error("Expected field name in struct pattern", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }

      std::string field_name;
      field_name += m_impl->advance();
      while (is_identifier_continue(m_impl->peek())) {
        field_name += m_impl->advance();
      }

      m_impl->skip_whitespace_and_comments();

      ast::Pattern field_pattern;

      // Check for shorthand syntax (field without colon)
      if (m_impl->peek() == ',' || m_impl->peek() == '}' || (m_impl->peek() == '.' && m_impl->peek(1) == '.')) {
        // Shorthand: field name becomes both field name and pattern binding
        ast::Simple_Pattern simple;
        simple.name = field_name;
        field_pattern = ast::Pattern{std::move(simple)};
      } else if (m_impl->peek() == ':') {
        // Full syntax: field: pattern
        m_impl->advance();  // consume ':'
        m_impl->skip_whitespace_and_comments();
        auto pattern = parse_pattern();
        if (!pattern) {
          m_impl->error("Expected pattern after ':' in field pattern", m_impl->make_range(m_impl->current_position()));
          return std::nullopt;
        }
        field_pattern = std::move(*pattern);
      } else {
        m_impl->error(
            "Expected ':' or ',' or '}' after field name in pattern",
            m_impl->make_range(m_impl->current_position())
        );
        return std::nullopt;
      }

      ast::Field_Pattern field_pat;
      field_pat.name = std::move(field_name);
      field_pat.pattern = std::make_shared<ast::Pattern>(std::move(field_pattern));
      fields.push_back(std::move(field_pat));

      m_impl->skip_whitespace_and_comments();
      if (m_impl->peek() == ',') {
        m_impl->advance();
        m_impl->skip_whitespace_and_comments();
        // Allow trailing comma before } or ..
        if (m_impl->peek() == '}' || (m_impl->peek() == '.' && m_impl->peek(1) == '.')) {
          if (m_impl->peek() == '.' && m_impl->peek(1) == '.') {
            continue;  // Let the loop handle ..
          }
          break;
        }
      } else if (m_impl->peek() != '}' && (m_impl->peek() != '.' || m_impl->peek(1) != '.')) {
        m_impl->error("Expected ',' or '}' after field pattern", m_impl->make_range(m_impl->current_position()));
        return std::nullopt;
      }
    }

    if (!m_impl->expect('}')) {
      m_impl->error("Expected '}' to close struct pattern", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    ast::Struct_Pattern struct_pat;
    struct_pat.type_name = std::move(*name);
    struct_pat.fields = std::move(fields);
    struct_pat.has_rest = has_rest;
    return ast::Pattern{std::move(struct_pat)};
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    m_impl->error("Pattern must be a simple name, not a function type", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    m_impl->error("Simple pattern must be a single identifier without type parameters", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  ast::Simple_Pattern simple_pat;
  simple_pat.name = path.segments[0].value;
  return ast::Pattern{std::move(simple_pat)};
}

[[nodiscard]] std::optional<ast::Pattern> Parser::parse_pattern() {
  // Parse first pattern
  auto first = parse_single_pattern();
  if (!first) {
    return std::nullopt;
  }

  // Check for | to form or-pattern
  m_impl->skip_whitespace_and_comments();
  if (m_impl->peek() != '|') {
    return first;  // Just a single pattern
  }

  // Parse or-pattern alternatives
  std::vector<std::shared_ptr<ast::Pattern>> alternatives;
  alternatives.push_back(std::make_shared<ast::Pattern>(std::move(*first)));

  while (m_impl->peek() == '|') {
    m_impl->advance();  // consume '|'
    m_impl->skip_whitespace_and_comments();

    auto alternative = parse_single_pattern();
    if (!alternative) {
      m_impl->error("Expected pattern after '|'", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }

    alternatives.push_back(std::make_shared<ast::Pattern>(std::move(*alternative)));
    m_impl->skip_whitespace_and_comments();
  }

  ast::Or_Pattern or_pat;
  or_pat.alternatives = std::move(alternatives);
  return ast::Pattern{std::move(or_pat)};
}

[[nodiscard]] std::optional<ast::Let_Statement> Parser::parse_let_statement() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  if (!m_impl->match_keyword("let")) {
    m_impl->error("Expected 'let' keyword", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  bool is_mut = false;
  if (m_impl->match_keyword("mut")) {
    is_mut = true;
    m_impl->skip_whitespace_and_comments();
  }

  auto pattern = parse_pattern();
  if (!pattern) {
    m_impl->error("Expected pattern after 'let'", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  std::optional<ast::Type_Name> type;
  if (m_impl->peek() == ':') {
    m_impl->advance();
    m_impl->skip_whitespace_and_comments();
    auto type_result = parse_type_name();
    if (!type_result) {
      m_impl->error("Expected type after ':' in let statement", m_impl->make_range(m_impl->current_position()));
      return std::nullopt;
    }
    type = std::move(*type_result);
    m_impl->skip_whitespace_and_comments();
  }

  if (!m_impl->expect('=')) {
    m_impl->error("Expected '=' in let statement", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  auto value = parse_expr();
  if (!value) {
    m_impl->error("Expected expression after '=' in let statement", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();
  if (!m_impl->expect(';')) {
    m_impl->error("Expected ';' after let statement", m_impl->make_range(m_impl->current_position()));
    return std::nullopt;
  }

  ast::Let_Statement result;
  result.is_mut = is_mut;
  result.pattern = std::move(*pattern);
  result.type = std::move(type);
  result.value = std::make_shared<ast::Expr>(std::move(*value));

  return result;
}

[[nodiscard]] std::optional<ast::Assignment_Statement> Parser::parse_assignment_statement() {
  m_impl->skip_whitespace_and_comments();

  auto const start_pos = m_impl->current_position();

  // Parse left-hand side (must be a valid lvalue: var name or field access)
  auto lhs = parse_postfix_expr();
  if (!lhs) {
    return std::nullopt;
  }

  m_impl->skip_whitespace_and_comments();

  // Check for assignment operator
  if (m_impl->peek() != '=') {
    // Not an assignment
    return std::nullopt;
  }

  // Make sure it's not '==' (equality comparison)
  if (m_impl->peek(1) == '=') {
    // Not an assignment
    return std::nullopt;
  }

  m_impl->advance();  // consume '='

  // Parse right-hand side (any expression)
  m_impl->skip_whitespace_and_comments();
  auto rhs = parse_expr();
  if (!rhs) {
    m_impl->error("Expected expression after '=' in assignment", m_impl->make_range(start_pos));
    return std::nullopt;
  }

  // Build assignment statement
  ast::Assignment_Statement assignment;
  assignment.target = std::make_shared<ast::Expr>(std::move(*lhs));
  assignment.value = std::make_shared<ast::Expr>(std::move(*rhs));

  return assignment;
}

}  // namespace life_lang::parser
