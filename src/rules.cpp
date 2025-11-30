#include "rules.hpp"

#include <fmt/core.h>

#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

namespace life_lang::parser {
namespace x3 = boost::spirit::x3;

using Space_Type = x3::ascii::space_type;
using Error_Handler_Type = x3::error_handler<Iterator_Type>;
using Phrase_Context_Type = x3::phrase_parse_context<Space_Type>::type;
using Context_Type =
    x3::context<x3::error_handler_tag, std::reference_wrapper<Error_Handler_Type>, Phrase_Context_Type>;

struct Error_Handler {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(
      Iterator& /*a_first*/, Iterator const& /*a_last*/, Exception const& a_x, Context const& a_context
  ) {
    auto& error_handler = x3::get<x3::error_handler_tag>(a_context);
    std::string const message = fmt::format("Error! Expecting: {} here:", a_x.which());
    error_handler(a_x.where(), message);
    return x3::error_handler_result::fail;
  }
};

// Import commonly used parsers
using x3::lexeme;
using x3::raw;
using x3::ascii::alnum;
using x3::ascii::alpha;
using x3::ascii::char_;
using x3::ascii::digit;
using x3::ascii::lit;
using x3::ascii::lower;

// Keyword management: centralized symbol table for reserved words
// Note: String literals are duplicated in k_keywords and individual parsers because:
// - k_keywords is used for identifier validation (reject reserved words)
// - Individual parsers (k_kw_*) are used in grammar rules for matching
x3::symbols<int> const k_keywords = []() {
  x3::symbols<int> syms;  // NOLINT(misc-const-correctness) - must be mutable for .add()
  syms.add("fn")("let")("return");
  return syms;
}();
auto const k_kw_fn = lexeme[lit("fn") >> !(alnum | '_')];
[[maybe_unused]] auto const k_kw_let = lexeme[lit("let") >> !(alnum | '_')];
auto const k_kw_return = lexeme[lit("return") >> !(alnum | '_')];

// Reserved word rule: matches any registered keyword (for identifier validation)
auto const k_reserved = lexeme[k_keywords >> !(alnum | '_')];

// Identifier patterns
auto const k_snake_case = raw[lexeme[lower >> *(lower | digit | '_') >> !(alnum | '_')]];

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
struct Path_Tag : Error_Handler, x3::position_tagged {};
struct Path_Segment_Tag : Error_Handler, x3::position_tagged {};
x3::rule<Path_Tag, ast::Path> const k_path_rule = "path";
x3::rule<Path_Segment_Tag, ast::Path_Segment> const k_path_segment_rule = "path segment";

// Parse segment name: alphanumeric identifier starting with letter
// Examples: "Vec", "Array", "foo_bar", "MyType123", "IO", "Iterator", "Column"
x3::rule<struct segment_name_tag, std::string> const k_segment_name = "segment name";
auto const k_segment_name_def = raw[lexeme[alpha >> *(alnum | '_')]];
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
// Note: Uses >> (sequence) which creates fusion::deque, requiring at_c<N> to extract values
auto const k_path_segment_rule_def = (k_segment_name >> -k_template_params)[([](auto& a_ctx) {
  auto const& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::Path_Segment{
      .value = boost::fusion::at_c<0>(attr),
      .template_parameters = boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Path>{})
  };
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
    (k_path_segment_rule % '.')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::Path{.segments = x3::_attr(a_ctx)}; })];
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
// Note: raw[] captures the entire matched text including quotes
struct String_Tag : Error_Handler, x3::position_tagged {};
x3::rule<String_Tag, ast::String> const k_string_rule = "string";
auto const k_string_rule_def =
    raw[lexeme[(lit('"') > *(k_escaped_char | (char_ - '"' - '\\'))) > lit('"')]][([](auto& a_ctx) {
      auto const& raw_string = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::String{.value = std::string(raw_string.begin(), raw_string.end())};
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
// Semantic action validates and removes underscores
struct Integer_Tag : Error_Handler, x3::position_tagged {};
x3::rule<Integer_Tag, ast::Integer> const k_integer_rule = "integer";
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
struct Function_Parameter_Tag : Error_Handler, x3::position_tagged {};
struct Function_Declaration_Tag : Error_Handler, x3::position_tagged {};
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
  auto const& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::Function_Parameter{.name = boost::fusion::at_c<0>(attr), .type = boost::fusion::at_c<1>(attr)};
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
      auto const& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::Function_Declaration{
          .name = boost::fusion::at_c<0>(attr),
          .parameters = boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Function_Parameter>{}),
          .return_type = boost::fusion::at_c<2>(attr)
      };
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
struct Expr_Tag : Error_Handler, x3::position_tagged {};
struct Function_Call_Expr_Tag : Error_Handler, x3::position_tagged {};
x3::rule<Expr_Tag, ast::Expr> const k_expr_rule = "expr";
x3::rule<Function_Call_Expr_Tag, ast::Function_Call_Expr> const k_function_call_expr_rule = "function call";

// Parse function call name: path expression
x3::rule<struct call_name_tag, ast::Path> const k_call_name = "call name";
auto const k_call_name_def = k_path_rule;
BOOST_SPIRIT_DEFINE(k_call_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_call_name), Iterator_Type, Context_Type)

// Parse function call arguments: comma-separated expressions
x3::rule<struct call_args_tag, std::vector<ast::Expr>> const k_call_args = "call arguments";
auto const k_call_args_def = k_expr_rule % ',';
BOOST_SPIRIT_DEFINE(k_call_args)
BOOST_SPIRIT_INSTANTIATE(decltype(k_call_args), Iterator_Type, Context_Type)

// Parse function call expression: "name(args)"
// Examples:
//   No args:     print()
//   With args:   add(1, 2)
//   Nested:      map(transform, filter(is_valid, data))
auto const k_function_call_expr_rule_def = ((k_call_name >> '(') >> -k_call_args >> ')')[([](auto& a_ctx) {
  auto const& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::Function_Call_Expr{
      .name = boost::fusion::at_c<0>(attr),
      .parameters = boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Expr>{})
  };
})];
BOOST_SPIRIT_DEFINE(k_function_call_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_call_expr_rule), Iterator_Type, Context_Type)

