#include "rules.hpp"

#include <fmt/core.h>

#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

namespace life_lang::parser {
namespace x3 = boost::spirit::x3;

using Space_Type = x3::ascii::space_type;
using Error_Handler_Type = x3::error_handler<Iterator_Type>;
using Phrase_Context_Type = x3::phrase_parse_context<Space_Type>::type;
using Context_Type =
    x3::context<x3::error_handler_tag, std::reference_wrapper<Error_Handler_Type>, Phrase_Context_Type>;

// Error handler for Spirit X3 expectation failures (operator `>`)
// Logs errors to x3::error_handler stream for later extraction
struct Error_Handler {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(
      Iterator& /*a_first*/, Iterator const& /*a_last*/, Exception const& a_ex, Context const& a_context
  ) {
    auto& error_handler = x3::get<x3::error_handler_tag>(a_context);
    std::string const message = fmt::format("Error! Expecting: {} here:", a_ex.which());
    error_handler(a_ex.where(), message);
    return x3::error_handler_result::fail;
  }
};

using x3::lexeme;
using x3::raw;
using x3::ascii::alnum;
using x3::ascii::alpha;
using x3::ascii::char_;
using x3::ascii::digit;
using x3::ascii::lit;
using x3::ascii::lower;

struct Keyword_Symbols : x3::symbols<> {
  Keyword_Symbols() { add("fn")("let")("return"); }
} const k_keywords;

// Individual keyword parsers for specific grammar rules (improves error messages)
auto const k_kw_fn = lexeme[lit("fn") >> !(alnum | '_')];
[[maybe_unused]] auto const k_kw_let = lexeme[lit("let") >> !(alnum | '_')];
auto const k_kw_return = lexeme[lit("return") >> !(alnum | '_')];

// Reserved word rule: matches any registered keyword (for identifier validation)
auto const k_reserved = lexeme[k_keywords >> !(alnum | '_')];

// Identifier patterns (exclude keywords to prevent using reserved words as identifiers)
auto const k_snake_case = raw[lexeme[lower >> *(lower | digit | '_') >> !(alnum | '_')]] - k_reserved;

// === Path Rules ===
// Paths represent type names, namespaces, or qualified identifiers
// Examples:
//   Simple:            "Int", "String", "MyClass"
//   Qualified:         "Std.String", "Std.Collections.Array"
//   Simple Template:   "Array<Int>", "Map<String, Int>"
//   Nested Templates:  "Vec<Vec<Int>>", "Option<Result<T, Error>>"
//   Qualified Paths in Templates:
//                      "Map<Std.String, Int>"                    - qualified type as template param
//                      "Array<Data.Model.User>"                  - deeply nested path as param
//                      "Result<IO.Error, Data.Value>"            - multiple qualified params
//   Complex Mixed:     "Std.Collections.Map<Key.Type, Value.Type>"
//                      "Network.Protocol<Http.Request, Http.Response>"  - nested qualified templates
//                      "Parser<Input.Stream<Byte>, Output.Tree<AST.Node>>"
//   Multiple Templated Segments:
//                      "Container<Int>.Iterator<Forward>"        - multiple segments with templates
//                      "Db.Table<User>.Column<Name>.Validator"   - templates in middle segments
//                      "Parser<Token>.Result<AST>.Error<String>" - chained templated segments

// Forward declarations for mutually recursive rules
struct Path_Tag : x3::annotate_on_success, Error_Handler {};
struct Path_Segment_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Path_Tag, ast::Path> const k_path_rule = "type path";
x3::rule<Path_Segment_Tag, ast::Path_Segment> const k_path_segment_rule = "type path segment";

// Parse segment name: alphanumeric identifier starting with letter (but not a keyword)
// Examples: "Vec", "Array", "foo_bar", "MyType123", "IO", "Iterator", "Column"
x3::rule<struct segment_name_tag, std::string> const k_segment_name = "segment name";
auto const k_segment_name_def = raw[lexeme[alpha >> *(alnum | '_')]] - k_reserved;
BOOST_SPIRIT_DEFINE(k_segment_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_segment_name), Iterator_Type, Context_Type)

