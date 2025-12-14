// Parser Implementation for life-lang
//
// CRITICAL: This parser must implement the grammar defined in GRAMMAR.md exactly.
//
// Grammar Synchronization Rules:
// 1. GRAMMAR.md is the authoritative source of truth for language syntax
// 2. Every parse_* method corresponds to a grammar rule in GRAMMAR.md
// 3. When adding/modifying parse_* methods, update GRAMMAR.md accordingly
// 4. When changing grammar rules, update corresponding parse_* methods
// 5. Parser must NOT accept inputs that violate the grammar
//
// Key Implementation Notes:
// - parse_module(): Enforces module = { item }, rejects non-item statements
// - Recursive descent: Each non-terminal becomes a parse_* method
// - Error recovery: Parser attempts to continue after errors for better diagnostics
// - Diagnostics: All errors recorded in m_diagnostics with source positions
//
// See GRAMMAR.md for the complete EBNF specification.

#include "parser.hpp"

#include <cctype>
#include <format>

namespace life_lang::parser {

Parser::Parser(std::string_view source_, std::string filename_)
    : m_source(source_), m_diagnostics(std::move(filename_), std::string(source_)) {}

char Parser::peek() const {
  if (m_pos >= m_source.size()) {
    return '\0';
  }
  return m_source[m_pos];
}

char Parser::peek(std::size_t offset_) const {
  std::size_t const pos = m_pos + offset_;
  if (pos >= m_source.size()) {
    return '\0';
  }
  return m_source[pos];
}

char Parser::advance() {
  if (m_pos >= m_source.size()) {
    return '\0';
  }

  char const current = m_source[m_pos];
  ++m_pos;

  // Track line and column
  // Handle different line endings: \n, \r\n, \r
  if (current == '\n') {
    ++m_line;
    m_column = 1;
  } else if (current == '\r') {
    // Treat \r as newline (handles both \r and \r\n)
    // If followed by \n, the \n will be consumed but won't increment line again
    if (peek() == '\n') {
      ++m_pos;  // Consume the \n that follows \r
    }
    ++m_line;
    m_column = 1;
  } else {
    ++m_column;
  }

  return current;
}

void Parser::skip_whitespace_and_comments() {
  while (true) {
    char const current = peek();

    // Skip whitespace
    if (std::isspace(static_cast<unsigned char>(current)) != 0) {
      advance();
      continue;
    }

    // Skip line comments (//)
    if (current == '/' && peek(1) == '/') {
      advance();  // consume first /
      advance();  // consume second /
      // Skip until newline or EOF
      while (peek() != '\n' && peek() != '\0') {
        advance();
      }
      continue;
    }

    // Skip block comments (/* ... */)
    if (current == '/' && peek(1) == '*') {
      advance();  // consume /
      advance();  // consume *

      // Track nesting level for nested block comments
      int nesting = 1;
      while (nesting > 0 && peek() != '\0') {
        if (peek() == '/' && peek(1) == '*') {
          advance();
          advance();
          ++nesting;
        } else if (peek() == '*' && peek(1) == '/') {
          advance();
          advance();
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

Source_Position Parser::current_position() const {
  return Source_Position{.line = m_line, .column = m_column};
}

Source_Range Parser::make_range(Source_Position start_) const {
  return Source_Range{.start = start_, .end = current_position()};
}

void Parser::error(std::string message_, Source_Range range_) {
  m_diagnostics.add_error(range_, std::move(message_));
}

void Parser::error(std::string message_) {
  auto const pos = current_position();
  error(std::move(message_), Source_Range{.start = pos, .end = pos});
}

bool Parser::expect(char ch_) {
  skip_whitespace_and_comments();

  if (peek() != ch_) {
    error(std::format("Expected '{}', found '{}'", ch_, peek()));
    return false;
  }

  advance();
  return true;
}

bool Parser::expect(std::string_view str_) {
  skip_whitespace_and_comments();

  for (std::size_t i = 0; i < str_.size(); ++i) {
    if (peek(i) != str_[i]) {
      error(std::format("Expected '{}'", str_));
      return false;
    }
  }

  // Consume the string
  for (std::size_t i = 0; i < str_.size(); ++i) {
    advance();
  }

  return true;
}

bool Parser::match_keyword(std::string_view keyword_) {
  skip_whitespace_and_comments();

  // Check if keyword matches
  for (std::size_t i = 0; i < keyword_.size(); ++i) {
    if (peek(i) != keyword_[i]) {
      return false;
    }
  }

  // Ensure it's followed by a non-identifier character
  char const next = peek(keyword_.size());
  if (is_identifier_continue(next)) {
    return false;
  }

  // Consume the keyword
  for (std::size_t i = 0; i < keyword_.size(); ++i) {
    advance();
  }

  return true;
}

bool Parser::match_operator(std::string_view op_) {
  skip_whitespace_and_comments();

  // Check if operator matches
  for (std::size_t i = 0; i < op_.size(); ++i) {
    if (peek(i) != op_[i]) {
      return false;
    }
  }

  // Consume the operator
  for (std::size_t i = 0; i < op_.size(); ++i) {
    advance();
  }

  return true;
}

bool Parser::lookahead(std::string_view str_) const {
  for (std::size_t i = 0; i < str_.size(); ++i) {
    if (peek(i) != str_[i]) {
      return false;
    }
  }
  return true;
}

bool Parser::is_identifier_start(char ch_) {
  return std::isalpha(static_cast<unsigned char>(ch_)) != 0 || ch_ == '_';
}

bool Parser::is_identifier_continue(char ch_) {
  return std::isalnum(static_cast<unsigned char>(ch_)) != 0 || ch_ == '_';
}

Expected<ast::Module, Diagnostic_Engine> Parser::parse_module() {
  skip_whitespace_and_comments();

  std::vector<ast::Statement> statements;

  while (m_pos < m_source.size()) {
    skip_whitespace_and_comments();

    if (m_pos >= m_source.size()) {
      break;
    }

    // Module-level items must start with a keyword (fn, struct, enum, impl, trait, type)
    // Reject arbitrary expressions/statements at module level
    auto const start_pos = current_position();
    auto const start_char = peek();

    if (start_char != '\0' && !lookahead("fn") && !lookahead("struct") && !lookahead("enum") && !lookahead("impl") &&
        !lookahead("trait") && !lookahead("type")) {
      error(
          "Expected module-level item (fn, struct, enum, impl, trait, or type), found unexpected content",
          make_range(start_pos)
      );
      return unexpected(std::move(m_diagnostics));
    }

    auto stmt = parse_statement();
    if (!stmt) {
      if (m_diagnostics.has_errors()) {
        return unexpected(std::move(m_diagnostics));
      }
      error("Expected statement or declaration at module level", make_range(current_position()));
      return unexpected(std::move(m_diagnostics));
    }

    statements.push_back(std::move(*stmt));
    skip_whitespace_and_comments();
  }

  if (m_diagnostics.has_errors()) {
    return unexpected(std::move(m_diagnostics));
  }

  ast::Module result;
  result.statements = std::move(statements);

  return result;
}

std::optional<ast::Integer> Parser::parse_integer() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();
  std::string value;
  std::optional<std::string> suffix;

  // Check for leading zero (only "0" is allowed, not "01", "02", etc.)
  if (peek() == '0') {
    if (std::isdigit(static_cast<unsigned char>(peek(1))) != 0 || peek(1) == '_') {
      error("Invalid integer: leading zero not allowed (except standalone '0')", make_range(start_pos));
      return std::nullopt;
    }
    value += advance();
  } else if (peek() >= '1' && peek() <= '9') {
    // Non-zero start - collect all digits and underscores
    char last_char = peek();
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0 || peek() == '_') {
      last_char = peek();
      char const ch = advance();
      if (ch != '_') {
        value += ch;
      }
    }
    // Check for trailing underscore
    if (last_char == '_') {
      error("Invalid integer: trailing underscore not allowed", make_range(start_pos));
      return std::nullopt;
    }
  } else {
    error("Expected integer literal", make_range(start_pos));
    return std::nullopt;
  }

  // Check for zero with trailing underscore (like "0_")
  if (value == "0" && peek() == '_') {
    advance();  // consume the '_'
    error("Invalid integer: trailing underscore not allowed", make_range(start_pos));
    return std::nullopt;
  }

  // Check for optional type suffix (I8, I16, I32, I64, U8, U16, U32, U64)
  if (peek() == 'I' || peek() == 'U') {
    suffix = std::string(1, advance());
    if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
      error("Expected digit after type suffix", make_range(start_pos));
      return std::nullopt;
    }
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
      *suffix += advance();
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();
  std::string value;
  std::optional<std::string> suffix;

  // Float requires digits before or after dot (or both)
  // Also supports scientific notation (e/E)

  // Collect digits before dot (if any)
  char last_char_before_dot = '\0';
  while (std::isdigit(static_cast<unsigned char>(peek())) != 0 || peek() == '_') {
    last_char_before_dot = peek();
    char const ch = advance();
    if (ch != '_') {
      value += ch;
    }
  }

  // Must have dot or exponent
  bool has_dot = false;
  bool has_exponent = false;

  if (peek() == '.') {
    // Check for trailing underscore before dot
    if (last_char_before_dot == '_') {
      error("Invalid float: underscore before decimal point", make_range(start_pos));
      return std::nullopt;
    }

    has_dot = true;
    value += advance();  // consume '.'

    // Collect fractional digits
    char last_char_after_dot = '\0';
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0 || peek() == '_') {
      last_char_after_dot = peek();
      char const ch = advance();
      if (ch != '_') {
        value += ch;
      }
    }
    // Check for trailing underscore after fractional part
    if (last_char_after_dot == '_') {
      error("Invalid float: trailing underscore after decimal", make_range(start_pos));
      return std::nullopt;
    }
  }

  // Check for exponent (e or E)
  if (peek() == 'e' || peek() == 'E') {
    // Check for trailing underscore before exponent (handled above)
    // (This check is redundant now but kept for clarity)

    has_exponent = true;
    value += advance();  // consume 'e' or 'E'

    // Optional sign
    if (peek() == '+' || peek() == '-') {
      value += advance();
    }

    // Check for leading underscore after e/E or sign
    if (peek() == '_') {
      error("Invalid float: underscore after exponent marker", make_range(start_pos));
      return std::nullopt;
    }

    // Collect exponent digits
    if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
      error("Expected digits after exponent", make_range(start_pos));
      return std::nullopt;
    }

    char last_char_in_exponent = '\0';
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0 || peek() == '_') {
      last_char_in_exponent = peek();
      char const ch = advance();
      if (ch != '_') {
        value += ch;
      }
    }
    // Check for trailing underscore in exponent
    if (last_char_in_exponent == '_') {
      error("Invalid float: trailing underscore in exponent", make_range(start_pos));
      return std::nullopt;
    }
  }

  // Must have at least dot or exponent to be a float
  if (!has_dot && !has_exponent) {
    error("Expected float literal", make_range(start_pos));
    return std::nullopt;
  }

  // Check for optional type suffix (F32, F64)
  if (peek() == 'F') {
    suffix = std::string(1, advance());
    if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
      error("Expected digit after type suffix", make_range(start_pos));
      return std::nullopt;
    }
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
      *suffix += advance();
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (peek() != '"') {
    error("Expected string literal", make_range(start_pos));
    return std::nullopt;
  }

  std::string value;
  value += advance();  // consume opening quote

  while (peek() != '"' && peek() != '\0') {
    if (peek() == '\\') {
      // Escape sequence
      value += advance();  // consume backslash
      if (peek() == '\0') {
        error("Unterminated string literal", make_range(start_pos));
        return std::nullopt;
      }
      value += advance();  // consume escaped character
    } else {
      value += advance();
    }
  }

  if (peek() != '"') {
    error("Unterminated string literal", make_range(start_pos));
    return std::nullopt;
  }

  value += advance();  // consume closing quote

  // Create AST node (stores with quotes)
  ast::String result;
  result.value = std::move(value);

  return result;
}

std::optional<ast::Char> Parser::parse_char() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (peek() != '\'') {
    error("Expected character literal", make_range(start_pos));
    return std::nullopt;
  }