// Parse expression: variant of different expression types
// The '|' operator tries each alternative in order until one matches
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
struct Return_Statement_Tag : Error_Handler, x3::position_tagged {};
x3::rule<Return_Statement_Tag, ast::Return_Statement> const k_return_statement_rule = "return statement";
auto const k_return_statement_rule_def =
    ((k_kw_return > k_expr_rule) >
     ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::Return_Statement{.expr = x3::_attr(a_ctx)}; })];
BOOST_SPIRIT_DEFINE(k_return_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_return_statement_rule), Iterator_Type, Context_Type)

// Parse function call statement: "call(args);"
// Examples: "print(msg);", "process_data(items);"
struct Function_Call_Statement_Tag : Error_Handler, x3::position_tagged {};
x3::rule<Function_Call_Statement_Tag, ast::Function_Call_Statement> const k_function_call_statement_rule =
    "function call statement";
auto const k_function_call_statement_rule_def =
    (k_function_call_expr_rule >
     ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::Function_Call_Statement{.expr = x3::_attr(a_ctx)}; })];
BOOST_SPIRIT_DEFINE(k_function_call_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_call_statement_rule), Iterator_Type, Context_Type)

// === Block and Function Definition Rules ===
// Blocks contain sequences of statements
// Function definitions combine declaration with body
// Examples:
//   Block:     { stmt1; stmt2; return x; }
//   Function:  fn add(a: Int, b: Int): Int { return a + b; }