// Parse template parameters: angle-bracketed comma-separated paths
// Each parameter can itself be a full qualified path with templates
// Examples:
//   "<Int>"                                      - simple type
//   "<Key, Value>"                               - multiple simple types
//   "<Array<Int>>"                               - nested template
//   "<Data.Model.User, Config.Settings>"         - qualified paths as params
//   "<Std.String, IO.Error>"                     - multiple qualified params
//   "<Parser<Token.Type>, Result<AST.Node, E>>"  - complex nested with qualified paths
x3::rule<struct template_params_tag, std::vector<ast::Path>> const k_template_params = "template parameters";
auto const k_template_params_def = (lit('<') > (k_path_rule % ',')) > lit('>');
BOOST_SPIRIT_DEFINE(k_template_params)
BOOST_SPIRIT_INSTANTIATE(decltype(k_template_params), Iterator_Type, Context_Type)

// Parse path segment: name with optional template parameters
// A segment can have template parameters that are full paths (including qualified)
// Examples:
//   "Array"                                  - simple name
//   "Array<Int>"                             - simple template
//   "Map<String, Int>"                       - multi-param template
//   "Table<Data.Model.User>"                 - template with qualified path
//   "Result<IO.Error, Data.Value>"           - template with multiple qualified paths
//   "Container<Int>"                         - templated segment (can be followed by more segments)
auto const k_path_segment_rule_def = (k_segment_name >> -k_template_params)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_path_segment(
      std::move(boost::fusion::at_c<0>(attr)), boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Path>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_path_segment_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_path_segment_rule), Iterator_Type, Context_Type)

// Parse full path: dot-separated path segments
// Any segment in the path can have template parameters, not just the last one!
// This allows paths like "Container<T>.Iterator<Forward>" where intermediate segments are templated.
// Examples:
//   "Int"                                              - simple path
//   "Std.String"                                       - qualified path
//   "Std.Collections.Array<T>"                         - qualified with template on last segment
//   "Std.Collections.Map<Key.Type, Value>"             - qualified segment with qualified template param
//   "Container<Int>.Iterator<Forward>"                 - multiple segments with templates
//   "Db.Table<User>.Column<Name>.Validator"            - templates in middle segments
//   "Parser<Token>.Result<AST>.Error<String>"          - multiple templated segments in chain
//   "Network.Protocol<Http.Request, Http.Response>"    - deeply nested qualified templates
//   "IO.Result<Data.Error, Parser.AST>"                - multiple qualified params
auto const k_path_rule_def =
    (k_path_segment_rule % '.')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::Path{{}, x3::_attr(a_ctx)}; })];
BOOST_SPIRIT_DEFINE(k_path_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_path_rule), Iterator_Type, Context_Type)

// === String Literal Rules ===
// String literals with escape sequences
// Examples:
//   Simple:   "hello"
//   Escaped:  "hello\nworld", "say \"hi\""
//   Hex:      "null byte: \x00"
auto const k_escaped_char = lexeme[lit('\\') > (char_("\"\\ntr") | ('x' > x3::repeat(2)[x3::xdigit]))];

// Parse string literal: quoted text with escape sequences
struct String_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<String_Tag, ast::String> const k_string_rule = "string literal";
auto const k_string_rule_def =
    raw[lexeme[(lit('"') > *(k_escaped_char | (char_ - '"' - '\\'))) > lit('"')]][([](auto& a_ctx) {
      auto const& raw_string = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_string(std::string(raw_string.begin(), raw_string.end()));
    })];
BOOST_SPIRIT_DEFINE(k_string_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_string_rule), Iterator_Type, Context_Type)

// === Integer Literal Rules ===
// Integer literals with optional digit separators
// Examples:
//   Simple:      0, 42, 123
//   Separated:   1_000_000, 123_456
//   Invalid:     01 (leading zero), 123_ (trailing underscore)
// Parse integer: '0' or non-zero digit followed by digits/underscores
struct Integer_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Integer_Tag, ast::Integer> const k_integer_rule = "integer literal";
auto const k_integer_rule_def =
    raw[lexeme[(lit('0') >> !(digit | '_')) | (char_('1', '9') >> *(char_('0', '9') | '_'))]][([](auto& a_ctx) {
      auto const& attr = x3::_attr(a_ctx);
      std::string str{attr.begin(), attr.end()};
      if (str.back() == '_') {
        x3::_pass(a_ctx) = false;  // Reject trailing underscore
      } else {
        std::erase(str, '_');  // Remove digit separators
        x3::_val(a_ctx) = ast::make_integer(str);
      }
    })];
