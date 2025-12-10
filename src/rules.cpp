#include "rules.hpp"

#include <fmt/core.h>

#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <variant>

namespace life_lang::parser {
namespace x3 = boost::spirit::x3;

// Marker string used in Spirit X3 error messages to identify expectation failures
// This prefix is added by Error_Handler::on_error() and searched for during error extraction
inline constexpr std::string_view k_spirit_error_marker = "[PARSE_ERROR]";

using Space_Type = x3::ascii::space_type;
using Error_Handler_Type = x3::error_handler<Iterator_Type>;
using Phrase_Context_Type = x3::phrase_parse_context<Space_Type>::type;
using Context_Type =
    x3::context<x3::error_handler_tag, std::reference_wrapper<Error_Handler_Type>, Phrase_Context_Type>;

// Error handler for Spirit X3 expectation failures (operator `>`)
// Logs errors to x3::error_handler stream for later extraction
struct Error_Handler {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result
  on_error(Iterator& /*a_first*/, Iterator const& /*a_last*/, Exception const& a_ex, Context const& a_context) {
    auto& error_handler = x3::get<x3::error_handler_tag>(a_context);
    std::string const message = fmt::format("{} Expecting: {} here:", k_spirit_error_marker, a_ex.which());
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

// Comment parsers (used in skipper)
// Line comment: // ... until end of line
// Block comment: /* ... */ (not nested)
// Note: Using x3::char_ (not x3::ascii::char_) to support UTF-8 in comments
namespace {
auto const k_line_comment = lit("//") >> *(x3::char_ - x3::eol);
auto const k_block_comment = lit("/*") >> *(x3::char_ - "*/") >> "*/";
auto const k_comment = k_line_comment | k_block_comment;
}  // namespace

// Skipper combines whitespace and comments
auto const k_skipper = x3::ascii::space | k_comment;

struct Keyword_Symbols : x3::symbols<> {
  Keyword_Symbols() {
    add("fn")("let")("return")("struct")("enum")("impl")("self")("mut")("if")("else")("while")("for")("in")("match")(
        "break")("continue"
    );
  }
} const k_keywords;

// Individual keyword parsers for specific grammar rules (improves error messages)
auto const k_kw_fn = lexeme[lit("fn") >> !(alnum | '_')];
auto const k_kw_let = lexeme[lit("let") >> !(alnum | '_')];
auto const k_kw_return = lexeme[lit("return") >> !(alnum | '_')];
auto const k_kw_struct = lexeme[lit("struct") >> !(alnum | '_')];
auto const k_kw_enum = lexeme[lit("enum") >> !(alnum | '_')];
auto const k_kw_impl = lexeme[lit("impl") >> !(alnum | '_')];
auto const k_kw_self = lexeme[lit("self") >> !(alnum | '_')];
auto const k_kw_mut = lexeme[lit("mut") >> !(alnum | '_')];
auto const k_kw_if = lexeme[lit("if") >> !(alnum | '_')];
auto const k_kw_else = lexeme[lit("else") >> !(alnum | '_')];
auto const k_kw_while = lexeme[lit("while") >> !(alnum | '_')];
auto const k_kw_for = lexeme[lit("for") >> !(alnum | '_')];
auto const k_kw_in = lexeme[lit("in") >> !(alnum | '_')];
auto const k_kw_match = lexeme[lit("match") >> !(alnum | '_')];
auto const k_kw_break = lexeme[lit("break") >> !(alnum | '_')];
auto const k_kw_continue = lexeme[lit("continue") >> !(alnum | '_')];

// Reserved word rule: matches any registered keyword (for identifier validation)
auto const k_reserved = lexeme[k_keywords >> !(alnum | '_')];

// Parse segment name: alphanumeric identifier starting with letter (not a keyword, except 'self')
// Accepts any valid identifier pattern (snake_case, Camel_Snake_Case, or mixed)
// Naming convention enforcement is deferred to semantic analysis phase:
//   - Variables/functions should be snake_case
//   - Types/modules should be Camel_Snake_Case
// Note: 'self' is a reserved keyword but allowed as identifier for UFCS (parameter/variable name)
// Examples: "Vec", "Array", "foo", "MyType123", "IO", "my_var", "HTTP_Server", "self"
x3::rule<struct segment_name_tag, std::string> const k_segment_name = "segment name";
auto const k_segment_name_def = raw[lexeme[alpha >> *(alnum | '_')]] - (k_reserved - k_kw_self);
BOOST_SPIRIT_DEFINE(k_segment_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_segment_name), Iterator_Type, Context_Type)

// === Type Name Rules ===
// Type names represent type annotations with optional template parameters
// Examples:
// **IMPORTANT** Some examples are not valid for later semantic analysis,
// but are included here to demonstrate parsing capabilities.
//
//   Simple:            "Int", "String", "MyClass"
//   Qualified:         "Std.String", "Std.Collections.Array"
//   Simple Template:   "Array<Int>", "Map<String, Int>"
//   Nested Templates:  "Vec<Vec<Int>>", "Option<Result<T, Error>>"
//   Qualified Names in Templates:
//                      "Map<Std.String, Int>"                    - qualified type as template param
//                      "Array<Data.Model.User>"                  - deeply nested qualified name as param
//                      "Result<IO.Error, Data.Value>"            - multiple qualified params
//   Complex Mixed:     "Std.Collections.Map<Key.Type, Value.Type>"
//                      "Network.Protocol<Http.Request, Http.Response>"  - nested qualified templates
//                      "Parser<Input.Stream<Byte>, Output.Tree<AST.Node>>"
//   Multiple Templated Segments:
//                      "Container<Int>.Iterator<Forward>"        - multiple segments with templates
//                      "Db.Table<User>.Column<Name>.Validator"   - templates in middle segments
//                      "Parser<Token>.Result<AST>.Error<String>" - chained templated segments

// Forward declarations for mutually recursive rules
struct Type_Name_Tag : x3::annotate_on_success, Error_Handler {};
struct Type_Name_Segment_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Type_Name_Tag, ast::Type_Name> const k_type_name_rule = "type name";
x3::rule<Type_Name_Segment_Tag, ast::Type_Name_Segment> const k_type_name_segment_rule = "type name segment";

// Parse type parameters: angle-bracketed comma-separated qualified names
// Each parameter can itself be a full qualified name with type parameters
// Examples:
//   "<Int>"                                      - simple type
//   "<Key, Value>"                               - multiple simple types
//   "<Array<Int>>"                               - nested type parameter
//   "<Data.Model.User, Config.Settings>"         - qualified names as params
//   "<Std.String, IO.Error>"                     - multiple qualified params
//   "<Parser<Token.Type>, Result<AST.Node, E>>"  - complex nested with qualified names
x3::rule<struct type_params_tag, std::vector<ast::Type_Name>> const k_type_params = "type parameters";
auto const k_type_params_def = lit('<') >> (k_type_name_rule % ',') >> lit('>');
BOOST_SPIRIT_DEFINE(k_type_params)
BOOST_SPIRIT_INSTANTIATE(decltype(k_type_params), Iterator_Type, Context_Type)

// Parse qualified name segment: name with optional type parameters
// A segment can have type parameters that are full qualified names (including multi-segment)
// Examples:
//   "Array"                                  - simple name
//   "Array<Int>"                             - simple type parameter
//   "Map<String, Int>"                       - multi-param type parameter
//   "Table<Data.Model.User>"                 - type parameter with qualified name
//   "Result<IO.Error, Data.Value>"           - type parameter with multiple qualified names
//   "Container<Int>"                         - segment with type parameters (can be followed by more segments)
auto const k_type_name_segment_rule_def = (k_segment_name >> -k_type_params)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_type_name_segment(
      std::move(boost::fusion::at_c<0>(attr)),
      boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_type_name_segment_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_type_name_segment_rule), Iterator_Type, Context_Type)

// Parse full qualified name: dot-separated qualified name segments
// Any segment can have type parameters, not just the last one!
// This allows names like "Container<T>.Iterator<Forward>" where intermediate segments have type parameters.
// Examples:
//   "Int"                                              - simple qualified name
//   "Std.String"                                       - multi-segment qualified name
//   "Std.Collections.Array<T>"                         - qualified with type parameter on last segment
//   "Std.Collections.Map<Key.Type, Value>"             - qualified segment with qualified type param
//   "Container<Int>.Iterator<Forward>"                 - multiple segments with type parameters
//   "Db.Table<User>.Column<Name>.Validator"            - type parameters in middle segments
//   "Parser<Token>.Result<AST>.Error<String>"          - multiple segments with type parameters in chain
//   "Network.Protocol<Http.Request, Http.Response>"    - deeply nested qualified type parameters
//   "IO.Result<Data.Error, Parser.AST>"                - multiple qualified params
auto const k_type_name_rule_def =
    (k_type_name_segment_rule %
     (lit('.') >>
      !lit('.')))[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_type_name(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_type_name_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_type_name_rule), Iterator_Type, Context_Type)

// === Variable Name Rules ===
// Variable and function names with optional template parameters
// Parser is flexible - accepts any identifier pattern
// Naming conventions enforced at semantic analysis:
//   - Variables/functions: snake_case
//   - Modules: Camel_Snake_Case

// Forward declarations for mutually recursive rules
struct Variable_Name_Tag : x3::annotate_on_success, Error_Handler {};
struct Variable_Name_Segment_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Variable_Name_Tag, ast::Variable_Name> const k_variable_name_rule = "variable name";
x3::rule<Variable_Name_Segment_Tag, ast::Variable_Name_Segment> const k_variable_name_segment_rule =
    "variable name segment";

// Parse variable name segment with optional type parameters
// Examples: "foo", "map<Int, String>", "Vec<T>"
auto const k_variable_name_segment_rule_def = (k_segment_name >> -k_type_params)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_variable_name_segment(
      std::move(boost::fusion::at_c<0>(attr)),
      boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_variable_name_segment_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_variable_name_segment_rule), Iterator_Type, Context_Type)