// Forward declarations for statement rules
struct Statement_Tag : Error_Handler, x3::position_tagged {};
struct Block_Tag : Error_Handler, x3::position_tagged {};
struct Function_Definition_Tag : Error_Handler, x3::position_tagged {};
struct Module_Tag : Error_Handler, x3::position_tagged {};
x3::rule<Statement_Tag, ast::Statement> const k_statement_rule = "statement";
x3::rule<Block_Tag, ast::Block> const k_block_rule = "block";
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
    (('{' > k_statements) > '}')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::Block{.statements = x3::_attr(a_ctx)}; })];
BOOST_SPIRIT_DEFINE(k_block_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_block_rule), Iterator_Type, Context_Type)

// Parse function definition: declaration followed by body block
// Example: fn main(): I32 { return 0; }
// Note: Uses >> which creates fusion::deque, requiring at_c<N>
auto const k_function_definition_rule_def = (k_function_declaration_rule >> k_block_rule)[([](auto& a_ctx) {
  auto const& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) =
      ast::Function_Definition{.declaration = boost::fusion::at_c<0>(attr), .body = boost::fusion::at_c<1>(attr)};
})];
BOOST_SPIRIT_DEFINE(k_function_definition_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_definition_rule), Iterator_Type, Context_Type)

// Parse statement: variant of different statement types
// Spirit X3 automatically handles variant alternatives - no semantic action needed
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
    (*k_statement_rule)[([](auto& a_ctx) { x3::_val(a_ctx) = ast::Module{.statements = x3::_attr(a_ctx)}; })];
BOOST_SPIRIT_DEFINE(k_module_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_module_rule), Iterator_Type, Context_Type)
}  // namespace life_lang::parser

namespace life_lang::internal {
namespace {
// Generic parser wrapper with error handling
// Returns parsed AST on success, or detailed error message on failure.
// Also writes error details to a_out stream for legacy compatibility.
template <typename Rule, typename Ast>
parser::Parse_Result<Ast> parse_with_rule(
    Rule const& a_rule, parser::Iterator_Type& a_begin, parser::Iterator_Type a_end, std::ostream& a_out
) {
  // Capture error messages in local stream, then copy to a_out
  std::ostringstream error_stream;
  parser::Error_Handler_Type error_handler(a_begin, a_end, error_stream);

  auto const parser = with<parser::x3::error_handler_tag>(std::ref(error_handler))[a_rule];
  Ast ast;
  bool const success = phrase_parse(a_begin, a_end, parser, parser::Space_Type{}, ast);

  if (success) {
    return ast;
  }

  // Copy error message to both a_out and the returned error
  std::string const error_msg = error_stream.str();
  a_out << error_msg;
  return tl::unexpected(error_msg.empty() ? "Parse failed" : error_msg);
}
}  // namespace

// Parse function implementations
#define PARSE_FN_IMPL(ast_type, fn_name, rule_name)                                                               \
  parser::Parse_Result<ast::ast_type> parse_##fn_name(                                                            \
      parser::Iterator_Type& a_begin, parser::Iterator_Type a_end, std::ostream& a_out                            \
  ) {                                                                                                             \
    return parse_with_rule<decltype(parser::rule_name), ast::ast_type>(parser::rule_name, a_begin, a_end, a_out); \
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
// Public API: parse a complete module (compilation unit)
// Requires consuming ALL input - partial parses are considered failures.
Parse_Result<ast::Module> parse(Iterator_Type& a_begin, Iterator_Type a_end, std::ostream& a_out) {
  auto const start = a_begin;
  auto result = internal::parse_module(a_begin, a_end, a_out);

  if (!result) {
    return result;  // Parse failed
  }

  // Success - but did we consume all input?
  if (a_begin != a_end) {
    a_out << "Error: unexpected input after module: ";
    a_out.write(&*a_begin, std::distance(a_begin, a_end));
    a_out << "\n";
    a_begin = start;  // Reset iterator for error reporting
    return tl::unexpected("Unexpected input after module");
  }

  return result;
}
}  // namespace life_lang::parser