BOOST_SPIRIT_DEFINE(k_integer_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_integer_rule), Iterator_Type, Context_Type)

// === Function Declaration Rules ===
// Function declarations specify function signature (name, parameters, return type)
// Examples:
//   No params:   fn main(): I32
//   With params: fn add(a: Int, b: Int): Int
//   Generic:     fn map(f: Fn<T, U>, arr: Array<T>): Array<U>

// Forward declarations for function rules
struct Function_Parameter_Tag : x3::annotate_on_success, Error_Handler {};
struct Function_Declaration_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Function_Parameter_Tag, ast::Function_Parameter> const k_function_parameter_rule = "function parameter";
x3::rule<Function_Declaration_Tag, ast::Function_Declaration> const k_function_declaration_rule =
    "function declaration";

// Parse parameter name: snake_case identifier
x3::rule<struct param_name_tag, std::string> const k_param_name = "parameter name";
auto const k_param_name_def = k_snake_case;
BOOST_SPIRIT_DEFINE(k_param_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_param_name), Iterator_Type, Context_Type)

// Parse parameter type: path expression
x3::rule<struct param_type_tag, ast::Path> const k_param_type = "parameter type";
auto const k_param_type_def = k_path_rule;
BOOST_SPIRIT_DEFINE(k_param_type)
BOOST_SPIRIT_INSTANTIATE(decltype(k_param_type), Iterator_Type, Context_Type)

// Parse function parameter: "name: Type"
// Example: "x: Int", "callback: Fn<String, Bool>"
auto const k_function_parameter_rule_def = ((k_param_name > ':') > k_param_type)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) =
      ast::make_function_parameter(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
})];
BOOST_SPIRIT_DEFINE(k_function_parameter_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_parameter_rule), Iterator_Type, Context_Type)

// Parse function name: snake_case identifier
x3::rule<struct func_name_tag, std::string> const k_func_name = "function name";
auto const k_func_name_def = k_snake_case;
BOOST_SPIRIT_DEFINE(k_func_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_func_name), Iterator_Type, Context_Type)

// Parse function parameters list: comma-separated, optional
x3::rule<struct func_params_tag, std::vector<ast::Function_Parameter>> const k_func_params = "function parameters";
auto const k_func_params_def = k_function_parameter_rule % ',';
BOOST_SPIRIT_DEFINE(k_func_params)
BOOST_SPIRIT_INSTANTIATE(decltype(k_func_params), Iterator_Type, Context_Type)

// Parse function return type: path expression
x3::rule<struct func_return_type_tag, ast::Path> const k_func_return_type = "function return type";
auto const k_func_return_type_def = k_path_rule;
BOOST_SPIRIT_DEFINE(k_func_return_type)
BOOST_SPIRIT_INSTANTIATE(decltype(k_func_return_type), Iterator_Type, Context_Type)

// Parse function declaration: "fn name(params): ReturnType"
// Examples:
//   fn main(): I32
//   fn add(a: Int, b: Int): Int
//   fn process(data: Array<String>): Result<(), Error>
auto const k_function_declaration_rule_def =
    ((((((k_kw_fn > k_func_name) > '(') > -k_func_params) > ')') > ':') > k_func_return_type)[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_function_declaration(
          std::move(boost::fusion::at_c<0>(attr)),
          boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Function_Parameter>{}),
          std::move(boost::fusion::at_c<2>(attr))
      );
    })];
BOOST_SPIRIT_DEFINE(k_function_declaration_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_declaration_rule), Iterator_Type, Context_Type)

// === Expression Rules ===
// Expressions are values that can be computed or evaluated
// Examples:
//   Literals:      42, "hello", true
//   Paths:         x, Std.PI, MyModule.constant
//   Function call: print("hi"), add(1, 2), map(transform, items)