// Parse variable name: single segment for variable references in expressions
// Examples: "foo", "my_var", "x", "map<Int>"
// Field access handles dotted expressions: "obj.field" → Field_Access_Expr
auto const k_variable_name_rule_def = k_variable_name_segment_rule[([](auto& a_ctx) {
  x3::_val(a_ctx) = ast::make_variable_name(std::move(x3::_attr(a_ctx)));
})];
BOOST_SPIRIT_DEFINE(k_variable_name_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_variable_name_rule), Iterator_Type, Context_Type)

// Parse qualified variable name: dotted path for function calls
// Supports module-qualified function names with templates: "Std.print", "Vec<Int>.new"
// Examples: "foo", "Std.print", "A.B.func<T, U>"
x3::rule<struct qualified_variable_name_tag, ast::Variable_Name> const k_qualified_variable_name_rule =
    "qualified variable name";
auto const k_qualified_variable_name_rule_def =
    (k_variable_name_segment_rule %
     (lit('.') >>
      !lit('.')))[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_variable_name(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_qualified_variable_name_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_qualified_variable_name_rule), Iterator_Type, Context_Type)

// === String Literal Rules ===
// String literals with escape sequences and UTF-8 support
// Examples:
//   Simple:   "hello"
//   Escaped:  "hello\nworld", "say \"hi\""
//   Hex:      "null byte: \x00"
//   UTF-8:    "hello 世界" "emoji: 🎉"
// Note: Using x3::char_ (not x3::ascii::char_) to support UTF-8 content
auto const k_escaped_char = lexeme[lit('\\') > (x3::char_("\"'\\ntr") | ('x' > x3::repeat(2)[x3::xdigit]))];

// Parse string literal: quoted text with escape sequences
struct String_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<String_Tag, ast::String> const k_string_rule = "string literal";
auto const k_string_rule_def =
    raw[lexeme[(lit('"') > *(k_escaped_char | (x3::char_ - '"' - '\\'))) > lit('"')]][([](auto& a_ctx) {
      auto const& raw_string = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_string(std::string(raw_string.begin(), raw_string.end()));
    })];
BOOST_SPIRIT_DEFINE(k_string_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_string_rule), Iterator_Type, Context_Type)

// === Character Literal Rules ===
// Character literals with escape sequences and UTF-8 support
// Examples:
//   Simple:   'a', 'X', '9'
//   Escaped:  '\n', '\t', '\r', '\\', '\''
//   Hex:      '\x41' (letter A)
//   UTF-8:    '世', '🎉'
// Note: Using x3::char_ (not x3::ascii::char_) to support UTF-8 content
// For UTF-8, we need to handle:
// - 1 byte: 0xxxxxxx (ASCII)
// - 2 bytes: 110xxxxx 10xxxxxx
// - 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
// - 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// Continuation bytes: 10xxxxxx
auto const k_utf8_continuation = x3::char_('\x80', '\xBF');
auto const k_utf8_2byte = x3::char_('\xC0', '\xDF') >> k_utf8_continuation;
auto const k_utf8_3byte = x3::char_('\xE0', '\xEF') >> k_utf8_continuation >> k_utf8_continuation;
auto const k_utf8_4byte =
    x3::char_('\xF0', '\xF7') >> k_utf8_continuation >> k_utf8_continuation >> k_utf8_continuation;
auto const k_utf8_char =
    k_utf8_4byte | k_utf8_3byte | k_utf8_2byte | (x3::char_ - '\'' - '\\' - x3::char_('\x80', '\xFF'));

struct Char_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Char_Tag, ast::Char> const k_char_rule = "character literal";
auto const k_char_rule_def = raw[lexeme[(lit('\'') > (k_escaped_char | k_utf8_char) > lit('\''))]][([](auto& a_ctx) {
  auto const& raw_char = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_char(std::string(raw_char.begin(), raw_char.end()));
})];
BOOST_SPIRIT_DEFINE(k_char_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_char_rule), Iterator_Type, Context_Type)

// === Integer Literal Rules ===
// Integer literals with optional digit separators and type suffix
// Examples:
//   Simple:      0, 42, 123
//   Separated:   1_000_000, 123_456
//   With suffix: 42I32, 255U8, 1000I64
//   Invalid:     01 (leading zero), 123_ (trailing underscore)
// Suffixes: I8, I16, I32, I64, U8, U16, U32, U64 (uppercase)
struct Integer_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Integer_Tag, ast::Integer> const k_integer_rule = "integer literal";
auto const k_integer_rule_def = raw
    [lexeme[((lit('0') >> !(digit | '_')) | (char_('1', '9') >> *(char_('0', '9') | '_'))) >> -(char_("IU") >> +digit)]]
    [([](auto& a_ctx) {
      auto const& attr = x3::_attr(a_ctx);
      std::string const full{attr.begin(), attr.end()};

      // Split value and suffix
      std::string value;
      boost::optional<std::string> suffix;

      // Find where suffix starts (I or U followed by digits)
      auto suffix_start = full.find_first_of("IU");
      if (suffix_start != std::string::npos) {
        value = full.substr(0, suffix_start);
        suffix = full.substr(suffix_start);
      } else {
        value = full;
      }

      if (!value.empty() && value.back() == '_') {
        x3::_pass(a_ctx) = false;  // Reject trailing underscore
        return;
      }
      std::erase(value, '_');  // Remove digit separators

      x3::_val(a_ctx) = ast::make_integer(value, suffix);
    })];
BOOST_SPIRIT_DEFINE(k_integer_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_integer_rule), Iterator_Type, Context_Type)

// === Float Literal Rules ===
// Float literals with optional digit separators, scientific notation, and type suffix
// Examples:
//   Simple:      3.14, 0.5, 123.456
//   Separated:   1_000.5, 123_456.789_012
//   Scientific:  1.0e10, 2.5E-3, 1e+5
//   With suffix: 3.14F32, 2.5F64, 1.0e10F64
//   Edge cases:  0.0, 1.0, .5 (leading dot not allowed)
// Suffixes: F32, F64 (uppercase)
struct Float_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Float_Tag, ast::Float> const k_float_rule = "float literal";
auto const k_float_rule_def =
    raw[lexeme
            [(((+(char_('0', '9') | '_') >> '.' >> +(char_('0', '9') | '_') >>
                -(x3::no_case[char_('e')] >> -char_("+-") >> +(char_('0', '9') | '_'))) |
               (+(char_('0', '9') | '_') >> x3::no_case[char_('e')] >> -char_("+-") >> +(char_('0', '9') | '_')))) >>
             -(char_('F') >> +digit)]][([](auto& a_ctx) {
      auto const& attr = x3::_attr(a_ctx);
      std::string const full{attr.begin(), attr.end()};

      // Split value and suffix
      std::string value;
      boost::optional<std::string> suffix;

      // Find where suffix starts (F followed by digits)
      auto suffix_start = full.find('F');
      if (suffix_start != std::string::npos) {
        value = full.substr(0, suffix_start);
        suffix = full.substr(suffix_start);
      } else {
        value = full;
      }

      // Find dot and 'e'/'E' positions
      auto const dot_pos = value.find('.');
      auto const e_pos = value.find_first_of("eE");

      // Check for trailing underscore in different parts
      if (dot_pos != std::string::npos && dot_pos > 0 && value[dot_pos - 1] == '_') {
        x3::_pass(a_ctx) = false;  // Reject trailing _ before dot
        return;
      }
      if (e_pos != std::string::npos) {
        if (e_pos > 0 && value[e_pos - 1] == '_') {
          x3::_pass(a_ctx) = false;  // Reject trailing _ before e
          return;
        }
        // Check if there's a +/- after 'e'/'E'
        auto const sign_pos =
            (e_pos + 1 < value.size() && (value[e_pos + 1] == '+' || value[e_pos + 1] == '-')) ? e_pos + 1 : e_pos;
        if (sign_pos + 1 < value.size() && value[sign_pos + 1] == '_') {
          x3::_pass(a_ctx) = false;  // Reject leading _ after e or e+/-
          return;
        }
        // Check for trailing _ after the exponent
        if (!value.empty() && value.back() == '_') {
          x3::_pass(a_ctx) = false;  // Reject trailing _ at end of exponent
          return;
        }
      } else if (!value.empty() && value.back() == '_') {
        x3::_pass(a_ctx) = false;  // Reject trailing _ at end (no exponent case)
        return;
      }

      // Remove all underscores
      std::erase(value, '_');
      x3::_val(a_ctx) = ast::make_float(value, suffix);
    })];