  std::string value;
  value += advance();  // consume opening quote

  if (peek() == '\'') {
    error("Empty character literal", make_range(start_pos));
    return std::nullopt;
  }

  if (peek() == '\\') {
    // Escape sequence
    value += advance();  // consume backslash
    if (peek() == '\0') {
      error("Unterminated character literal", make_range(start_pos));
      return std::nullopt;
    }

    char const escape_char = peek();
    value += advance();  // consume escape type char

    // Handle multi-character escapes
    if (escape_char == 'x') {
      // Hex escape: \xHH (exactly 2 hex digits)
      for (int i = 0; i < 2; i++) {
        if (peek() == '\0' || std::isxdigit(static_cast<unsigned char>(peek())) == 0) {
          error("Invalid hex escape sequence (expected 2 hex digits)", make_range(start_pos));
          return std::nullopt;
        }
        value += advance();
      }
    } else if (escape_char == 'u') {
      // Unicode escape: \u{HHHHHH} (1-6 hex digits in braces)
      if (peek() != '{') {
        error("Invalid unicode escape (expected '{')", make_range(start_pos));
        return std::nullopt;
      }
      value += advance();  // consume '{'

      int digit_count = 0;
      while (peek() != '}' && digit_count < 6) {
        if (std::isxdigit(static_cast<unsigned char>(peek())) == 0) {
          error("Invalid unicode escape (expected hex digit or '}')", make_range(start_pos));
          return std::nullopt;
        }
        value += advance();
        digit_count++;
      }

      if (digit_count == 0) {
        error("Invalid unicode escape (expected at least 1 hex digit)", make_range(start_pos));
        return std::nullopt;
      }

      if (peek() != '}') {
        error("Invalid unicode escape (expected '}')", make_range(start_pos));
        return std::nullopt;
      }
      value += advance();  // consume '}'
    }
    // For simple escapes like \n, \t, \', \", \\, we've already consumed the char
  } else {
    // Regular character (may be multi-byte UTF-8)
    char const first_byte = peek();
    value += advance();

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
        if (peek() == '\0') {
          error("Invalid UTF-8 sequence in character literal", make_range(start_pos));
          return std::nullopt;
        }
        value += advance();
      }
    }
  }

  if (peek() != '\'') {
    error("Unterminated character literal", make_range(start_pos));
    return std::nullopt;
  }

  value += advance();  // consume closing quote

  // Create AST node (stores with quotes)
  ast::Char result;
  result.value = std::move(value);

  return result;
}

std::optional<ast::Unit_Literal> Parser::parse_unit_literal() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (peek() != '(' || peek(1) != ')') {
    error("Expected unit literal '()'", make_range(start_pos));
    return std::nullopt;
  }

  advance();  // consume '('
  advance();  // consume ')'

  return ast::Unit_Literal{};
}

std::optional<ast::Struct_Literal> Parser::parse_struct_literal() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  // Parse type name (must start with uppercase)
  if (!is_identifier_start(peek()) || std::isupper(static_cast<unsigned char>(peek())) == 0) {
    error("Expected type name for struct literal", make_range(start_pos));
    return std::nullopt;
  }

  std::string type_name;
  type_name += advance();
  while (is_identifier_continue(peek())) {
    type_name += advance();
  }

  skip_whitespace_and_comments();

  // Expect '{'
  if (!expect('{')) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Field_Initializer> fields;

  // Parse field initializers
  // Empty struct literal is valid: Point {}
  if (peek() != '}') {
    while (true) {
      skip_whitespace_and_comments();

      // Parse field name
      if (!is_identifier_start(peek())) {
        error("Expected field name", make_range(current_position()));
        return std::nullopt;
      }

      std::string field_name;
      field_name += advance();
      while (is_identifier_continue(peek())) {
        field_name += advance();
      }

      skip_whitespace_and_comments();

      // Expect ':'
      if (!expect(':')) {
        return std::nullopt;
      }

      skip_whitespace_and_comments();

      // Parse field value expression
      auto value = parse_expr();
      if (!value) {
        error("Expected expression for field value", make_range(current_position()));
        return std::nullopt;
      }

      ast::Field_Initializer field;
      field.name = std::move(field_name);
      field.value = std::make_shared<ast::Expr>(std::move(*value));
      fields.push_back(std::move(field));

      skip_whitespace_and_comments();

      // Check for comma or end
      if (peek() == ',') {
        advance();  // consume ','
        skip_whitespace_and_comments();
        // Allow trailing comma
        if (peek() == '}') {
          break;
        }
        continue;
      }

      // No comma, must be end of fields
      break;
    }
  }

  skip_whitespace_and_comments();

  // Expect '}'
  if (!expect('}')) {
    return std::nullopt;
  }

  ast::Struct_Literal result;
  result.type_name = std::move(type_name);
  result.fields = std::move(fields);

  return result;
}