// Forward declarations for expression rules
struct Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Function_Call_Expr_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Expr_Tag, ast::Expr> const k_expr_rule = "expression";
x3::rule<Function_Call_Expr_Tag, ast::Function_Call_Expr> const k_function_call_expr_rule = "function call expression";

// Parse function call name: path expression
x3::rule<struct call_name_tag, ast::Path> const k_call_name = "function name";
auto const k_call_name_def = k_path_rule;
BOOST_SPIRIT_DEFINE(k_call_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_call_name), Iterator_Type, Context_Type)

// Parse function call arguments: comma-separated expressions
x3::rule<struct call_args_tag, std::vector<ast::Expr>> const k_call_args = "function arguments";
auto const k_call_args_def = k_expr_rule % ',';
BOOST_SPIRIT_DEFINE(k_call_args)
BOOST_SPIRIT_INSTANTIATE(decltype(k_call_args), Iterator_Type, Context_Type)

// Parse function call expression: "name(args)"
// Examples:
//   No args:     print()
//   With args:   add(1, 2)
//   Nested:      map(transform, filter(is_valid, data))
auto const k_function_call_expr_rule_def = (((k_call_name >> '(') > -k_call_args) > ')')[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_function_call_expr(
      std::move(boost::fusion::at_c<0>(attr)), boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Expr>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_function_call_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_call_expr_rule), Iterator_Type, Context_Type)

// Parse expression: variant of different expression types
auto const k_expr_rule_def = k_function_call_expr_rule | k_string_rule | k_path_rule | k_integer_rule;
BOOST_SPIRIT_DEFINE(k_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_expr_rule), Iterator_Type, Context_Type)

// === Statement Rules ===
// Statements are executable units that perform actions
// Examples:
//   Return:        return 42;
//   Function call: print("hello");
//   Block:         { stmt1; stmt2; }

// Parse return statement: "return expr;"
// Examples: "return 0;", "return calculate(x);"
struct Return_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Return_Statement_Tag, ast::Return_Statement> const k_return_statement_rule = "return statement";
auto const k_return_statement_rule_def =
    ((k_kw_return > k_expr_rule) >
     ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_return_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_return_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_return_statement_rule), Iterator_Type, Context_Type)

// Parse function call statement: "call(args);"
// Examples: "print(msg);", "process_data(items);"
struct Function_Call_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Function_Call_Statement_Tag, ast::Function_Call_Statement> const k_function_call_statement_rule =
    "function call statement";
auto const k_function_call_statement_rule_def =
    (k_function_call_expr_rule >
     ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_function_call_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_function_call_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_call_statement_rule), Iterator_Type, Context_Type)

// === Block and Function Definition Rules ===
// Blocks contain sequences of statements
// Function definitions combine declaration with body
// Examples:
//   Block:     { stmt1; stmt2; return x; }
//   Function:  fn add(a: Int, b: Int): Int { return a + b; }

// Forward declarations for statement rules
struct Statement_Tag : x3::annotate_on_success, Error_Handler {};
struct Block_Tag : x3::annotate_on_success, Error_Handler {};
struct Function_Definition_Tag : x3::annotate_on_success, Error_Handler {};
struct Module_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Statement_Tag, ast::Statement> const k_statement_rule = "statement";
x3::rule<Block_Tag, ast::Block> const k_block_rule = "code block";
x3::rule<Function_Definition_Tag, ast::Function_Definition> const k_function_definition_rule = "function definition";
x3::rule<Module_Tag, ast::Module> const k_module_rule = "module";

// Parse list of statements: zero or more statements
x3::rule<struct statements_tag, std::vector<ast::Statement>> const k_statements = "statements";
auto const k_statements_def = *k_statement_rule;
BOOST_SPIRIT_DEFINE(k_statements)
BOOST_SPIRIT_INSTANTIATE(decltype(k_statements), Iterator_Type, Context_Type)

// Parse block: "{ statements }"
// Example: { print("hi"); return 0; }
auto const k_block_rule_def =
    (('{' > k_statements) > '}')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_block(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_block_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_block_rule), Iterator_Type, Context_Type)