BOOST_SPIRIT_DEFINE(k_float_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_float_rule), Iterator_Type, Context_Type)

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

// Parse parameter name: any identifier (naming convention checked at semantic analysis)
x3::rule<struct param_name_tag, std::string> const k_param_name = "parameter name";
auto const k_param_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_param_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_param_name), Iterator_Type, Context_Type)

// Parse parameter type: type name
x3::rule<struct param_type_tag, ast::Type_Name> const k_param_type = "parameter type";
auto const k_param_type_def = k_type_name_rule;
BOOST_SPIRIT_DEFINE(k_param_type)
BOOST_SPIRIT_INSTANTIATE(decltype(k_param_type), Iterator_Type, Context_Type)

// Parse function parameter: "name: Type" or "mut name: Type"
// Example: "x: Int", "mut self: Point", "callback: Fn<String, Bool>"
auto const k_function_parameter_rule_def =
    ((x3::matches[k_kw_mut] >> k_param_name) > ':' > k_param_type)[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_function_parameter(
          boost::fusion::at_c<0>(attr),             // is_mut: bool
          std::move(boost::fusion::at_c<1>(attr)),  // name: string
          std::move(boost::fusion::at_c<2>(attr))   // type: Type_Name
      );
    })];
BOOST_SPIRIT_DEFINE(k_function_parameter_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_parameter_rule), Iterator_Type, Context_Type)

// Parse function name: any identifier
x3::rule<struct func_name_tag, std::string> const k_func_name = "function name";
auto const k_func_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_func_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_func_name), Iterator_Type, Context_Type)

// Parse function parameters list: comma-separated, optional
x3::rule<struct func_params_tag, std::vector<ast::Function_Parameter>> const k_func_params = "function parameters";
auto const k_func_params_def = k_function_parameter_rule % ',';
BOOST_SPIRIT_DEFINE(k_func_params)
BOOST_SPIRIT_INSTANTIATE(decltype(k_func_params), Iterator_Type, Context_Type)

// Parse function return type: type name
x3::rule<struct func_return_type_tag, ast::Type_Name> const k_func_return_type = "return type";
auto const k_func_return_type_def = k_type_name_rule;
BOOST_SPIRIT_DEFINE(k_func_return_type)
BOOST_SPIRIT_INSTANTIATE(decltype(k_func_return_type), Iterator_Type, Context_Type)

// Parse function declaration: "fn name<T>(params): ReturnType"
// Examples:
//   fn main(): I32
//   fn add(a: Int, b: Int): Int
//   fn process(data: Array<String>): Result<(), Error>
//   fn map<T, U>(items: Array<T>): Array<U>
auto const k_function_declaration_rule_def =
    (k_kw_fn > k_func_name > -k_type_params > '(' > -k_func_params > ')' > ':' > k_func_return_type)[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_function_declaration(
          std::move(boost::fusion::at_c<0>(attr)),
          boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{}),
          boost::fusion::at_c<2>(attr).value_or(std::vector<ast::Function_Parameter>{}),
          std::move(boost::fusion::at_c<3>(attr))
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
struct If_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct While_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct For_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Range_Expr_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Expr_Tag, ast::Expr> const k_expr_rule = "expression";
x3::rule<Function_Call_Expr_Tag, ast::Function_Call_Expr> const k_function_call_expr_rule = "function call expression";
x3::rule<If_Expr_Tag, ast::If_Expr> const k_if_expr_rule = "if expression";
x3::rule<While_Expr_Tag, ast::While_Expr> const k_while_expr_rule = "while expression";
x3::rule<For_Expr_Tag, ast::For_Expr> const k_for_expr_rule = "for expression";
x3::rule<Range_Expr_Tag, ast::Expr> const k_range_expr_rule = "range expression";

// Parse function call name: qualified variable name (supports module paths)
x3::rule<struct call_name_tag, ast::Variable_Name> const k_call_name = "function name";
auto const k_call_name_def = k_qualified_variable_name_rule;
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
auto const k_function_call_expr_rule_def = (k_call_name >> '(' > -k_call_args > ')')[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_function_call_expr(
      std::move(boost::fusion::at_c<0>(attr)),
      boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Expr>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_function_call_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_call_expr_rule), Iterator_Type, Context_Type)

// Parse struct literal: "TypeName { field: expr, field: expr }"
// Examples:
//   Empty:        Point { }
//   With fields:  Point { x: 1, y: 2 }
//   Nested:       Line { start: Point { x: 0, y: 0 }, end: Point { x: 1, y: 1 } }
struct Field_Initializer_Tag : x3::annotate_on_success, Error_Handler {};
struct Struct_Literal_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Field_Initializer_Tag, ast::Field_Initializer> const k_field_initializer_rule = "field initializer";
x3::rule<Struct_Literal_Tag, ast::Struct_Literal> const k_struct_literal_rule = "struct literal";

// Parse field name in initializer: any identifier (naming convention checked at semantic analysis)
x3::rule<struct field_init_name_tag, std::string> const k_field_init_name = "field name";
auto const k_field_init_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_field_init_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_field_init_name), Iterator_Type, Context_Type)

// Parse field initializer: "name: expr"
auto const k_field_initializer_rule_def = (k_field_init_name > ':' > k_expr_rule)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) =
      ast::make_field_initializer(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
})];
BOOST_SPIRIT_DEFINE(k_field_initializer_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_field_initializer_rule), Iterator_Type, Context_Type)

// Parse field initializers: comma-separated with optional trailing comma
x3::rule<struct field_initializers_tag, std::vector<ast::Field_Initializer>> const k_field_initializers =
    "field initializers";
auto const k_field_initializers_def = (k_field_initializer_rule % ',') >> -lit(',');
BOOST_SPIRIT_DEFINE(k_field_initializers)
BOOST_SPIRIT_INSTANTIATE(decltype(k_field_initializers), Iterator_Type, Context_Type)

// Parse Camel_Snake_Case identifier (for type names)
// Starts with uppercase letter, followed by alphanumeric or underscore
x3::rule<struct camel_snake_case_tag, std::string> const k_camel_snake_case = "Camel_Snake_Case identifier";
auto const k_camel_snake_case_def = raw[lexeme[x3::upper >> *(alnum | '_')]];
BOOST_SPIRIT_DEFINE(k_camel_snake_case)
BOOST_SPIRIT_INSTANTIATE(decltype(k_camel_snake_case), Iterator_Type, Context_Type)

// Parse struct literal: "TypeName { fields }"
// Type name: Camel_Snake_Case identifier (enforced at parse time to prevent ambiguity)
// This prevents "if x {}" from being parsed as "if (x {})" where lowercase x matches struct literal
// Since variables use snake_case and types use Camel_Snake_Case, x{} won't match but Point{} will
x3::rule<struct struct_literal_type_tag, std::string> const k_struct_literal_type = "struct type name";
auto const k_struct_literal_type_def = k_camel_snake_case;
BOOST_SPIRIT_DEFINE(k_struct_literal_type)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_literal_type), Iterator_Type, Context_Type)

auto const k_struct_literal_rule_def = (k_struct_literal_type >> '{' > -k_field_initializers > '}')[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_struct_literal(
      std::move(boost::fusion::at_c<0>(attr)),
      boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Field_Initializer>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_struct_literal_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_literal_rule), Iterator_Type, Context_Type)

// Primary expressions (before postfix operations)
// Note: Function calls are included here - they parse "name(args)" as a complete unit
// Postfix operations then apply to the result: foo(x).bar or foo(x).baz()
// Note: If expressions are added to k_expr_rule (after block rules) to support expression context
x3::rule<struct primary_expr_tag, ast::Expr> const k_primary_expr = "primary expression";
// Ordering rationale:
// 1. struct_literal first: Requires Camel_Snake_Case + '{', most specific pattern
// 2. function_call second: Requires name + '(', specific delimiter
// 3. variable_name later: More general
// This prevents "if x {}" ambiguity: x{} won't match struct_literal (x is lowercase)
auto const k_primary_expr_def = k_struct_literal_rule | k_function_call_expr_rule | k_string_rule | k_char_rule |
                                k_variable_name_rule | k_float_rule | k_integer_rule;