std::optional<ast::Var_Name> Parser::parse_variable_name() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();
  std::vector<ast::Var_Name_Segment> segments;

  // Parse first segment
  if (!is_identifier_start(peek())) {
    error("Expected identifier", make_range(start_pos));
    return std::nullopt;
  }

  std::string name;
  name += advance();
  while (is_identifier_continue(peek())) {
    name += advance();
  }

  // Check if it's a keyword (keywords can't be used as variable names)
  static std::array<std::string_view, 16> const keywords = {
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
      "in"
  };
  for (auto const& kw : keywords) {
    if (name == kw) {
      error(std::format("Cannot use keyword '{}' as variable name", name), make_range(start_pos));
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

// Parse qualified variable name: dotted path for function calls
// Examples: "foo", "Std.print", "Vec<Int>.new"
[[nodiscard]] std::optional<ast::Var_Name> Parser::parse_qualified_variable_name() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();
  std::vector<ast::Var_Name_Segment> segments;

  // Parse first segment (same as parse_variable_name, but then continue for more segments)
  if (!is_identifier_start(peek())) {
    error("Expected identifier", make_range(start_pos));
    return std::nullopt;
  }

  std::string name;
  name += advance();
  while (is_identifier_continue(peek())) {
    name += advance();
  }

  // Check for type parameters after first segment
  std::vector<ast::Type_Name> type_params;
  skip_whitespace_and_comments();
  if (peek() == '<') {
    advance();  // consume '<'

    // Parse type parameters
    while (true) {
      skip_whitespace_and_comments();
      auto type_param = parse_type_name();
      if (!type_param) {
        error("Expected type parameter", make_range(start_pos));
        return std::nullopt;
      }
      type_params.push_back(std::move(*type_param));

      skip_whitespace_and_comments();
      if (peek() == '>') {
        advance();  // consume '>'
        break;
      }
      if (peek() == ',') {
        advance();  // consume ','
        continue;
      }
      error("Expected ',' or '>' in type parameters", make_range(start_pos));
      return std::nullopt;
    }
  }

  ast::Var_Name_Segment segment;
  segment.value = std::move(name);
  segment.type_params = std::move(type_params);
  segments.push_back(std::move(segment));

  // Parse additional path segments (Std.IO.println)
  while (true) {
    skip_whitespace_and_comments();
    if (peek() != '.' || peek(1) == '.') {  // Stop at '..' (range operator)
      break;
    }
    advance();  // consume '.'

    skip_whitespace_and_comments();
    if (!is_identifier_start(peek())) {
      error("Expected identifier after '.'", make_range(start_pos));
      return std::nullopt;
    }

    std::string segment_name;
    segment_name += advance();
    while (is_identifier_continue(peek())) {
      segment_name += advance();
    }

    // Type parameters for path segments
    std::vector<ast::Type_Name> segment_type_params;
    skip_whitespace_and_comments();
    if (peek() == '<') {
      advance();  // consume '<'

      while (true) {
        skip_whitespace_and_comments();
        auto type_param = parse_type_name();
        if (!type_param) {
          error("Expected type parameter", make_range(start_pos));
          return std::nullopt;
        }
        segment_type_params.push_back(std::move(*type_param));

        skip_whitespace_and_comments();
        if (peek() == '>') {
          advance();  // consume '>'
          break;
        }
        if (peek() == ',') {
          advance();  // consume ','
          continue;
        }
        error("Expected ',' or '>' in type parameters", make_range(start_pos));
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
  skip_whitespace_and_comments();

  // Type_Name is a variant of Path_Type and Function_Type
  // Try function type first (starts with "fn(")
  // Use match_keyword to ensure "fn" is followed by non-identifier character
  if (peek() == 'f' && peek(1) == 'n' && !is_identifier_continue(peek(2))) {
    // Could be function type "fn(...)" or path type starting with something like "Fn"
    // Check if next non-whitespace is '(' to distinguish
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    advance();  // 'f'
    advance();  // 'n'
    skip_whitespace_and_comments();

    if (peek() == '(') {
      // Restore and parse as function type
      m_pos = saved_pos;
      m_line = saved_line;
      m_column = saved_col;
      skip_whitespace_and_comments();

      auto func_type = parse_function_type();
      if (func_type) {
        return ast::Type_Name{std::move(*func_type)};
      }
      return std::nullopt;
    }

    // Not a function type, restore and parse as path type
    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
    skip_whitespace_and_comments();
  }

  // Parse as path type
  auto path_type = parse_path_type();
  if (path_type) {
    return ast::Type_Name{std::move(*path_type)};
  }

  return std::nullopt;
}

std::optional<ast::Path_Type> Parser::parse_path_type() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  // Special case: unit type ()
  if (peek() == '(' && peek(1) == ')') {
    advance();  // consume '('
    advance();  // consume ')'

    ast::Type_Name_Segment segment;
    segment.value = "()";

    ast::Path_Type result;
    result.segments.push_back(std::move(segment));
    return result;
  }

  // Parse first segment
  if (!is_identifier_start(peek())) {
    error("Expected type name", make_range(start_pos));
    return std::nullopt;
  }

  std::vector<ast::Type_Name_Segment> segments;

  // Parse segments separated by '.'
  while (true) {
    skip_whitespace_and_comments();

    if (!is_identifier_start(peek())) {
      if (segments.empty()) {
        error("Expected type name", make_range(start_pos));
        return std::nullopt;
      }
      break;
    }

    std::string name;
    name += advance();
    while (is_identifier_continue(peek())) {
      name += advance();
    }

    // Parse optional type parameters <T, U>
    std::vector<ast::Type_Name> type_params;
    skip_whitespace_and_comments();
    if (peek() == '<') {
      advance();  // consume '<'

      while (true) {
        skip_whitespace_and_comments();
        auto type_param = parse_type_name();
        if (!type_param) {
          error("Expected type parameter", make_range(start_pos));
          return std::nullopt;
        }
        type_params.push_back(std::move(*type_param));

        skip_whitespace_and_comments();
        if (peek() == '>') {
          advance();  // consume '>'
          break;
        }
        if (peek() == ',') {
          advance();  // consume ','
          continue;
        }
        error("Expected ',' or '>' in type parameters", make_range(start_pos));
        return std::nullopt;
      }
    }

    ast::Type_Name_Segment segment;
    segment.value = std::move(name);
    segment.type_params = std::move(type_params);
    segments.push_back(std::move(segment));

    // Check for '.' to continue path
    skip_whitespace_and_comments();
    if (peek() == '.' && peek(1) != '.') {
      advance();  // consume '.'
      continue;
    }
    break;
  }

  if (segments.empty()) {
    error("Expected type name", make_range(start_pos));
    return std::nullopt;
  }

  ast::Path_Type result;
  result.segments = std::move(segments);
  return result;
}

std::optional<ast::Function_Type> Parser::parse_function_type() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("fn")) {
    error("Expected 'fn' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect('(')) {
    return std::nullopt;
  }

  // Parse parameter types
  std::vector<ast::Type_Name> param_types;
  skip_whitespace_and_comments();
  if (peek() != ')') {
    while (true) {
      skip_whitespace_and_comments();
      auto param_type = parse_type_name();
      if (!param_type) {
        error("Expected parameter type", make_range(start_pos));
        return std::nullopt;
      }
      param_types.push_back(std::move(*param_type));

      skip_whitespace_and_comments();
      if (peek() == ')') {
        break;
      }
      if (peek() == ',') {
        advance();  // consume ','
        continue;
      }
      error("Expected ',' or ')' in function type", make_range(start_pos));
      return std::nullopt;
    }
  }

  if (!expect(')')) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(':')) {
    return std::nullopt;
  }

  // Parse return type
  skip_whitespace_and_comments();
  auto return_type = parse_type_name();
  if (!return_type) {
    error("Expected return type", make_range(start_pos));
    return std::nullopt;
  }

  // Create Function_Type
  // Note: Function_Type uses forward_ast for recursive types in Spirit X3
  // We need to handle this properly
  ast::Function_Type result;

  // Convert vector of Type_Name to vector of forward_ast<Type_Name>
  for (auto& param : param_types) {
    result.param_types.push_back(std::make_shared<ast::Type_Name>(std::move(param)));
  }

  result.return_type = std::make_shared<ast::Type_Name>(std::move(*return_type));

  return result;
}

std::optional<ast::Type_Param> Parser::parse_type_param() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  // Parse type parameter name (simple identifier, not a path)
  auto name = parse_type_name();
  if (!name) {
    error("Expected type parameter name", make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional trait bounds (: Trait1 + Trait2)
  std::vector<ast::Trait_Bound> bounds;
  skip_whitespace_and_comments();
  if (peek() == ':') {
    advance();  // consume ':'

    while (true) {
      skip_whitespace_and_comments();
      auto trait_name = parse_type_name();
      if (!trait_name) {
        error("Expected trait name", make_range(start_pos));
        return std::nullopt;
      }

      ast::Trait_Bound bound;
      bound.trait_name = std::move(*trait_name);
      bounds.push_back(std::move(bound));

      skip_whitespace_and_comments();
      if (peek() == '+') {
        advance();  // consume '+'
        continue;
      }
      break;
    }
  }

  ast::Type_Param result;
  result.name = std::move(*name);
  result.bounds = std::move(bounds);

  return result;
}

std::optional<ast::Where_Clause> Parser::parse_where_clause() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  // Note: Caller should have already matched 'where' keyword
  // This function parses the predicates only

  std::vector<ast::Where_Predicate> predicates;

  while (true) {
    skip_whitespace_and_comments();

    // Parse type being constrained
    auto type_name = parse_type_name();
    if (!type_name) {
      error("Expected type name in where clause", make_range(start_pos));
      return std::nullopt;
    }

    // Parse trait bounds (: Trait1 + Trait2)
    std::vector<ast::Trait_Bound> bounds;
    skip_whitespace_and_comments();
    if (peek() == ':') {
      advance();  // consume ':'

      while (true) {
        skip_whitespace_and_comments();
        auto trait_name = parse_type_name();
        if (!trait_name) {
          error("Expected trait name", make_range(start_pos));
          return std::nullopt;
        }

        ast::Trait_Bound bound;
        bound.trait_name = std::move(*trait_name);
        bounds.push_back(std::move(bound));

        skip_whitespace_and_comments();
        if (peek() == '+') {
          advance();  // consume '+'
          continue;
        }
        break;
      }
    }

    ast::Where_Predicate predicate;
    predicate.type_name = std::move(*type_name);
    predicate.bounds = std::move(bounds);
    predicates.push_back(std::move(predicate));

    // Check for more predicates
    skip_whitespace_and_comments();
    if (peek() == ',') {
      advance();  // consume ','
      continue;
    }
    break;
  }

  ast::Where_Clause result;
  result.predicates = std::move(predicates);

  return result;
}

[[nodiscard]] int Parser::get_precedence(ast::Binary_Op op_) {
  // Precedence levels (higher = tighter binding):
  // 1: || (logical OR)
  // 2: && (logical AND)
  // 3: ==, != (equality)
  // 4: <, >, <=, >= (comparison)
  // 5: +, - (additive)
  // 6: *, /, % (multiplicative)

  switch (op_) {
    case ast::Binary_Op::Or:
      return 1;
    case ast::Binary_Op::And:
      return 2;
    case ast::Binary_Op::Eq:
    case ast::Binary_Op::Ne:
      return 3;
    case ast::Binary_Op::Lt:
    case ast::Binary_Op::Gt:
    case ast::Binary_Op::Le:
    case ast::Binary_Op::Ge:
      return 4;
    case ast::Binary_Op::Add:
    case ast::Binary_Op::Sub:
      return 5;
    case ast::Binary_Op::Mul:
    case ast::Binary_Op::Div:
    case ast::Binary_Op::Mod:
      return 6;
  }
  return 0;
}

[[nodiscard]] std::optional<ast::Binary_Op> Parser::try_parse_binary_op() {
  skip_whitespace_and_comments();

  // Two-character operators first
  if (peek() == '=' && peek(1) == '=') {
    advance();
    advance();
    return ast::Binary_Op::Eq;
  }
  if (peek() == '!' && peek(1) == '=') {
    advance();
    advance();
    return ast::Binary_Op::Ne;
  }
  if (peek() == '<' && peek(1) == '=') {
    advance();
    advance();
    return ast::Binary_Op::Le;
  }
  if (peek() == '>' && peek(1) == '=') {
    advance();
    advance();
    return ast::Binary_Op::Ge;
  }
  if (peek() == '&' && peek(1) == '&') {
    advance();
    advance();
    return ast::Binary_Op::And;
  }
  if (peek() == '|' && peek(1) == '|') {
    advance();
    advance();
    return ast::Binary_Op::Or;
  }

  // Single-character operators
  switch (peek()) {
    case '+':
      advance();
      return ast::Binary_Op::Add;
    case '-':
      // Could be subtract or range operator (..)
      if (peek(1) != '.') {
        advance();
        return ast::Binary_Op::Sub;
      }
      return std::nullopt;
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
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<ast::Unary_Op> Parser::try_parse_unary_op() {
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

std::optional<ast::Expr> Parser::parse_primary_expr() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  // Try control flow expressions first (they start with keywords)
  if (lookahead("if")) {
    auto if_expr = parse_if_expr();
    if (if_expr) {
      return ast::Expr{std::make_shared<ast::If_Expr>(std::move(*if_expr))};
    }
  }

  if (lookahead("while")) {
    auto while_expr = parse_while_expr();
    if (while_expr) {
      return ast::Expr{std::make_shared<ast::While_Expr>(std::move(*while_expr))};
    }
  }

  if (lookahead("for")) {
    auto for_expr = parse_for_expr();
    if (for_expr) {
      return ast::Expr{std::make_shared<ast::For_Expr>(std::move(*for_expr))};
    }
  }

  if (lookahead("match")) {
    auto match_expr = parse_match_expr();
    if (match_expr) {
      return ast::Expr{std::make_shared<ast::Match_Expr>(std::move(*match_expr))};
    }
  }

  // Try block
  if (peek() == '{') {
    auto block = parse_block();
    if (block) {
      return ast::Expr{std::make_shared<ast::Block>(std::move(*block))};
    }
  }

  // Try literals

  // Try integer or float
  if (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
    // Need to distinguish between integer and float
    // Look ahead for decimal point or exponent
    bool is_float = false;
    for (std::size_t i = 0; peek(i) != '\0'; ++i) {
      char const ch = peek(i);
      if (ch == '.' && peek(i + 1) != '.') {
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
      auto float_lit = parse_float();
      if (float_lit) {
        return ast::Expr{std::move(*float_lit)};
      }
    } else {
      auto integer = parse_integer();
      if (integer) {
        return ast::Expr{std::move(*integer)};
      }
    }
  }

  // Try string
  if (peek() == '"') {
    auto string = parse_string();
    if (string) {
      return ast::Expr{std::move(*string)};
    }
  }

  // Try char
  if (peek() == '\'') {
    auto char_lit = parse_char();
    if (char_lit) {
      return ast::Expr{std::move(*char_lit)};
    }
  }

  // Try unit literal () or parenthesized expression
  if (peek() == '(') {
    if (peek(1) == ')') {
      auto unit = parse_unit_literal();
      if (unit) {
        return ast::Expr{*unit};
      }
    } else {
      advance();  // consume '('
      auto expr = parse_expr();
      if (!expr) {
        error("Expected expression", make_range(start_pos));
        return std::nullopt;
      }
      skip_whitespace_and_comments();
      if (!expect(')')) {
        return std::nullopt;
      }
      return expr;
    }
  }

  // Try struct literal or variable name / qualified function call
  if (is_identifier_start(peek())) {
    // Look ahead to determine:
    //   - TypeName { ... }  → struct literal (uppercase start + brace)
    //   - a.b.c()  → qualified function call (dotted.path(...))
    //   - a.b.c    → field access (parse single segment, postfix handles dots)
    //   - a()      → simple function call (parse single segment)

    // Check if this is a struct literal (uppercase identifier followed by '{')
    bool is_struct_literal = false;
    if (std::isupper(static_cast<unsigned char>(peek())) != 0) {
      std::size_t look_ahead = 0;
      while (is_identifier_start(peek(look_ahead)) || is_identifier_continue(peek(look_ahead))) {
        look_ahead++;
      }

      // Skip whitespace
      while (peek(look_ahead) == ' ' || peek(look_ahead) == '\t' || peek(look_ahead) == '\n' ||
             peek(look_ahead) == '\r') {
        look_ahead++;
      }

      if (peek(look_ahead) == '{') {
        is_struct_literal = true;
      }
    }

    if (is_struct_literal) {
      auto struct_lit = parse_struct_literal();
      if (struct_lit) {
        return ast::Expr{std::move(*struct_lit)};
      }
    }

    bool is_qualified_call = false;
    std::size_t look_ahead = 0;

    // Skip first identifier
    while (is_identifier_start(peek(look_ahead)) || is_identifier_continue(peek(look_ahead))) {
      look_ahead++;
    }

    // Skip optional type parameters
    if (peek(look_ahead) == '<') {
      int depth = 1;
      look_ahead++;
      while (depth > 0 && peek(look_ahead) != '\0') {
        if (peek(look_ahead) == '<') {
          depth++;
        }
        if (peek(look_ahead) == '>') {
          depth--;
        }
        look_ahead++;
      }
    }

    // Check for dotted path pattern
    while (peek(look_ahead) == '.' && peek(look_ahead + 1) != '.') {
      look_ahead++;  // skip '.'

      // Skip identifier
      if (!is_identifier_start(peek(look_ahead))) {
        break;
      }
      while (is_identifier_start(peek(look_ahead)) || is_identifier_continue(peek(look_ahead))) {
        look_ahead++;
      }

      // Skip optional type parameters
      if (peek(look_ahead) == '<') {
        int depth = 1;
        look_ahead++;
        while (depth > 0 && peek(look_ahead) != '\0') {
          if (peek(look_ahead) == '<') {
            depth++;
          }
          if (peek(look_ahead) == '>') {
            depth--;
          }
          look_ahead++;
        }
      }
    }

    // If we found a dotted path and it's followed by '(', it's a qualified call
    if (peek(look_ahead) == '(') {
      // Count how many segments we saw
      std::size_t segment_count = 1;  // at least one segment
      for (std::size_t i = 0; i < look_ahead; i++) {
        if (peek(i) == '.' && peek(i + 1) != '.') {
          segment_count++;
        }
      }
      is_qualified_call = segment_count > 1;
    }

    if (is_qualified_call) {
      auto var_name = parse_qualified_variable_name();
      if (var_name) {
        return ast::Expr{std::move(*var_name)};
      }
    } else {
      auto var_name = parse_variable_name();
      if (var_name) {
        return ast::Expr{std::move(*var_name)};
      }
    }
  }

  error("Expected expression", make_range(start_pos));
  return std::nullopt;
}

[[nodiscard]] std::optional<ast::Expr> Parser::parse_expr() {
  // Try assignment first (lowest precedence, right-associative)
  // Assignment: non_assignment '=' expr
  auto const saved_pos = m_pos;
  auto const saved_line = m_line;
  auto const saved_col = m_column;

  // Parse left-hand side (non-assignment expression)
  auto lhs = parse_binary_expr(0);
  if (!lhs) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Check for assignment operator '=' (but not '==' or '=>')
  if (peek() == '=' && peek(1) != '=' && peek(1) != '>') {
    advance();  // consume '='

    // Parse right-hand side (full expression, allows nested assignments)
    auto rhs = parse_expr();
    if (!rhs) {
      error("Expected expression after '='");
      m_pos = saved_pos;
      m_line = saved_line;
      m_column = saved_col;
      return std::nullopt;
    }

    // Build assignment expression
    ast::Assignment_Expr assignment;
    assignment.target = std::make_shared<ast::Expr>(std::move(*lhs));
    assignment.value = std::make_shared<ast::Expr>(std::move(*rhs));

    return ast::Expr{std::make_shared<ast::Assignment_Expr>(std::move(assignment))};
  }

  // No assignment, return the binary expression
  return lhs;
}

[[nodiscard]] std::optional<ast::Expr> Parser::parse_unary_expr() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  // Try unary operator
  auto unary_op = try_parse_unary_op();
  if (unary_op) {
    auto operand = parse_unary_expr();  // Right associative
    if (!operand) {
      error("Expected expression after unary operator", make_range(start_pos));
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
    skip_whitespace_and_comments();

    // Check for range operators (.., ..=) before binary operators
    // Ranges have low precedence (between logical OR and assignment)
    if (peek() == '.' && peek(1) == '.') {
      int const range_precedence = 0;  // Lower than all binary ops
      if (range_precedence < min_precedence_) {
        break;
      }

      advance();  // consume first '.'
      advance();  // consume second '.'

      bool inclusive = false;
      if (peek() == '=') {
        advance();  // consume '='
        inclusive = true;
      }

      // Parse right-hand side (end of range)
      skip_whitespace_and_comments();
      auto rhs = parse_binary_expr(range_precedence + 1);
      if (!rhs) {
        error("Expected expression after range operator");
        return std::nullopt;
      }

      // Build range expression
      ast::Range_Expr range;
      range.start = std::make_shared<ast::Expr>(std::move(*lhs));
      range.end = std::make_shared<ast::Expr>(std::move(*rhs));
      range.inclusive = inclusive;

      lhs = ast::Expr{std::make_shared<ast::Range_Expr>(std::move(range))};
      continue;
    }

    // Try to parse binary operator
    auto const saved_pos = m_pos;
    auto const saved_line = m_line;
    auto const saved_column = m_column;

    auto op = try_parse_binary_op();
    if (!op) {
      break;
    }

    int const precedence = get_precedence(*op);
    if (precedence < min_precedence_) {
      // Restore position - operator has lower precedence
      m_pos = saved_pos;
      m_line = saved_line;
      m_column = saved_column;
      break;
    }

    // Parse right-hand side with higher precedence (left associative)
    auto rhs = parse_binary_expr(precedence + 1);
    if (!rhs) {
      error("Expected expression after binary operator");
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
    skip_whitespace_and_comments();

    // Field access: expr.field
    if (peek() == '.' && peek(1) != '.') {
      advance();  // consume '.'
      skip_whitespace_and_comments();

      if (!is_identifier_start(peek())) {
        error("Expected field name after '.'");
        return std::nullopt;
      }

      std::string field_name;
      field_name += advance();
      while (is_identifier_continue(peek())) {
        field_name += advance();
      }

      ast::Field_Access_Expr field_access;
      field_access.object = std::make_shared<ast::Expr>(std::move(*expr));
      field_access.field_name = std::move(field_name);

      expr = ast::Expr{std::make_shared<ast::Field_Access_Expr>(std::move(field_access))};
      continue;
    }

    // Function call: expr(args)
    if (peek() == '(') {
      advance();  // consume '('

      std::vector<ast::Expr> params;
      skip_whitespace_and_comments();

      if (peek() != ')') {
        while (true) {
          auto param = parse_expr();
          if (!param) {
            error("Expected expression in function call");
            return std::nullopt;
          }
          params.push_back(std::move(*param));

          skip_whitespace_and_comments();
          if (peek() == ')') {
            break;
          }
          if (peek() == ',') {
            advance();  // consume ','
            continue;
          }
          error("Expected ',' or ')' in function call");
          return std::nullopt;
        }
      }

      if (!expect(')')) {
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

      error("Function call target must be a variable name or field access");
      return std::nullopt;
    }

    // No more postfix operations
    break;
  }

  return expr;
}

std::optional<ast::Expr> Parser::parse_field_access() {
  error("parse_field_access should not be called directly - use parse_postfix_expr");
  return std::nullopt;
}

std::optional<ast::Expr> Parser::parse_func_call() {
  error("parse_func_call should not be called directly - use parse_postfix_expr");
  return std::nullopt;
}

[[nodiscard]] std::optional<ast::If_Expr> Parser::parse_if_expr() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("if")) {
    error("Expected 'if' keyword", make_range(start_pos));
    return std::nullopt;
  }

  // Parse condition (no parentheses)
  skip_whitespace_and_comments();
  auto condition = parse_expr();
  if (!condition) {
    error("Expected condition after 'if'", make_range(start_pos));
    return std::nullopt;
  }

  // Parse then block
  skip_whitespace_and_comments();
  auto then_block = parse_block();
  if (!then_block) {
    error("Expected block after if condition", make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional else-if and else clauses
  std::vector<ast::Else_If_Clause> else_ifs;
  std::optional<std::shared_ptr<ast::Block>> else_block;

  while (true) {
    skip_whitespace_and_comments();

    if (!match_keyword("else")) {
      break;
    }

    skip_whitespace_and_comments();

    // Check for 'else if'
    if (match_keyword("if")) {
      skip_whitespace_and_comments();
      auto else_if_condition = parse_expr();
      if (!else_if_condition) {
        error("Expected condition after 'else if'", make_range(start_pos));
        return std::nullopt;
      }

      skip_whitespace_and_comments();
      auto else_if_block = parse_block();
      if (!else_if_block) {
        error("Expected block after else-if condition", make_range(start_pos));
        return std::nullopt;
      }

      ast::Else_If_Clause else_if;
      else_if.condition = std::make_shared<ast::Expr>(std::move(*else_if_condition));
      else_if.then_block = std::make_shared<ast::Block>(std::move(*else_if_block));
      else_ifs.push_back(std::move(else_if));
      continue;
    }

    // Just 'else'
    skip_whitespace_and_comments();
    auto final_else_block = parse_block();
    if (!final_else_block) {
      error("Expected block after 'else'", make_range(start_pos));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!expect('{')) {
    return std::nullopt;
  }

  std::vector<ast::Statement> statements;
  std::optional<std::shared_ptr<ast::Expr>> trailing_expr;

  while (true) {
    skip_whitespace_and_comments();

    if (peek() == '}') {
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
      skip_whitespace_and_comments();
      if (peek() == '}') {
        // This is the trailing expression
        trailing_expr = std::make_shared<ast::Expr>(std::move(*expr));
        break;
      }
      error("Expected ';' or '}' after expression", make_range(start_pos));
      return std::nullopt;
    }

    error("Expected statement or expression in block", make_range(start_pos));
    return std::nullopt;
  }

  if (!expect('}')) {
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("while")) {
    error("Expected 'while' keyword", make_range(start_pos));
    return std::nullopt;
  }

  // Parse condition (no parentheses)
  skip_whitespace_and_comments();
  auto condition = parse_expr();
  if (!condition) {
    error("Expected condition after 'while'", make_range(start_pos));
    return std::nullopt;
  }

  // Parse body block
  skip_whitespace_and_comments();
  auto body = parse_block();
  if (!body) {
    error("Expected block after while condition", make_range(start_pos));
    return std::nullopt;
  }

  ast::While_Expr result;
  result.condition = std::make_shared<ast::Expr>(std::move(*condition));
  result.body = std::make_shared<ast::Block>(std::move(*body));

  return result;
}

[[nodiscard]] std::optional<ast::For_Expr> Parser::parse_for_expr() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("for")) {
    error("Expected 'for' keyword", make_range(start_pos));
    return std::nullopt;
  }

  // Parse pattern (supports simple, tuple, struct patterns)
  skip_whitespace_and_comments();
  auto pattern = parse_pattern();
  if (!pattern) {
    error("Expected pattern after 'for'", make_range(start_pos));
    return std::nullopt;
  }

  // Validate that simple patterns are not keywords
  if (auto* simple = std::get_if<ast::Simple_Pattern>(&*pattern)) {
    static std::array<std::string_view, 16> const keywords = {
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
        "in"
    };
    for (auto const& kw : keywords) {
      if (simple->name == kw) {
        error("Cannot use keyword '" + simple->name + "' as pattern binding", make_range(start_pos));
        return std::nullopt;
      }
    }
  }

  // Parse 'in' keyword
  skip_whitespace_and_comments();
  if (!match_keyword("in")) {
    error("Expected 'in' keyword after for pattern", make_range(start_pos));
    return std::nullopt;
  }

  // Parse iterator expression
  skip_whitespace_and_comments();
  auto iterator = parse_expr();
  if (!iterator) {
    error("Expected expression after 'in'", make_range(start_pos));
    return std::nullopt;
  }

  // Parse body block
  skip_whitespace_and_comments();
  auto body = parse_block();
  if (!body) {
    error("Expected block after for iterator", make_range(start_pos));
    return std::nullopt;
  }

  ast::For_Expr result;
  result.pattern = std::move(*pattern);
  result.iterator = std::make_shared<ast::Expr>(std::move(*iterator));
  result.body = std::make_shared<ast::Block>(std::move(*body));

  return result;
}

[[nodiscard]] std::optional<ast::Range_Expr> Parser::parse_range_expr() {
  // Range expressions are parsed as part of binary expression parsing
  // This function should not be called directly
  error("parse_range_expr should not be called directly");
  return std::nullopt;
}

[[nodiscard]] std::optional<ast::Match_Expr> Parser::parse_match_expr() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("match")) {
    error("Expected 'match' keyword", make_range(start_pos));
    return std::nullopt;
  }

  // Parse scrutinee expression
  skip_whitespace_and_comments();
  auto scrutinee = parse_expr();
  if (!scrutinee) {
    error("Expected expression after 'match'", make_range(start_pos));
    return std::nullopt;
  }

  // Parse match arms in braces
  skip_whitespace_and_comments();
  if (!expect('{')) {
    return std::nullopt;
  }

  std::vector<ast::Match_Arm> arms;

  while (true) {
    skip_whitespace_and_comments();

    if (peek() == '}') {
      break;
    }

    // Parse pattern using full pattern parser
    auto pattern = parse_pattern();
    if (!pattern) {
      error("Expected pattern in match arm", make_range(start_pos));
      return std::nullopt;
    }

    // Optional guard (if condition)
    std::optional<std::shared_ptr<ast::Expr>> guard;
    skip_whitespace_and_comments();
    if (match_keyword("if")) {
      skip_whitespace_and_comments();
      auto guard_expr = parse_expr();
      if (!guard_expr) {
        error("Expected expression after 'if' in match guard", make_range(start_pos));
        return std::nullopt;
      }
      guard = std::make_shared<ast::Expr>(std::move(*guard_expr));
    }

    // Parse => arrow
    skip_whitespace_and_comments();
    if (!expect('=')) {
      return std::nullopt;
    }
    if (!expect('>')) {
      return std::nullopt;
    }

    // Parse result expression
    skip_whitespace_and_comments();
    auto result = parse_expr();
    if (!result) {
      error("Expected expression after '=>' in match arm", make_range(start_pos));
      return std::nullopt;
    }

    ast::Match_Arm arm;
    arm.pattern = std::move(*pattern);
    arm.guard = std::move(guard);
    arm.result = std::make_shared<ast::Expr>(std::move(*result));
    arms.push_back(std::move(arm));

    // Check for comma or closing brace
    skip_whitespace_and_comments();
    if (peek() == ',') {
      advance();
      continue;
    }
    if (peek() == '}') {
      break;
    }
    error("Expected ',' or '}' in match expression", make_range(start_pos));
    return std::nullopt;
  }

  if (!expect('}')) {
    return std::nullopt;
  }

  ast::Match_Expr result;
  result.scrutinee = std::make_shared<ast::Expr>(std::move(*scrutinee));
  result.arms = std::move(arms);

  return result;
}

// Statements

[[nodiscard]] std::optional<ast::Statement> Parser::parse_statement() {
  skip_whitespace_and_comments();

  // Try function definition
  if (lookahead("fn")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto func_def = parse_func_def();
    if (func_def) {
      return ast::Statement{std::make_shared<ast::Func_Def>(std::move(*func_def))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try struct definition
  if (lookahead("struct")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto struct_def = parse_struct_def();
    if (struct_def) {
      return ast::Statement{std::make_shared<ast::Struct_Def>(std::move(*struct_def))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try enum definition
  if (lookahead("enum")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto enum_def = parse_enum_def();
    if (enum_def) {
      return ast::Statement{std::make_shared<ast::Enum_Def>(std::move(*enum_def))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try trait definition or impl blocks (both start with keywords)
  if (lookahead("trait")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto trait_def = parse_trait_def();
    if (trait_def) {
      return ast::Statement{std::make_shared<ast::Trait_Def>(std::move(*trait_def))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try impl blocks - need to distinguish between trait impl and regular impl
  if (lookahead("impl")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto trait_impl = parse_trait_impl();
    if (trait_impl) {
      return ast::Statement{std::make_shared<ast::Trait_Impl>(std::move(*trait_impl))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;

    auto impl_block = parse_impl_block();
    if (impl_block) {
      return ast::Statement{std::make_shared<ast::Impl_Block>(std::move(*impl_block))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try type alias
  if (lookahead("type")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto type_alias = parse_type_alias();
    if (type_alias) {
      return ast::Statement{std::make_shared<ast::Type_Alias>(std::move(*type_alias))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try let statement
  if (lookahead("let")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto let_stmt = parse_let_statement();
    if (let_stmt) {
      return ast::Statement{std::make_shared<ast::Let_Statement>(std::move(*let_stmt))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try return statement
  if (lookahead("return")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto return_stmt = parse_return_statement();
    if (return_stmt) {
      return ast::Statement{std::move(*return_stmt)};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try break statement
  if (lookahead("break")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto break_stmt = parse_break_statement();
    if (break_stmt) {
      return ast::Statement{std::move(*break_stmt)};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try continue statement
  if (lookahead("continue")) {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto continue_stmt = parse_continue_statement();
    if (continue_stmt) {
      return ast::Statement{*continue_stmt};
    }

    // Restore position on failure
    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try block statement (nested blocks)
  if (peek() == '{') {
    auto saved_pos = m_pos;
    auto saved_line = m_line;
    auto saved_col = m_column;

    auto block = parse_block();
    if (block) {
      skip_whitespace_and_comments();
      // Block as statement doesn't need semicolon
      return ast::Statement{std::make_shared<ast::Block>(std::move(*block))};
    }

    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_col;
  }

  // Try expression - some expressions can be statements without semicolons
  // Save position in case expression parse succeeds but semicolon is missing (for trailing expressions in blocks)
  auto const expr_saved_pos = m_pos;
  auto const expr_saved_line = m_line;
  auto const expr_saved_col = m_column;

  auto expr = parse_expr();
  if (expr) {
    skip_whitespace_and_comments();

    // Check if this is a while expression - can be used as statement without semicolon
    if (auto* while_fwd = std::get_if<std::shared_ptr<ast::While_Expr>>(&*expr)) {
      ast::While_Statement while_stmt;
      while_stmt.expr = *while_fwd;
      return ast::Statement{std::make_shared<ast::While_Statement>(std::move(while_stmt))};
    }

    // Check if this is a for expression - can be used as statement without semicolon
    if (auto* for_fwd = std::get_if<std::shared_ptr<ast::For_Expr>>(&*expr)) {
      ast::For_Statement for_stmt;
      for_stmt.expr = *for_fwd;
      return ast::Statement{std::make_shared<ast::For_Statement>(std::move(for_stmt))};
    }

    // Check if this is an if expression - can be used as statement without semicolon
    if (auto* if_fwd = std::get_if<std::shared_ptr<ast::If_Expr>>(&*expr)) {
      ast::If_Statement if_stmt;
      if_stmt.expr = *if_fwd;
      return ast::Statement{std::make_shared<ast::If_Statement>(std::move(if_stmt))};
    }

    // Other expressions require semicolon
    skip_whitespace_and_comments();
    if (peek() == ';') {
      advance();  // consume ';'

      // Check if this is a function call - use Func_Call_Statement
      if (auto* func_call_fwd = std::get_if<std::shared_ptr<ast::Func_Call_Expr>>(&*expr)) {
        ast::Func_Call_Statement func_call_stmt;
        func_call_stmt.expr = **func_call_fwd;  // Dereference shared_ptr then copy
        return ast::Statement{std::move(func_call_stmt)};
      }

      // Otherwise use generic Expr_Statement
      ast::Expr_Statement expr_stmt;
      expr_stmt.expr = std::make_shared<ast::Expr>(std::move(*expr));
      return ast::Statement{std::make_shared<ast::Expr_Statement>(std::move(expr_stmt))};
    }

    // Semicolon missing - restore position so block parser can try parsing as trailing expression
    m_pos = expr_saved_pos;
    m_line = expr_saved_line;
    m_column = expr_saved_col;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<ast::Return_Statement> Parser::parse_return_statement() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("return")) {
    error("Expected 'return' keyword", make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional return expression
  skip_whitespace_and_comments();
  auto expr = parse_expr();
  if (!expr) {
    error("Expected expression after 'return'", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    return std::nullopt;
  }

  ast::Return_Statement result;
  result.expr = std::move(*expr);

  return result;
}

[[nodiscard]] std::optional<ast::Break_Statement> Parser::parse_break_statement() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("break")) {
    error("Expected 'break' keyword", make_range(start_pos));
    return std::nullopt;
  }

  // Parse optional break value
  std::optional<ast::Expr> value;
  skip_whitespace_and_comments();

  if (peek() != ';') {
    auto expr = parse_expr();
    if (expr) {
      value = std::move(*expr);
    }
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    return std::nullopt;
  }

  ast::Break_Statement result;
  result.value = std::move(value);

  return result;
}

[[nodiscard]] std::optional<ast::Continue_Statement> Parser::parse_continue_statement() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("continue")) {
    error("Expected 'continue' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    return std::nullopt;
  }

  return ast::Continue_Statement{};
}

[[nodiscard]] std::optional<ast::Func_Param> Parser::parse_func_param() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  bool is_mut = false;
  if (match_keyword("mut")) {
    is_mut = true;
    skip_whitespace_and_comments();
  }

  auto name_result = parse_variable_name();
  if (!name_result) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::optional<ast::Type_Name> type;
  // Check for optional type annotation
  if (peek() == ':') {
    advance();  // consume ':'
    skip_whitespace_and_comments();
    auto type_result = parse_type_name();
    if (!type_result) {
      error("Expected type annotation after ':' in parameter", make_range(start_pos));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("fn")) {
    error("Expected 'fn' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto name_result = parse_variable_name();
  if (!name_result) {
    error("Expected function name after 'fn'", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in function declaration", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  if (!expect('(')) {
    error("Expected '(' to start parameter list", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Func_Param> func_params;
  if (peek() != ')') {
    while (true) {
      auto param = parse_func_param();
      if (!param) {
        error("Expected function parameter", make_range(current_position()));
        return std::nullopt;
      }
      func_params.push_back(std::move(*param));

      skip_whitespace_and_comments();
      if (peek() == ',') {
        advance();
        skip_whitespace_and_comments();
      } else {
        break;
      }
    }
  }

  if (!expect(')')) {
    error("Expected ')' to close parameter list", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  if (!expect(':')) {
    error("Expected ':' before return type", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto return_type = parse_type_name();
  if (!return_type) {
    error("Expected return type after ':'", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::optional<ast::Where_Clause> where_clause;
  if (match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      error("Expected where clause after 'where'", make_range(current_position()));
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
  skip_whitespace_and_comments();

  auto decl = parse_func_decl();
  if (!decl) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto body = parse_block();
  if (!body) {
    error("Expected function body block", make_range(current_position()));
    return std::nullopt;
  }

  ast::Func_Def result;
  result.declaration = std::move(*decl);
  result.body = std::move(*body);

  return result;
}

[[nodiscard]] std::optional<ast::Struct_Field> Parser::parse_struct_field() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  auto name = parse_variable_name();
  if (!name) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(':')) {
    error("Expected ':' after field name", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto type = parse_type_name();
  if (!type) {
    error("Expected type after ':' in field declaration", make_range(current_position()));
    return std::nullopt;
  }

  ast::Struct_Field result;
  result.name = std::move(name->segments[0].value);
  result.type = std::move(*type);

  return result;
}

[[nodiscard]] std::optional<ast::Struct_Def> Parser::parse_struct_def() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("struct")) {
    error("Expected 'struct' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Parse struct name (just the identifier, not a full type name with type args)
  if (!is_identifier_start(peek())) {
    error("Expected struct name after 'struct'", make_range(current_position()));
    return std::nullopt;
  }

  std::string struct_name;
  struct_name += advance();
  while (is_identifier_continue(peek())) {
    struct_name += advance();
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in struct definition", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  std::optional<ast::Where_Clause> where_clause;
  if (match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      error("Expected where clause after 'where'", make_range(current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    skip_whitespace_and_comments();
  }

  if (!expect('{')) {
    error("Expected '{' to start struct body", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Struct_Field> fields;
  while (peek() != '}' && m_pos < m_source.size()) {
    auto field = parse_struct_field();
    if (!field) {
      error("Expected struct field", make_range(current_position()));
      return std::nullopt;
    }
    fields.push_back(std::move(*field));

    skip_whitespace_and_comments();
    if (peek() == ',') {
      advance();
      skip_whitespace_and_comments();
    } else if (peek() != '}') {
      error("Expected ',' or '}' after struct field", make_range(current_position()));
      return std::nullopt;
    }
  }

  if (!expect('}')) {
    error("Expected '}' to close struct body", make_range(current_position()));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  auto name = parse_type_name();
  if (!name) {
    return std::nullopt;
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    error("Enum variant name must be a simple type name", make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    error("Enum variant name must be a simple identifier", make_range(start_pos));
    return std::nullopt;
  }

  std::string variant_name = path.segments[0].value;

  skip_whitespace_and_comments();

  if (peek() == '(') {
    advance();
    skip_whitespace_and_comments();

    std::vector<ast::Type_Name> tuple_fields;
    if (peek() != ')') {
      while (true) {
        auto field_type = parse_type_name();
        if (!field_type) {
          error("Expected type in tuple variant", make_range(current_position()));
          return std::nullopt;
        }
        tuple_fields.push_back(std::move(*field_type));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
          // Allow trailing comma before ')'
          if (peek() == ')') {
            break;
          }
        } else {
          break;
        }
      }
    }

    if (!expect(')')) {
      error("Expected ')' to close tuple variant", make_range(current_position()));
      return std::nullopt;
    }

    ast::Tuple_Variant tuple_var;
    tuple_var.name = std::move(variant_name);
    tuple_var.tuple_fields = std::move(tuple_fields);

    return ast::Enum_Variant{std::move(tuple_var)};
  }
  if (peek() == '{') {
    advance();
    skip_whitespace_and_comments();

    std::vector<ast::Struct_Field> struct_fields;
    while (peek() != '}' && m_pos < m_source.size()) {
      auto field = parse_struct_field();
      if (!field) {
        error("Expected struct field in variant", make_range(current_position()));
        return std::nullopt;
      }
      struct_fields.push_back(std::move(*field));

      skip_whitespace_and_comments();
      if (peek() == ',') {
        advance();
        skip_whitespace_and_comments();
      } else if (peek() != '}') {
        error("Expected ',' or '}' after struct field", make_range(current_position()));
        return std::nullopt;
      }
    }

    if (!expect('}')) {
      error("Expected '}' to close struct variant", make_range(current_position()));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("enum")) {
    error("Expected 'enum' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Parse enum name (just the identifier, not a full type name with type args)
  if (!is_identifier_start(peek())) {
    error("Expected enum name after 'enum'", make_range(current_position()));
    return std::nullopt;
  }

  std::string enum_name;
  enum_name += advance();
  while (is_identifier_continue(peek())) {
    enum_name += advance();
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in enum definition", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  std::optional<ast::Where_Clause> where_clause;
  if (match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      error("Expected where clause after 'where'", make_range(current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    skip_whitespace_and_comments();
  }

  if (!expect('{')) {
    error("Expected '{' to start enum body", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Enum_Variant> variants;
  while (peek() != '}' && m_pos < m_source.size()) {
    auto variant = parse_enum_variant();
    if (!variant) {
      error("Expected enum variant", make_range(current_position()));
      return std::nullopt;
    }
    variants.push_back(std::move(*variant));

    skip_whitespace_and_comments();
    if (peek() == ',') {
      advance();
      skip_whitespace_and_comments();
    } else if (peek() != '}') {
      error("Expected ',' or '}' after enum variant", make_range(current_position()));
      return std::nullopt;
    }
  }

  if (!expect('}')) {
    error("Expected '}' to close enum body", make_range(current_position()));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("type")) {
    error("Expected 'type' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto name = parse_type_name();
  if (!name) {
    error("Expected associated type name after 'type'", make_range(current_position()));
    return std::nullopt;
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    error("Associated type name must be a simple type name", make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    error("Associated type name must be a simple identifier", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Trait_Bound> bounds;
  if (peek() == ':') {
    advance();
    skip_whitespace_and_comments();

    while (true) {
      auto bound_type = parse_type_name();
      if (!bound_type) {
        error("Expected trait bound after ':'", make_range(current_position()));
        return std::nullopt;
      }

      if (std::get_if<ast::Path_Type>(&*bound_type) == nullptr) {
        error("Trait bound must be a path type", make_range(start_pos));
        return std::nullopt;
      }

      ast::Trait_Bound bound;
      bound.trait_name = std::get<ast::Path_Type>(std::move(*bound_type));
      bounds.push_back(std::move(bound));

      skip_whitespace_and_comments();
      if (peek() == '+') {
        advance();
        skip_whitespace_and_comments();
      } else {
        break;
      }
    }
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    error("Expected ';' after associated type declaration", make_range(current_position()));
    return std::nullopt;
  }

  ast::Assoc_Type_Decl result;
  result.name = path.segments[0].value;
  result.bounds = std::move(bounds);

  return result;
}

[[nodiscard]] std::optional<ast::Trait_Def> Parser::parse_trait_def() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("trait")) {
    error("Expected 'trait' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Parse trait name (just the identifier, not type parameters)
  if (!is_identifier_start(peek())) {
    error("Expected trait name after 'trait'", make_range(current_position()));
    return std::nullopt;
  }

  std::string trait_name;
  trait_name += advance();
  while (is_identifier_continue(peek())) {
    trait_name += advance();
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in trait definition", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  std::optional<ast::Where_Clause> where_clause;
  if (match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      error("Expected where clause after 'where'", make_range(current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    skip_whitespace_and_comments();
  }

  if (!expect('{')) {
    error("Expected '{' to start trait body", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Assoc_Type_Decl> assoc_types;
  std::vector<ast::Func_Decl> methods;

  while (peek() != '}' && m_pos < m_source.size()) {
    auto const item_start = current_position();

    if (lookahead("type")) {
      auto assoc_type = parse_assoc_type_decl();
      if (!assoc_type) {
        error("Expected associated type declaration", make_range(current_position()));
        return std::nullopt;
      }
      assoc_types.push_back(std::move(*assoc_type));
    } else if (lookahead("fn")) {
      auto method = parse_func_decl();
      if (!method) {
        error("Expected method declaration", make_range(current_position()));
        return std::nullopt;
      }

      skip_whitespace_and_comments();
      if (!expect(';')) {
        error("Expected ';' after method declaration in trait", make_range(current_position()));
        return std::nullopt;
      }

      methods.push_back(std::move(*method));
    } else {
      error("Expected 'type' or 'fn' in trait body", make_range(item_start));
      return std::nullopt;
    }

    skip_whitespace_and_comments();
  }

  if (!expect('}')) {
    error("Expected '}' to close trait body", make_range(current_position()));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("type")) {
    error("Expected 'type' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Parse type alias name (just the identifier)
  if (!is_identifier_start(peek())) {
    error("Expected type alias name after 'type'", make_range(current_position()));
    return std::nullopt;
  }

  std::string alias_name;
  alias_name += advance();
  while (is_identifier_continue(peek())) {
    alias_name += advance();
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in type alias", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  if (!expect('=')) {
    error("Expected '=' in type alias definition", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto aliased_type = parse_type_name();
  if (!aliased_type) {
    error("Expected type after '=' in type alias", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    error("Expected ';' after type alias definition", make_range(current_position()));
    return std::nullopt;
  }

  ast::Type_Alias result;
  result.name = std::move(alias_name);
  result.type_params = std::move(type_params);
  result.aliased_type = std::move(*aliased_type);

  return result;
}

[[nodiscard]] std::optional<ast::Impl_Block> Parser::parse_impl_block() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("impl")) {
    error("Expected 'impl' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in impl block", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  auto type_name = parse_type_name();
  if (!type_name) {
    error("Expected type name in impl block", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::optional<ast::Where_Clause> where_clause;
  if (match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      error("Expected where clause after 'where'", make_range(current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    skip_whitespace_and_comments();
  }

  if (!expect('{')) {
    error("Expected '{' to start impl block body", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Func_Def> methods;
  while (peek() != '}' && m_pos < m_source.size()) {
    auto method = parse_func_def();
    if (!method) {
      error("Expected method definition in impl block", make_range(current_position()));
      return std::nullopt;
    }
    methods.push_back(std::move(*method));
    skip_whitespace_and_comments();
  }

  if (!expect('}')) {
    error("Expected '}' to close impl block body", make_range(current_position()));
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
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("type")) {
    error("Expected 'type' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto name = parse_type_name();
  if (!name) {
    error("Expected associated type name after 'type'", make_range(current_position()));
    return std::nullopt;
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    error("Associated type name must be a simple type name", make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    error("Associated type name must be a simple identifier", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect('=')) {
    error("Expected '=' in associated type implementation", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto type_value = parse_type_name();
  if (!type_value) {
    error("Expected type after '=' in associated type implementation", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    error("Expected ';' after associated type implementation", make_range(current_position()));
    return std::nullopt;
  }

  ast::Assoc_Type_Impl result;
  result.name = path.segments[0].value;
  result.type_value = std::move(*type_value);

  return result;
}

[[nodiscard]] std::optional<ast::Trait_Impl> Parser::parse_trait_impl() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("impl")) {
    error("Expected 'impl' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Type_Param> type_params;
  if (peek() == '<') {
    advance();
    skip_whitespace_and_comments();

    if (peek() != '>') {
      while (true) {
        auto param = parse_type_param();
        if (!param) {
          error("Expected type parameter in trait impl", make_range(current_position()));
          return std::nullopt;
        }
        type_params.push_back(std::move(*param));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect('>')) {
      error("Expected '>' to close type parameter list", make_range(current_position()));
      return std::nullopt;
    }
    skip_whitespace_and_comments();
  }

  auto trait_name = parse_type_name();
  if (!trait_name) {
    error("Expected trait name in trait impl", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  // Check for 'for' keyword - if not present, this is likely an impl block, not trait impl
  if (!lookahead("for")) {
    return std::nullopt;  // Not a trait impl, backtrack
  }

  advance();  // consume 'f'
  advance();  // consume 'o'
  advance();  // consume 'r'

  skip_whitespace_and_comments();
  auto type_name = parse_type_name();
  if (!type_name) {
    error("Expected type name after 'for' in trait impl", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::optional<ast::Where_Clause> where_clause;
  if (match_keyword("where")) {
    auto clause = parse_where_clause();
    if (!clause) {
      error("Expected where clause after 'where'", make_range(current_position()));
      return std::nullopt;
    }
    where_clause = std::move(*clause);
    skip_whitespace_and_comments();
  }

  if (!expect('{')) {
    error("Expected '{' to start trait impl body", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::vector<ast::Assoc_Type_Impl> assoc_type_impls;
  std::vector<ast::Func_Def> methods;

  while (peek() != '}' && m_pos < m_source.size()) {
    auto const item_start = current_position();

    if (lookahead("type")) {
      auto assoc_type = parse_assoc_type_impl();
      if (!assoc_type) {
        error("Expected associated type implementation", make_range(current_position()));
        return std::nullopt;
      }
      assoc_type_impls.push_back(std::move(*assoc_type));
    } else if (lookahead("fn")) {
      auto method = parse_func_def();
      if (!method) {
        error("Expected method definition", make_range(current_position()));
        return std::nullopt;
      }
      methods.push_back(std::move(*method));
    } else {
      error("Expected 'type' or 'fn' in trait impl body", make_range(item_start));
      return std::nullopt;
    }

    skip_whitespace_and_comments();
  }

  if (!expect('}')) {
    error("Expected '}' to close trait impl body", make_range(current_position()));
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

[[nodiscard]] std::optional<ast::Pattern> Parser::parse_pattern() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (peek() == '_') {
    advance();
    return ast::Pattern{ast::Wildcard_Pattern{}};
  }

  if (peek() == '(') {
    advance();
    skip_whitespace_and_comments();

    std::vector<std::shared_ptr<ast::Pattern>> elements;
    if (peek() != ')') {
      while (true) {
        auto element = parse_pattern();
        if (!element) {
          error("Expected pattern in tuple", make_range(current_position()));
          return std::nullopt;
        }
        elements.push_back(std::make_shared<ast::Pattern>(std::move(*element)));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect(')')) {
      error("Expected ')' to close tuple pattern", make_range(current_position()));
      return std::nullopt;
    }

    ast::Tuple_Pattern tuple_pat;
    tuple_pat.elements = std::move(elements);
    return ast::Pattern{std::move(tuple_pat)};
  }

  if (peek() == '"' || std::isdigit(peek()) != 0 || (peek() == '-' && std::isdigit(peek(1)) != 0)) {
    auto expr = parse_primary_expr();
    if (!expr) {
      error("Expected literal in pattern", make_range(current_position()));
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

  skip_whitespace_and_comments();

  if (peek() == '(') {
    advance();
    skip_whitespace_and_comments();

    std::vector<std::shared_ptr<ast::Pattern>> patterns;
    if (peek() != ')') {
      while (true) {
        auto pattern = parse_pattern();
        if (!pattern) {
          error("Expected pattern in enum variant", make_range(current_position()));
          return std::nullopt;
        }
        patterns.push_back(std::make_shared<ast::Pattern>(std::move(*pattern)));

        skip_whitespace_and_comments();
        if (peek() == ',') {
          advance();
          skip_whitespace_and_comments();
        } else {
          break;
        }
      }
    }

    if (!expect(')')) {
      error("Expected ')' to close enum pattern", make_range(current_position()));
      return std::nullopt;
    }

    ast::Enum_Pattern enum_pat;
    enum_pat.type_name = std::move(*name);
    enum_pat.patterns = std::move(patterns);
    return ast::Pattern{std::move(enum_pat)};
  }
  if (peek() == '{') {
    advance();
    skip_whitespace_and_comments();

    std::vector<ast::Field_Pattern> fields;
    while (peek() != '}' && m_pos < m_source.size()) {
      // Parse field name
      if (!is_identifier_start(peek())) {
        error("Expected field name in struct pattern", make_range(current_position()));
        return std::nullopt;
      }

      std::string field_name;
      field_name += advance();
      while (is_identifier_continue(peek())) {
        field_name += advance();
      }

      skip_whitespace_and_comments();

      ast::Pattern field_pattern;

      // Check for shorthand syntax (field without colon)
      if (peek() == ',' || peek() == '}') {
        // Shorthand: field name becomes both field name and pattern binding
        ast::Simple_Pattern simple;
        simple.name = field_name;
        field_pattern = ast::Pattern{std::move(simple)};
      } else if (peek() == ':') {
        // Full syntax: field: pattern
        advance();  // consume ':'
        skip_whitespace_and_comments();
        auto pattern = parse_pattern();
        if (!pattern) {
          error("Expected pattern after ':' in field pattern", make_range(current_position()));
          return std::nullopt;
        }
        field_pattern = std::move(*pattern);
      } else {
        error("Expected ':' or ',' or '}' after field name in pattern", make_range(current_position()));
        return std::nullopt;
      }

      ast::Field_Pattern field_pat;
      field_pat.name = std::move(field_name);
      field_pat.pattern = std::make_shared<ast::Pattern>(std::move(field_pattern));
      fields.push_back(std::move(field_pat));

      skip_whitespace_and_comments();
      if (peek() == ',') {
        advance();
        skip_whitespace_and_comments();
        // Allow trailing comma
        if (peek() == '}') {
          break;
        }
      } else if (peek() != '}') {
        error("Expected ',' or '}' after field pattern", make_range(current_position()));
        return std::nullopt;
      }
    }

    if (!expect('}')) {
      error("Expected '}' to close struct pattern", make_range(current_position()));
      return std::nullopt;
    }

    ast::Struct_Pattern struct_pat;
    struct_pat.type_name = std::move(*name);
    struct_pat.fields = std::move(fields);
    return ast::Pattern{std::move(struct_pat)};
  }

  if (std::get_if<ast::Path_Type>(&*name) == nullptr) {
    error("Pattern must be a simple name, not a function type", make_range(start_pos));
    return std::nullopt;
  }

  auto const& path = std::get<ast::Path_Type>(*name);
  if (path.segments.size() != 1 || !path.segments[0].type_params.empty()) {
    error("Simple pattern must be a single identifier without type parameters", make_range(start_pos));
    return std::nullopt;
  }

  ast::Simple_Pattern simple_pat;
  simple_pat.name = path.segments[0].value;
  return ast::Pattern{std::move(simple_pat)};
}

[[nodiscard]] std::optional<ast::Let_Statement> Parser::parse_let_statement() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();

  if (!match_keyword("let")) {
    error("Expected 'let' keyword", make_range(start_pos));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  bool is_mut = false;
  if (match_keyword("mut")) {
    is_mut = true;
    skip_whitespace_and_comments();
  }

  auto pattern = parse_pattern();
  if (!pattern) {
    error("Expected pattern after 'let'", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  std::optional<ast::Type_Name> type;
  if (peek() == ':') {
    advance();
    skip_whitespace_and_comments();
    auto type_result = parse_type_name();
    if (!type_result) {
      error("Expected type after ':' in let statement", make_range(current_position()));
      return std::nullopt;
    }
    type = std::move(*type_result);
    skip_whitespace_and_comments();
  }

  if (!expect('=')) {
    error("Expected '=' in let statement", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  auto value = parse_expr();
  if (!value) {
    error("Expected expression after '=' in let statement", make_range(current_position()));
    return std::nullopt;
  }

  skip_whitespace_and_comments();
  if (!expect(';')) {
    error("Expected ';' after let statement", make_range(current_position()));
    return std::nullopt;
  }

  ast::Let_Statement result;
  result.is_mut = is_mut;
  result.pattern = std::move(*pattern);
  result.type = std::move(type);
  result.value = std::make_shared<ast::Expr>(std::move(*value));

  return result;
}

[[nodiscard]] std::optional<ast::Assignment_Expr> Parser::parse_assignment() {
  skip_whitespace_and_comments();

  auto const start_pos = current_position();
  auto const saved_pos = m_pos;
  auto const saved_line = m_line;
  auto const saved_column = m_column;

  // Parse left-hand side (must be a valid lvalue: var name or field access)
  auto lhs = parse_postfix_expr();
  if (!lhs) {
    return std::nullopt;
  }

  skip_whitespace_and_comments();

  // Check for assignment operator
  if (peek() != '=') {
    // Restore position - not an assignment
    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_column;
    return std::nullopt;
  }

  // Make sure it's not '==' (equality comparison)
  if (peek(1) == '=') {
    // Restore position - not an assignment
    m_pos = saved_pos;
    m_line = saved_line;
    m_column = saved_column;
    return std::nullopt;
  }

  advance();  // consume '='

  // Parse right-hand side (any expression)
  skip_whitespace_and_comments();
  auto rhs = parse_expr();
  if (!rhs) {
    error("Expected expression after '=' in assignment", make_range(start_pos));
    return std::nullopt;
  }

  // Build assignment expression
  ast::Assignment_Expr assignment;
  assignment.target = std::make_shared<ast::Expr>(std::move(*lhs));
  assignment.value = std::make_shared<ast::Expr>(std::move(*rhs));

  return assignment;
}

}  // namespace life_lang::parser