// Parse function definition: declaration followed by body block
// Example: fn main(): I32 { return 0; }
auto const k_function_definition_rule_def = (k_function_declaration_rule > k_block_rule)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) =
      ast::make_function_definition(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
})];
BOOST_SPIRIT_DEFINE(k_function_definition_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_definition_rule), Iterator_Type, Context_Type)

// Parse statement: variant of different statement types
// Order matters: try function definition first (longest match), then others
auto const k_statement_rule_def =
    k_function_definition_rule | k_function_call_statement_rule | k_block_rule | k_return_statement_rule;
BOOST_SPIRIT_DEFINE(k_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_statement_rule), Iterator_Type, Context_Type)

// Parse module: zero or more top-level statements
// A module represents a complete compilation unit (file)
// Top-level statements are currently only function definitions, but will include:
// - Import/export statements
// - Type definitions (struct, enum, trait, etc.)
// - Module-level constants
// Example:
//   fn helper(): Void { }
//   fn main(): I32 { return 0; }
auto const k_module_rule_def =
    (*k_statement_rule)[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_module(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_module_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_module_rule), Iterator_Type, Context_Type)
// ============================================================================
// Position Tracker - converts byte offsets to line:column positions
// ============================================================================
class Position_Tracker {
 public:
  explicit Position_Tracker(std::string_view a_source)
      : m_source_begin(a_source.begin()), m_source_end(a_source.end()) {
    build_line_map();
  }

  // Convert byte offset to line:column
  [[nodiscard]] Source_Position offset_to_position(std::size_t a_offset) const {
    auto const it = std::ranges::upper_bound(m_line_starts, a_offset);
    if (it == m_line_starts.begin()) {
      return {.line = 1, .column = 1};
    }

    auto const prev = it - 1;
    auto const line = static_cast<std::size_t>(std::distance(m_line_starts.begin(), prev) + 1);
    auto const column = a_offset - *prev + 1;
    return {.line = line, .column = column};
  }

  // Get range from iterator positions
  [[nodiscard]] Source_Range iterator_to_range(Iterator_Type a_begin, Iterator_Type a_end) const {
    auto const start_offset = static_cast<std::size_t>(a_begin - m_source_begin);
    auto const end_offset = static_cast<std::size_t>(a_end - m_source_begin);
    return {.start = offset_to_position(start_offset), .end = offset_to_position(end_offset)};
  }

 private:
  Iterator_Type m_source_begin;
  Iterator_Type m_source_end;
  std::vector<std::size_t> m_line_starts;

  void build_line_map() {
    m_line_starts.push_back(0);
    std::size_t offset = 0;
    for (auto it = m_source_begin; it != m_source_end; ++it, ++offset) {
      // Handle all line ending conventions:
      // - Unix/Linux: \n (LF)
      // - Windows: \r\n (CRLF)
      // - Old Mac: \r (CR)
      if (*it == '\n') {
        m_line_starts.push_back(offset + 1);
      } else if (*it == '\r') {
        // Check if it's \r\n (Windows) or standalone \r (old Mac)
        auto next = it + 1;
        if (next != m_source_end && *next == '\n') {
          // Windows CRLF: skip the \r, let \n handling record the line start
          continue;
        }
        // Old Mac CR: treat as line ending
        m_line_starts.push_back(offset + 1);
      }
    }
  }
};

}  // namespace life_lang::parser