BOOST_SPIRIT_DEFINE(k_primary_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_primary_expr), Iterator_Type, Context_Type)

// Postfix operations: field access (.field) or method call (.method(args))
// Helper structures for postfix chain
struct Postfix_Field_Access {
  std::string field_name;
};

struct Postfix_Method_Call {
  std::string method_name;
  std::vector<ast::Expr> arguments;
};

using Postfix_Op = std::variant<Postfix_Field_Access, Postfix_Method_Call>;

// Parse method arguments: same as function call arguments
x3::rule<struct method_args_tag, std::vector<ast::Expr>> const k_method_args = "method arguments";
auto const k_method_args_def = k_expr_rule % ',';
BOOST_SPIRIT_DEFINE(k_method_args)
BOOST_SPIRIT_INSTANTIATE(decltype(k_method_args), Iterator_Type, Context_Type)

// Parse method call postfix: .name(args)
// Use >> for initial parts to allow backtracking, then > after '(' commits us
x3::rule<struct postfix_method_call_tag, Postfix_Method_Call> const k_postfix_method_call = "method call";
auto const k_postfix_method_call_def = ('.' >> k_segment_name >> '(' > -k_method_args > ')')[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = Postfix_Method_Call{
      std::move(boost::fusion::at_c<0>(attr)),
      boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Expr>{})
  };
})];
BOOST_SPIRIT_DEFINE(k_postfix_method_call)
BOOST_SPIRIT_INSTANTIATE(decltype(k_postfix_method_call), Iterator_Type, Context_Type)

// Parse field access postfix: .name (but not .. for range operator)
x3::rule<struct postfix_field_access_tag, Postfix_Field_Access> const k_postfix_field_access = "field access";
auto const k_postfix_field_access_def =
    ((lit('.') >> !lit('.')) >
     k_segment_name)[([](auto& a_ctx) { x3::_val(a_ctx) = Postfix_Field_Access{std::move(x3::_attr(a_ctx))}; })];
BOOST_SPIRIT_DEFINE(k_postfix_field_access)
BOOST_SPIRIT_INSTANTIATE(decltype(k_postfix_field_access), Iterator_Type, Context_Type)

// Combined postfix operation: try method call first (longer match), then field access
x3::rule<struct postfix_op_tag, Postfix_Op> const k_postfix_op = "postfix operation";
auto const k_postfix_op_def = k_postfix_method_call | k_postfix_field_access;
BOOST_SPIRIT_DEFINE(k_postfix_op)
BOOST_SPIRIT_INSTANTIATE(decltype(k_postfix_op), Iterator_Type, Context_Type)

// Postfix expression: primary followed by zero or more postfix operations
// Supports: foo().bar, foo().bar(), foo.bar().baz, etc.
auto const k_postfix_expr_def = (k_primary_expr >> *k_postfix_op)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  ast::Expr expr = std::move(boost::fusion::at_c<0>(attr));
  auto const& ops = boost::fusion::at_c<1>(attr);

  // Build left-associative chain: apply each postfix operation in order
  for (auto const& op : ops) {
    std::visit(
        [&expr](auto const& a_operation) {
          using T = std::decay_t<decltype(a_operation)>;
          if constexpr (std::same_as<T, Postfix_Field_Access>) {
            // Field access: obj.field
            // i.e. a.b.c desugars to ((a.b).c)
            expr = ast::make_expr(ast::make_field_access_expr(std::move(expr), std::string(a_operation.field_name)));
          } else if constexpr (std::same_as<T, Postfix_Method_Call>) {
            // Method call: obj.method(args)
            // Create a variable name from method_name, then wrap in function call with obj as first arg
            // i.e. a().b(x, y).c(z) desugars to c(b(a(), x, y), z)
            auto method_var_name = ast::make_variable_name(std::string(a_operation.method_name));

            // Build argument list: [obj, ...args]
            std::vector<ast::Expr> all_args;
            all_args.reserve(1 + a_operation.arguments.size());
            all_args.push_back(std::move(expr));
            all_args.insert(all_args.end(), a_operation.arguments.begin(), a_operation.arguments.end());

            // Create function call expression
            expr = ast::make_expr(ast::make_function_call_expr(std::move(method_var_name), std::move(all_args)));
          }
        },
        op
    );
  }
  x3::_val(a_ctx) = std::move(expr);
})];
struct Postfix_Expr_Tag : x3::annotate_on_success {};
x3::rule<Postfix_Expr_Tag, ast::Expr> const k_postfix_expr = "postfix_expr";
BOOST_SPIRIT_DEFINE(k_postfix_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_postfix_expr), Iterator_Type, Context_Type)

// === Binary Operator Parsing with Precedence ===
// Operator precedence (from lowest to highest):
// 1. Logical OR:  ||
// 2. Logical AND: &&
// 3. Equality:    ==, !=
// 4. Comparison:  <, >, <=, >=
// 5. Additive:    +, -
// 6. Multiplicative: *, /, %

// Operator symbol tables mapping strings to enums
struct Multiplicative_Op_Symbols : x3::symbols<ast::Binary_Op> {
  Multiplicative_Op_Symbols() { add("*", ast::Binary_Op::Mul)("/", ast::Binary_Op::Div)("%", ast::Binary_Op::Mod); }
} const k_multiplicative_op;

struct Additive_Op_Symbols : x3::symbols<ast::Binary_Op> {
  Additive_Op_Symbols() { add("+", ast::Binary_Op::Add)("-", ast::Binary_Op::Sub); }
} const k_additive_op;

struct Comparison_Op_Symbols : x3::symbols<ast::Binary_Op> {
  Comparison_Op_Symbols() {
    add("<=", ast::Binary_Op::Le)(">=", ast::Binary_Op::Ge)("<", ast::Binary_Op::Lt)(">", ast::Binary_Op::Gt);
  }
} const k_comparison_op;

struct Equality_Op_Symbols : x3::symbols<ast::Binary_Op> {
  Equality_Op_Symbols() { add("==", ast::Binary_Op::Eq)("!=", ast::Binary_Op::Ne); }
} const k_equality_op;

struct Logical_And_Op_Symbols : x3::symbols<ast::Binary_Op> {
  Logical_And_Op_Symbols() { add("&&", ast::Binary_Op::And); }
} const k_logical_and_op;

struct Logical_Or_Op_Symbols : x3::symbols<ast::Binary_Op> {
  Logical_Or_Op_Symbols() { add("||", ast::Binary_Op::Or); }
} const k_logical_or_op;

// Unary operator symbols
struct Unary_Op_Symbols : x3::symbols<ast::Unary_Op> {
  Unary_Op_Symbols() {
    add("-", ast::Unary_Op::Neg)("+", ast::Unary_Op::Pos)("!", ast::Unary_Op::Not)("~", ast::Unary_Op::BitNot);
  }
} const k_unary_op;

// Forward declarations for precedence levels
struct Unary_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Multiplicative_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Additive_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Comparison_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Equality_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Logical_And_Expr_Tag : x3::annotate_on_success, Error_Handler {};
struct Logical_Or_Expr_Tag : x3::annotate_on_success, Error_Handler {};

x3::rule<Unary_Expr_Tag, ast::Expr> const k_unary_expr = "unary expression";
x3::rule<Multiplicative_Expr_Tag, ast::Expr> const k_multiplicative_expr = "multiplicative expression";
x3::rule<Additive_Expr_Tag, ast::Expr> const k_additive_expr = "additive expression";
x3::rule<Comparison_Expr_Tag, ast::Expr> const k_comparison_expr = "comparison expression";
x3::rule<Equality_Expr_Tag, ast::Expr> const k_equality_expr = "equality expression";
x3::rule<Logical_And_Expr_Tag, ast::Expr> const k_logical_and_expr = "logical AND expression";
x3::rule<Logical_Or_Expr_Tag, ast::Expr> const k_logical_or_expr = "logical OR expression";

// Build left-associative binary expression chains
// Used by most precedence levels where operands are already Expr
auto const k_build_binary_expr = [](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  ast::Expr lhs = std::move(boost::fusion::at_c<0>(attr));
  auto const& rhs_list = boost::fusion::at_c<1>(attr);

  // Build left-associative chain: ((lhs op rhs1) op rhs2) ...
  for (auto const& rhs_pair : rhs_list) {
    ast::Binary_Op const op = boost::fusion::at_c<0>(rhs_pair);
    ast::Expr rhs = std::move(boost::fusion::at_c<1>(rhs_pair));
    lhs = ast::make_expr(ast::make_binary_expr(std::move(lhs), op, std::move(rhs)));
  }
  x3::_val(a_ctx) = std::move(lhs);
};

// Level 6 (Highest): Unary (-, +, !, ~)
// Right-associative: -(-x) means -((-x))
// Handles prefix operators applied to postfix expressions
auto const k_unary_expr_def = (*(k_unary_op) >> k_postfix_expr)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  auto const& ops = boost::fusion::at_c<0>(attr);
  ast::Expr expr = std::move(boost::fusion::at_c<1>(attr));

  // Build right-to-left: op1(op2(expr))
  for (auto it = ops.rbegin(); it != ops.rend(); ++it) {
    expr = ast::make_expr(ast::make_unary_expr(*it, std::move(expr)));
  }
  x3::_val(a_ctx) = std::move(expr);
})];
BOOST_SPIRIT_DEFINE(k_unary_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_unary_expr), Iterator_Type, Context_Type)

// Level 5: Multiplicative (*, /, %)
auto const k_multiplicative_expr_def = (k_unary_expr >> *(k_multiplicative_op >> k_unary_expr))[k_build_binary_expr];
BOOST_SPIRIT_DEFINE(k_multiplicative_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_multiplicative_expr), Iterator_Type, Context_Type)

// Level 4: Additive (+, -)
auto const k_additive_expr_def =
    (k_multiplicative_expr >> *(k_additive_op >> k_multiplicative_expr))[k_build_binary_expr];
BOOST_SPIRIT_DEFINE(k_additive_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_additive_expr), Iterator_Type, Context_Type)

// Range expression: start..end (exclusive) or start..=end (inclusive)
// Precedence: between arithmetic and comparison
// Examples: 0..10, start..end, 1..=100
// Strategy: Try to parse range first (start..end or start..=end), if no range operator found, just return additive
auto const k_range_inclusive = lit("..=") >> x3::attr(true);
auto const k_range_exclusive = lit("..") >> !lit('.') >> x3::attr(false);  // Ensure we don't match ... or more dots
auto const k_range_expr_rule_def = k_additive_expr[([](auto& a_ctx) {
                                     // Start with just the additive expr
                                     x3::_val(a_ctx) = x3::_attr(a_ctx);
                                   })] >>
                                   -((k_range_inclusive | k_range_exclusive) >> k_additive_expr)[([](auto& a_ctx) {
                                     // If we see range operator, wrap in Range_Expr
                                     auto& attr = x3::_attr(a_ctx);
                                     auto const inclusive = boost::fusion::at_c<0>(attr);
                                     auto& end = boost::fusion::at_c<1>(attr);
                                     bool const is_inclusive = boost::get<bool>(inclusive);
                                     // _val already contains the start expression
                                     x3::_val(a_ctx) = ast::make_expr(
                                         ast::make_range_expr(std::move(x3::_val(a_ctx)), std::move(end), is_inclusive)
                                     );
                                   })];
BOOST_SPIRIT_DEFINE(k_range_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_range_expr_rule), Iterator_Type, Context_Type)

// Level 3: Comparison (<, >, <=, >=)
auto const k_comparison_expr_def = (k_range_expr_rule >> *(k_comparison_op >> k_range_expr_rule))[k_build_binary_expr];
BOOST_SPIRIT_DEFINE(k_comparison_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_comparison_expr), Iterator_Type, Context_Type)

// Level 2: Equality (==, !=)
auto const k_equality_expr_def = (k_comparison_expr >> *(k_equality_op >> k_comparison_expr))[k_build_binary_expr];
BOOST_SPIRIT_DEFINE(k_equality_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_equality_expr), Iterator_Type, Context_Type)

// Level 1: Logical AND (&&)
auto const k_logical_and_expr_def = (k_equality_expr >> *(k_logical_and_op >> k_equality_expr))[k_build_binary_expr];
BOOST_SPIRIT_DEFINE(k_logical_and_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_logical_and_expr), Iterator_Type, Context_Type)

// Level 0: Logical OR (||) - lowest precedence
auto const k_logical_or_expr_def =
    (k_logical_and_expr >> *(k_logical_or_op >> k_logical_and_expr))[k_build_binary_expr];
BOOST_SPIRIT_DEFINE(k_logical_or_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_logical_or_expr), Iterator_Type, Context_Type)

// Non-struct expression chain (for if condition context) - builds complete precedence chain without struct literals

// Parse expression: start with lowest precedence (logical OR)
// Note: k_expr_rule definition moved after k_if_expr_rule to include if expressions
// auto const k_expr_rule_def = k_logical_or_expr;
// BOOST_SPIRIT_DEFINE(k_expr_rule)
// BOOST_SPIRIT_INSTANTIATE(decltype(k_expr_rule), Iterator_Type, Context_Type)

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

// Parse break statement: "break;" or "break expr;" (Phase 2: expression-form loops)
// Examples: "break;", "break result;", "break calculate(x);"
struct Break_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Break_Statement_Tag, ast::Break_Statement> const k_break_statement_rule = "break statement";
auto const k_break_statement_rule_def =
    ((k_kw_break > -k_expr_rule) >
     ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_break_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_break_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_break_statement_rule), Iterator_Type, Context_Type)

// Examples: "continue;"
struct Continue_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Continue_Statement_Tag, ast::Continue_Statement> const k_continue_statement_rule = "continue statement";
auto const k_continue_statement_rule_def =
    (k_kw_continue > ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_continue_statement(); })];
BOOST_SPIRIT_DEFINE(k_continue_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_continue_statement_rule), Iterator_Type, Context_Type)

// Forward declaration for pattern rule (used in let_statement and for_expr)
struct Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Pattern_Tag, ast::Pattern> const k_pattern_rule = "pattern";

// Parse let statement: "let x = expr;" or "let mut y: Type = expr;" or "let (a, b) = tuple;"
// Introduces a new binding with optional type annotation and optional mutability
// Examples: "let x = 42;", "let mut count: I32 = 0;", "let (x, y) = point;"
struct Let_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Let_Statement_Tag, ast::Let_Statement> const k_let_statement_rule = "let statement";
auto const k_let_statement_rule_def =
    (k_kw_let > x3::matches[k_kw_mut] > k_pattern_rule > -(':' > k_type_name_rule) > '=' > k_expr_rule >
     ';')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      // Attribute structure: (is_mut: bool, Pattern, optional<Type_Name>, Expr)
      bool const is_mut = boost::fusion::at_c<0>(attr);
      auto& pattern = boost::fusion::at_c<1>(attr);
      auto& opt_type = boost::fusion::at_c<2>(attr);
      auto& value = boost::fusion::at_c<3>(attr);

      x3::_val(a_ctx) = ast::make_let_statement(is_mut, std::move(pattern), std::move(opt_type), std::move(value));
    })];
BOOST_SPIRIT_DEFINE(k_let_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_let_statement_rule), Iterator_Type, Context_Type)

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

// Parse expression statement: any expression followed by semicolon
// In practice, only assignments and function calls have side effects and should be used as statements
// But we accept any expression for now - semantic analysis will warn about useless statements later
// Examples: "x = 42;", "y = y + 1;", "obj.field = value;", "foo();"
struct Expression_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Expression_Statement_Tag, ast::Expression_Statement> const k_expression_statement_rule =
    "expression statement";
auto const k_expression_statement_rule_def =
    (k_expr_rule >>
     ';')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_expression_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_expression_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_expression_statement_rule), Iterator_Type, Context_Type)

// Parse if statement: forward declared here, defined after k_if_expr_rule
struct If_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<If_Statement_Tag, ast::If_Statement> const k_if_statement_rule = "if statement";

// Parse while statement: forward declared here, defined after k_while_expr_rule
struct While_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<While_Statement_Tag, ast::While_Statement> const k_while_statement_rule = "while statement";

// Parse for statement: forward declared here, defined after k_for_expr_rule
struct For_Statement_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<For_Statement_Tag, ast::For_Statement> const k_for_statement_rule = "for statement";

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
struct Struct_Field_Tag : x3::annotate_on_success, Error_Handler {};
struct Struct_Definition_Tag : x3::annotate_on_success, Error_Handler {};
struct Module_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Statement_Tag, ast::Statement> const k_statement_rule = "statement";
x3::rule<Block_Tag, ast::Block> const k_block_rule = "code block";
x3::rule<Function_Definition_Tag, ast::Function_Definition> const k_function_definition_rule = "function definition";
x3::rule<Struct_Field_Tag, ast::Struct_Field> const k_struct_field_rule = "struct field";
x3::rule<Struct_Definition_Tag, ast::Struct_Definition> const k_struct_definition_rule = "struct definition";
x3::rule<Module_Tag, ast::Module> const k_module_rule = "module";