namespace life_lang::internal {
namespace {
// Generic parser wrapper with diagnostic generation
// Returns parsed AST on success, or Diagnostic_Engine with clang-style error on failure.
// Error includes source location, line:column, and context from Spirit X3.
template <typename Rule, typename Ast>
parser::Parse_Result<Ast> parse_with_rule(
    Rule const& a_rule, parser::Iterator_Type& a_begin, parser::Iterator_Type a_end, std::string_view a_source
) {
  // Build diagnostic engine for error accumulation
  Diagnostic_Engine diagnostics("<input>", std::string(a_source));

  // Build position tracker for line:column conversion
  parser::Position_Tracker const tracker(a_source);

  // Capture Spirit X3's error messages for additional context
  std::ostringstream error_stream;
  parser::Error_Handler_Type error_handler(a_begin, a_end, error_stream);

  // Create parser with error_handler context
  auto const parser = with<parser::x3::error_handler_tag>(std::ref(error_handler))[a_rule];
  Ast ast;
  bool const success = phrase_parse(a_begin, a_end, parser, parser::Space_Type{}, ast);

  if (success) {
    return ast;
  }

  // Build diagnostic with clang-style formatting
  auto const range = tracker.iterator_to_range(a_begin, a_begin == a_end ? a_begin : a_begin + 1);

  // Build detailed error message using the rule's user-friendly description
  std::string error_msg = fmt::format("Failed to parse {}", a_rule.name);

  // Extract Spirit X3's error message text (without its formatting) if available
  std::string const spirit_error = error_stream.str();
  if (!spirit_error.empty()) {
    // Find the error message line (starts with "Error!")
    std::size_t const error_pos = spirit_error.find("Error!");
    if (error_pos != std::string::npos) {
      // Extract from "Error!" to the next newline
      std::size_t const newline_pos = spirit_error.find('\n', error_pos);
      if (newline_pos != std::string::npos) {
        std::string const error_text = spirit_error.substr(error_pos, newline_pos - error_pos);
        error_msg += fmt::format(": {}", error_text);
      }
    }
  }

  diagnostics.add_error(range, std::move(error_msg));
  return tl::unexpected(std::move(diagnostics));
}
}  // namespace

// Parse function implementations - generate diagnostics for errors
#define PARSE_FN_IMPL(ast_type, fn_name, rule_name)                                                                  \
  parser::Parse_Result<ast::ast_type> parse_##fn_name(parser::Iterator_Type& a_begin, parser::Iterator_Type a_end) { \
    std::string_view const source(a_begin, a_end);                                                                   \
    return parse_with_rule<decltype(parser::rule_name), ast::ast_type>(parser::rule_name, a_begin, a_end, source);   \
  }

PARSE_FN_IMPL(Path_Segment, path_segment, k_path_segment_rule)
PARSE_FN_IMPL(Path, path, k_path_rule)
PARSE_FN_IMPL(String, string, k_string_rule)
PARSE_FN_IMPL(Integer, integer, k_integer_rule)
PARSE_FN_IMPL(Function_Parameter, function_parameter, k_function_parameter_rule)
PARSE_FN_IMPL(Function_Declaration, function_declaration, k_function_declaration_rule)
PARSE_FN_IMPL(Expr, expr, k_expr_rule)
PARSE_FN_IMPL(Function_Call_Expr, function_call_expr, k_function_call_expr_rule)
PARSE_FN_IMPL(Function_Call_Statement, function_call_statement, k_function_call_statement_rule)
PARSE_FN_IMPL(Return_Statement, return_statement, k_return_statement_rule)
PARSE_FN_IMPL(Statement, statement, k_statement_rule)
PARSE_FN_IMPL(Block, block, k_block_rule)
PARSE_FN_IMPL(Function_Definition, function_definition, k_function_definition_rule)
PARSE_FN_IMPL(Module, module, k_module_rule)
#undef PARSE_FN_IMPL

}  // namespace life_lang::internal

namespace life_lang::parser {

tl::expected<ast::Module, Diagnostic_Engine> parse_module(std::string_view a_source, std::string a_filename) {
  // Convert string_view to string so we can use string::const_iterator
  std::string const source_str(a_source);
  Diagnostic_Engine diagnostics(std::move(a_filename), source_str);

  auto begin = source_str.begin();
  auto const end = source_str.end();

  // Parse the module
  auto result = internal::parse_module(begin, end);

  // Check if parse succeeded
  if (!result) {
    return tl::unexpected(std::move(result.error()));
  }

  // Parse succeeded - check if all input was consumed
  if (begin != end) {
    Position_Tracker const tracker(source_str);
    auto const range = tracker.iterator_to_range(begin, end);
    diagnostics.add_error(range, "Unexpected input after module");
    return tl::unexpected(std::move(diagnostics));
  }

  return *result;
}
}  // namespace life_lang::parser