// Parse block: "{ statements }"
// Example: { print("hi"); return 0; }
// Note: Inlined *k_statement_rule to avoid intermediate k_statements rule
auto const k_block_rule_def =
    (('{' > *k_statement_rule) >
     '}')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_block(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_block_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_block_rule), Iterator_Type, Context_Type)

// If expression: if condition { then_block } (else if condition { then_block })* else { else_block }
// Syntax: no parentheses around condition, else is optional for statements, supports else-if chains
// Note: Struct literal ambiguity resolved by Camel_Snake_Case enforcement (see k_camel_snake_case)
auto const k_if_expr_rule_def =
    ((k_kw_if > k_logical_or_expr > k_block_rule) >> *((k_kw_else >> k_kw_if >> k_logical_or_expr) > k_block_rule) >>
     -(k_kw_else > k_block_rule))[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      auto& condition = boost::fusion::at_c<0>(attr);       // Expr
      auto& then_block = boost::fusion::at_c<1>(attr);      // Block
      auto& else_if_tuples = boost::fusion::at_c<2>(attr);  // vector<tuple<Expr, Block>>
      auto& else_block_opt = boost::fusion::at_c<3>(attr);  // optional<Block>

      // Build else_ifs vector from the tuples
      std::vector<ast::Else_If_Clause> else_ifs;
      for (auto& tuple : else_if_tuples) {
        auto& else_if_condition = boost::fusion::at_c<0>(tuple);
        auto& else_if_then_block = boost::fusion::at_c<1>(tuple);
        else_ifs.push_back(ast::make_else_if_clause(std::move(else_if_condition), std::move(else_if_then_block)));
      }

      boost::optional<ast::Block> else_block;
      if (else_block_opt.has_value()) {
        else_block = std::move(else_block_opt.value());
      }
      x3::_val(a_ctx) =
          ast::make_if_expr(std::move(condition), std::move(then_block), std::move(else_ifs), std::move(else_block));
    })];
BOOST_SPIRIT_DEFINE(k_if_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_if_expr_rule), Iterator_Type, Context_Type)

// While expression: while condition { body }
// Syntax: no parentheses around condition, consistent with if expression
// Examples: "while x < 10 { x = x + 1; }", "while has_items() { process(); }"
auto const k_while_expr_rule_def = (k_kw_while > k_logical_or_expr > k_block_rule)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  auto& condition = boost::fusion::at_c<0>(attr);  // Expr
  auto& body = boost::fusion::at_c<1>(attr);       // Block
  x3::_val(a_ctx) = ast::make_while_expr(std::move(condition), std::move(body));
})];
BOOST_SPIRIT_DEFINE(k_while_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_while_expr_rule), Iterator_Type, Context_Type)

// Pattern parsing for for-loop bindings (supports nested patterns)
// Wildcard pattern: _ (matches anything, doesn't bind)
struct Wildcard_Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Wildcard_Pattern_Tag, ast::Wildcard_Pattern> const k_wildcard_pattern_rule = "wildcard pattern";
auto const k_wildcard_pattern_rule_def =
    lit('_')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_wildcard_pattern(); })];
BOOST_SPIRIT_DEFINE(k_wildcard_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_wildcard_pattern_rule), Iterator_Type, Context_Type)

// Literal pattern: integer, float, string, or char literal (matches exact value)
struct Literal_Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Literal_Pattern_Tag, ast::Literal_Pattern> const k_literal_pattern_rule = "literal pattern";
auto const k_literal_pattern_rule_def =
    (k_integer_rule[([](auto& a_ctx) {
       x3::_val(a_ctx) = ast::make_literal_pattern(ast::make_expr(std::move(x3::_attr(a_ctx))));
     })] |
     k_float_rule[([](auto& a_ctx) {
       x3::_val(a_ctx) = ast::make_literal_pattern(ast::make_expr(std::move(x3::_attr(a_ctx))));
     })] |
     k_string_rule[([](auto& a_ctx) {
       x3::_val(a_ctx) = ast::make_literal_pattern(ast::make_expr(std::move(x3::_attr(a_ctx))));
     })] |
     k_char_rule[([](auto& a_ctx) {
       x3::_val(a_ctx) = ast::make_literal_pattern(ast::make_expr(std::move(x3::_attr(a_ctx))));
     })]);
BOOST_SPIRIT_DEFINE(k_literal_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_literal_pattern_rule), Iterator_Type, Context_Type)

// Simple pattern: just an identifier (for item in ...)
struct Simple_Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Simple_Pattern_Tag, ast::Simple_Pattern> const k_simple_pattern_rule = "simple pattern";
auto const k_simple_pattern_rule_def =
    k_segment_name[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_simple_pattern(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_simple_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_simple_pattern_rule), Iterator_Type, Context_Type)

// Field pattern: name: pattern (for struct pattern matching)
// Supports two forms:
//   - Explicit: x: 3, y: px (field x matches 3, field y binds to px)
//   - Shorthand: x, y (equivalent to x: x, y: y - binds field to same-named variable)
struct Field_Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Field_Pattern_Tag, ast::Field_Pattern> const k_field_pattern_rule = "field pattern";
auto const k_field_pattern_rule_def =
    // Explicit form: name: pattern
    ((k_segment_name >> ':' > k_pattern_rule)[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) =
          ast::make_field_pattern(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
    })]) |
    // Shorthand form: name (desugars to name: name)
    (k_segment_name[([](auto& a_ctx) {
      auto& name = x3::_attr(a_ctx);
      auto pattern = ast::make_pattern(ast::make_simple_pattern(std::string(name)));
      x3::_val(a_ctx) = ast::make_field_pattern(std::move(name), std::move(pattern));
    })]);
BOOST_SPIRIT_DEFINE(k_field_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_field_pattern_rule), Iterator_Type, Context_Type)

// Struct pattern: Type { name: pattern, ... }
// Example: Point { x: 3, y: 4 } or nested: Pair { first: Point { x: 1, y: 2 }, second: 5 }
struct Struct_Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Struct_Pattern_Tag, ast::Struct_Pattern> const k_struct_pattern_rule = "struct pattern";
auto const k_struct_pattern_rule_def = (k_type_name_rule >> '{' > (k_field_pattern_rule % ',') > '}')[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  auto& type_name = boost::fusion::at_c<0>(attr);
  auto& field_patterns = boost::fusion::at_c<1>(attr);
  x3::_val(a_ctx) = ast::make_struct_pattern(std::move(type_name), std::move(field_patterns));
})];
BOOST_SPIRIT_DEFINE(k_struct_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_pattern_rule), Iterator_Type, Context_Type)

// Tuple pattern: (pattern1, pattern2, ...)
// Example: for (a, b) in pairs { } or nested: for (a, (b, c)) in ...
struct Tuple_Pattern_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Tuple_Pattern_Tag, ast::Tuple_Pattern> const k_tuple_pattern_rule = "tuple pattern";
auto const k_tuple_pattern_rule_def = (('(' > (k_pattern_rule % ',') > ')')[([](auto& a_ctx) {
  auto& patterns = x3::_attr(a_ctx);
  // Wrap each Pattern in forward_ast
  std::vector<x3::forward_ast<ast::Pattern>> elements;
  elements.reserve(patterns.size());
  for (auto& p : patterns) {
    elements.emplace_back(std::move(p));
  }
  x3::_val(a_ctx) = ast::make_tuple_pattern(std::move(elements));
})]);
BOOST_SPIRIT_DEFINE(k_tuple_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_tuple_pattern_rule), Iterator_Type, Context_Type)

// Pattern: try in order - struct, tuple, wildcard, literal, simple
// Order matters: literals must come before simple to match numbers
auto const k_pattern_rule_def =
    (k_struct_pattern_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_pattern(std::move(x3::_attr(a_ctx))); })]) |
    (k_tuple_pattern_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_pattern(std::move(x3::_attr(a_ctx))); })]) |
    (k_wildcard_pattern_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_pattern(std::move(x3::_attr(a_ctx))); })]) |
    (k_literal_pattern_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_pattern(std::move(x3::_attr(a_ctx))); })]) |
    (k_simple_pattern_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_pattern(std::move(x3::_attr(a_ctx))); })]);
BOOST_SPIRIT_DEFINE(k_pattern_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_pattern_rule), Iterator_Type, Context_Type)

// For expression: for pattern in iterator { body }
// Syntax: for <pattern> in <expr> { body }
// Examples:
//   Simple:  "for item in 0..10 { print(item); }"
//   Struct:  "for Point { x, y } in points { process(x, y); }"
auto const k_for_expr_rule_def =
    (k_kw_for > k_pattern_rule > k_kw_in > k_logical_or_expr > k_block_rule)[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      auto& pattern = boost::fusion::at_c<0>(attr);   // Pattern
      auto& iterator = boost::fusion::at_c<1>(attr);  // Expr
      auto& body = boost::fusion::at_c<2>(attr);      // Block
      x3::_val(a_ctx) = ast::make_for_expr(std::move(pattern), std::move(iterator), std::move(body));
    })];
BOOST_SPIRIT_DEFINE(k_for_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_for_expr_rule), Iterator_Type, Context_Type)

// Match arm: pattern [if guard] => result
// Examples: "0 => \"zero\"", "n if n > 0 => \"positive\"", "Point { x, y } => format(x, y)"
struct Match_Arm_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Match_Arm_Tag, ast::Match_Arm> const k_match_arm_rule = "match arm";
auto const k_match_arm_rule_def =
    (k_pattern_rule >> -(k_kw_if >> k_logical_or_expr) >> lit("=>") > k_expr_rule)[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      auto& pattern = boost::fusion::at_c<0>(attr);    // Pattern
      auto& guard_opt = boost::fusion::at_c<1>(attr);  // optional<Expr>
      auto& result = boost::fusion::at_c<2>(attr);     // Expr
      x3::_val(a_ctx) = ast::make_match_arm(std::move(pattern), std::move(guard_opt), std::move(result));
    })];
BOOST_SPIRIT_DEFINE(k_match_arm_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_match_arm_rule), Iterator_Type, Context_Type)

// Match expression: match scrutinee { arms }
// Syntax: match <expr> { <pattern> [if <guard>] => <expr>, ... }
// Examples:
//   Simple:  "match x { 0 => \"zero\", 1 => \"one\", _ => \"other\" }"
//   Guard:   "match x { n if n < 0 => \"neg\", 0 => \"zero\", _ => \"pos\" }"
//   Pattern: "match point { Point { x: 0, y: 0 } => \"origin\", Point { x, y } => format(x, y) }"
struct Match_Expr_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Match_Expr_Tag, ast::Match_Expr> const k_match_expr_rule = "match expression";
auto const k_match_expr_rule_def =
    (k_kw_match > k_logical_or_expr > '{' > (k_match_arm_rule % ',') > -lit(',') > '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      auto& scrutinee = boost::fusion::at_c<0>(attr);  // Expr
      auto& arms = boost::fusion::at_c<1>(attr);       // vector<Match_Arm>
      x3::_val(a_ctx) = ast::make_match_expr(std::move(scrutinee), std::move(arms));
    })];
BOOST_SPIRIT_DEFINE(k_match_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_match_expr_rule), Iterator_Type, Context_Type)

// Parse assignment expression: "target = value" (right-associative, lowest precedence)
// Target must be an lvalue (variable or field access) - checked in semantic analysis
// Examples: "x = 42", "point.x = 10", "count = count + 1", "x = y = z" (right-associative)
struct Assignment_Expr_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Assignment_Expr_Tag, ast::Expr> const k_assignment_expr_rule = "assignment expression";
// Right-associative: parse as (non_assignment = assignment_expr)
// Non-assignment is: if | while | for | match | logical_or (everything except assignment)
auto const k_non_assignment_expr =
    k_if_expr_rule | k_while_expr_rule | k_for_expr_rule | k_match_expr_rule | k_logical_or_expr;
auto const k_assignment_expr_rule_def = (k_non_assignment_expr >> '=' > k_expr_rule)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  // Extract Expr from variant<If_Expr, While_Expr, For_Expr, Expr>
  auto& target_variant = boost::fusion::at_c<0>(attr);
  ast::Expr target;
  // Visit the variant to wrap each alternative in Expr (or just move if already Expr)
  boost::apply_visitor(
      [&target](auto& a_value) {
        using T = std::remove_cvref_t<decltype(a_value)>;
        if constexpr (std::is_same_v<T, ast::Expr>) {
          target = std::move(a_value);  // Already Expr, just move
        } else {
          target = ast::make_expr(std::move(a_value));  // Wrap in Expr
        }
      },
      target_variant
  );
  x3::_val(a_ctx) =
      ast::make_expr(ast::make_assignment_expr(std::move(target), std::move(boost::fusion::at_c<1>(attr))));
})];
BOOST_SPIRIT_DEFINE(k_assignment_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_assignment_expr_rule), Iterator_Type, Context_Type)

// Parse expression: assignment has lowest precedence (right-associative)
// Try assignment first (it includes all other expression types), then fall back to non-assignment expressions
auto const k_expr_rule_def = k_assignment_expr_rule | k_non_assignment_expr;
BOOST_SPIRIT_DEFINE(k_expr_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_expr_rule), Iterator_Type, Context_Type)

// Parse if statement: if expression used as a statement (no semicolon needed)
// Examples: "if x { do_something(); }", "if a > b { return a; } else { return b; }"
auto const k_if_statement_rule_def =
    k_if_expr_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_if_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_if_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_if_statement_rule), Iterator_Type, Context_Type)

// Parse while statement: while expression used as a statement (no semicolon needed)
// Examples: "while x < 10 { process(x); }", "while !done { work(); }"
auto const k_while_statement_rule_def =
    k_while_expr_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_while_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_while_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_while_statement_rule), Iterator_Type, Context_Type)

// Parse for statement: for expression used as a statement (no semicolon needed)
// Examples: "for item in 0..10 { process(item); }", "for user in users { handle(user); }"
auto const k_for_statement_rule_def =
    k_for_expr_rule[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_for_statement(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_for_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_for_statement_rule), Iterator_Type, Context_Type)

// Parse function definition: declaration followed by body block
// Example: fn main(): I32 { return 0; }
auto const k_function_definition_rule_def = (k_function_declaration_rule > k_block_rule)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) =
      ast::make_function_definition(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
})];
BOOST_SPIRIT_DEFINE(k_function_definition_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_function_definition_rule), Iterator_Type, Context_Type)

// === Struct Rules ===
// Structs define user-defined data types with named fields
// Examples:
//   struct Point { x: I32, y: I32 }
//   struct Person { name: String, age: I32 }
//   struct Node { value: T, next: Option<Node> }

// Parse struct field name: any identifier (naming convention checked at semantic analysis)
x3::rule<struct struct_field_name_tag, std::string> const k_struct_field_name = "struct field name";
auto const k_struct_field_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_struct_field_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_field_name), Iterator_Type, Context_Type)

// Parse struct field: "name: Type"
// Examples: x: I32, name: String, items: Vec<T>
auto const k_struct_field_rule_def = (k_struct_field_name > ':' > k_type_name_rule)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) =
      ast::make_struct_field(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
})];
BOOST_SPIRIT_DEFINE(k_struct_field_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_field_rule), Iterator_Type, Context_Type)

// Parse struct fields: comma-separated list of fields with optional trailing comma
// Examples: "x: I32, y: I32" or "x: I32, y: I32,"
x3::rule<struct struct_fields_tag, std::vector<ast::Struct_Field>> const k_struct_fields = "struct fields";
auto const k_struct_fields_def = (k_struct_field_rule % ',') >> -lit(',');
BOOST_SPIRIT_DEFINE(k_struct_fields)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_fields), Iterator_Type, Context_Type)

// Parse struct name: any identifier (naming convention checked at semantic analysis)
// Rule wrapper needed for proper attribute extraction in semantic action
x3::rule<struct struct_name_tag, std::string> const k_struct_name = "struct name";
auto const k_struct_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_struct_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_name), Iterator_Type, Context_Type)

// Parse struct definition: "struct Name<T> { fields }"
// Examples:
//   struct Point { x: I32, y: I32 }
//   struct Empty { }
//   struct Box<T> { value: T }
//   struct Map<K, V> { keys: Vec<K>, values: Vec<V> }
auto const k_struct_definition_rule_def =
    (k_kw_struct > k_struct_name > -k_type_params > '{' > -k_struct_fields > '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_struct_definition(
          std::move(boost::fusion::at_c<0>(attr)),
          boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{}),
          boost::fusion::at_c<2>(attr).value_or(std::vector<ast::Struct_Field>{})
      );
    })];
BOOST_SPIRIT_DEFINE(k_struct_definition_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_definition_rule), Iterator_Type, Context_Type)

// === Enum Rules ===
// Enums define sum types (algebraic data types) with multiple variants
// Examples:
//   enum Option<T> { Some(T), None }
//   enum Color { Red, Green, Blue }
//   enum Result<T, E> { Ok(T), Err(E) }
//   enum Message { Quit, Move { x: I32, y: I32 }, Write(String) }

// Parse enum variant name: any identifier (naming convention checked at semantic analysis)
x3::rule<struct enum_variant_name_tag, std::string> const k_enum_variant_name = "enum variant name";
auto const k_enum_variant_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_enum_variant_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_enum_variant_name), Iterator_Type, Context_Type)

// Parse enum variant: unit, tuple, or struct variant
// Unit variant:   Red, None
// Tuple variant:  Some(T), Rgb(I32, I32, I32)
// Struct variant: Move { x: I32, y: I32 }, Empty { }
struct Enum_Variant_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Enum_Variant_Tag, ast::Enum_Variant> const k_enum_variant_rule = "enum variant";
auto const k_enum_variant_rule_def =
    // Struct variant: Name { fields } - requires at least one field
    (k_enum_variant_name >> '{' >> k_struct_fields >> '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) =
          ast::make_enum_variant(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
    })] |
    // Tuple variant: Name(types) - requires at least one type
    (k_enum_variant_name >> '(' >> (k_type_name_rule % ',') >> -lit(',') >> ')')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) =
          ast::make_enum_variant(std::move(boost::fusion::at_c<0>(attr)), std::move(boost::fusion::at_c<1>(attr)));
    })] |
    // Unit variant: just name
    k_enum_variant_name[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_enum_variant(std::move(x3::_attr(a_ctx))); })];

BOOST_SPIRIT_DEFINE(k_enum_variant_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_enum_variant_rule), Iterator_Type, Context_Type)

// Parse enum variants: comma-separated list with optional trailing comma
x3::rule<struct enum_variants_tag, std::vector<ast::Enum_Variant>> const k_enum_variants = "enum variants";
auto const k_enum_variants_def = (k_enum_variant_rule % ',') >> -lit(',');
BOOST_SPIRIT_DEFINE(k_enum_variants)
BOOST_SPIRIT_INSTANTIATE(decltype(k_enum_variants), Iterator_Type, Context_Type)

// Parse enum name: any identifier (naming convention checked at semantic analysis)
x3::rule<struct enum_name_tag, std::string> const k_enum_name = "enum name";
auto const k_enum_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_enum_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_enum_name), Iterator_Type, Context_Type)

// Parse enum definition: "enum Name<T> { variants }"
// Examples:
//   enum Option<T> { Some(T), None }
//   enum Color { Red, Green, Blue }
//   enum Result<T, E> { Ok(T), Err(E) }
//   enum Empty { }  // Empty enums allowed (semantic error, not parse error)
struct Enum_Definition_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Enum_Definition_Tag, ast::Enum_Definition> const k_enum_definition_rule = "enum definition";
auto const k_enum_definition_rule_def =
    (k_kw_enum > k_enum_name > -k_type_params > '{' > -k_enum_variants > '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_enum_definition(
          std::move(boost::fusion::at_c<0>(attr)),
          boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{}),
          boost::fusion::at_c<2>(attr).value_or(std::vector<ast::Enum_Variant>{})
      );
    })];
BOOST_SPIRIT_DEFINE(k_enum_definition_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_enum_definition_rule), Iterator_Type, Context_Type)

// === Impl Block Rules ===
// Impl blocks group method implementations for a type
// Examples:
//   impl Point { fn distance(self): F64 { ... } }
//   impl<T> Array<T> { fn len(self): I32 { ... } }

// Parse impl block methods: zero or more function definitions
x3::rule<struct impl_methods_tag, std::vector<ast::Function_Definition>> const k_impl_methods = "impl methods";
auto const k_impl_methods_def = *k_function_definition_rule;
BOOST_SPIRIT_DEFINE(k_impl_methods)
BOOST_SPIRIT_INSTANTIATE(decltype(k_impl_methods), Iterator_Type, Context_Type)

// Parse impl block: "impl [<T>] Type { methods }"
// Examples:
//   impl Point { fn distance(self): F64 { ... } }
//   impl<T> Array<T> { fn len(self): I32 { ... } fn get(self, idx: I32): Option<T> { ... } }
//   impl<K, V> Map<K, V> { fn insert(self, key: K, value: V): Bool { ... } }
struct Impl_Block_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Impl_Block_Tag, ast::Impl_Block> const k_impl_block_rule = "impl block";
auto const k_impl_block_rule_def =
    (k_kw_impl > -k_type_params > k_type_name_rule > '{' > k_impl_methods > '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_impl_block(
          std::move(boost::fusion::at_c<1>(attr)),
          boost::fusion::at_c<0>(attr).value_or(std::vector<ast::Type_Name>{}),
          std::move(boost::fusion::at_c<2>(attr))
      );
    })];
BOOST_SPIRIT_DEFINE(k_impl_block_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_impl_block_rule), Iterator_Type, Context_Type)

// Parse statement: variant of different statement types
// Order matters: try function definition first (longest match), then let, then others
// expression_statement must come last as it matches most broadly
auto const k_statement_rule_def = k_function_definition_rule | k_struct_definition_rule | k_enum_definition_rule |
                                  k_impl_block_rule | k_let_statement_rule | k_function_call_statement_rule |
                                  k_if_statement_rule | k_while_statement_rule | k_for_statement_rule | k_block_rule |
                                  k_return_statement_rule | k_break_statement_rule | k_continue_statement_rule |
                                  k_expression_statement_rule;
BOOST_SPIRIT_DEFINE(k_statement_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_statement_rule), Iterator_Type, Context_Type)

// Parse module: zero or more top-level statements
// A module represents a complete compilation unit (file)
// Top-level statements are currently only function definitions, but will include:
// - Import statements
// - Type definitions (struct, enum, trait, etc.)
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

  // Get [start, end) Source_Position from iterator positions
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
    Rule const& a_rule,
    parser::Iterator_Type& a_begin,
    parser::Iterator_Type a_end,
    std::string_view a_source
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
  bool const success = phrase_parse(a_begin, a_end, parser, parser::k_skipper, ast);

  // Check if there were any errors logged during parsing, even if parse "succeeded"
  // This handles cases like `*rule` matching zero items after an expectation failure
  std::string const spirit_error = error_stream.str();
  bool const has_logged_errors = !spirit_error.empty();

  if (success && !has_logged_errors) {
    return ast;
  }

  // Build diagnostic with clang-style formatting
  auto const range = tracker.iterator_to_range(a_begin, a_begin == a_end ? a_begin : a_begin + 1);

  // Build detailed error message using the rule's user-friendly description
  std::string error_msg = fmt::format("Failed to parse {}", a_rule.name);

  // Extract Spirit X3's error message text (without its formatting) if available
  if (has_logged_errors) {
    // Find the error message line (starts with k_spirit_error_marker)
    std::size_t const error_pos = spirit_error.find(parser::k_spirit_error_marker);
    if (error_pos != std::string::npos) {
      // Skip past the marker to get just the error message
      std::size_t const text_start = error_pos + parser::k_spirit_error_marker.length();
      std::size_t const newline_pos = spirit_error.find('\n', text_start);
      if (newline_pos != std::string::npos) {
        std::string error_text = spirit_error.substr(text_start, newline_pos - text_start);
        // Trim leading whitespace from the extracted error text
        auto const first_non_space = error_text.find_first_not_of(' ');
        if (first_non_space != std::string::npos) {
          error_text = error_text.substr(first_non_space);
        }
        error_msg += fmt::format(": {}", error_text);
      }
    }
  }

  diagnostics.add_error(range, std::move(error_msg));
  return tl::unexpected(std::move(diagnostics));
}
}  // namespace

// Parse function implementations - generate diagnostics for errors
#define PARSE_FN_IMPL(ast_type, fn_name)                                                                             \
  parser::Parse_Result<ast::ast_type> parse_##fn_name(parser::Iterator_Type& a_begin, parser::Iterator_Type a_end) { \
    return parse_with_rule<decltype(parser::k_##fn_name##_rule), ast::ast_type>(                                     \
        parser::k_##fn_name##_rule,                                                                                  \
        a_begin,                                                                                                     \
        a_end,                                                                                                       \
        {a_begin, a_end}                                                                                             \
    );                                                                                                               \
  }

// Exposed test API - semantic boundaries only
PARSE_FN_IMPL(Module, module)                            // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Function_Definition, function_definition)  // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Struct_Definition, struct_definition)      // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Enum_Definition, enum_definition)          // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Impl_Block, impl_block)                    // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Statement, statement)                      // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Block, block)                              // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Expr, expr)                                // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Type_Name, type_name)                      // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Integer, integer)                          // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Float, float)                              // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(String, string)                            // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Char, char)                                // NOLINT(misc-use-internal-linkage)
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
    // Copy diagnostics to our engine with correct filename
    for (auto const& diag : result.error().diagnostics()) {
      if (diag.level == Diagnostic_Level::Error) {
        diagnostics.add_error(diag.range, diag.message);
      } else {
        diagnostics.add_warning(diag.range, diag.message);
      }
    }
    return tl::unexpected(std::move(diagnostics));
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