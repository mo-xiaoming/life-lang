#include "rules.hpp"

#include <fmt/core.h>

#include <boost/fusion/include/at_c.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

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
  x3::error_handler_result on_error(
      Iterator& /*a_first*/, Iterator const& /*a_last*/, Exception const& a_ex, Context const& a_context
  ) {
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

struct Keyword_Symbols : x3::symbols<> {
  Keyword_Symbols() { add("fn")("let")("return")("struct")("self")("mut"); }
} const k_keywords;

// Individual keyword parsers for specific grammar rules (improves error messages)
auto const k_kw_fn = lexeme[lit("fn") >> !(alnum | '_')];
[[maybe_unused]] auto const k_kw_let = lexeme[lit("let") >> !(alnum | '_')];
auto const k_kw_return = lexeme[lit("return") >> !(alnum | '_')];
auto const k_kw_struct = lexeme[lit("struct") >> !(alnum | '_')];
auto const k_kw_self = lexeme[lit("self") >> !(alnum | '_')];
auto const k_kw_mut = lexeme[lit("mut") >> !(alnum | '_')];

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

// Parse template parameters: angle-bracketed comma-separated qualified names
// Each parameter can itself be a full qualified name with templates
// Examples:
//   "<Int>"                                      - simple type
//   "<Key, Value>"                               - multiple simple types
//   "<Array<Int>>"                               - nested template
//   "<Data.Model.User, Config.Settings>"         - qualified names as params
//   "<Std.String, IO.Error>"                     - multiple qualified params
//   "<Parser<Token.Type>, Result<AST.Node, E>>"  - complex nested with qualified names
x3::rule<struct template_params_tag, std::vector<ast::Type_Name>> const k_template_params = "template parameters";
auto const k_template_params_def = (lit('<') > (k_type_name_rule % ',')) > lit('>');
BOOST_SPIRIT_DEFINE(k_template_params)
BOOST_SPIRIT_INSTANTIATE(decltype(k_template_params), Iterator_Type, Context_Type)

// Parse qualified name segment: name with optional template parameters
// A segment can have template parameters that are full qualified names (including multi-segment)
// Examples:
//   "Array"                                  - simple name
//   "Array<Int>"                             - simple template
//   "Map<String, Int>"                       - multi-param template
//   "Table<Data.Model.User>"                 - template with qualified name
//   "Result<IO.Error, Data.Value>"           - template with multiple qualified names
//   "Container<Int>"                         - templated segment (can be followed by more segments)
auto const k_type_name_segment_rule_def = (k_segment_name >> -k_template_params)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_type_name_segment(
      std::move(boost::fusion::at_c<0>(attr)), boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{})
  );
})];
BOOST_SPIRIT_DEFINE(k_type_name_segment_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_type_name_segment_rule), Iterator_Type, Context_Type)

// Parse full qualified name: dot-separated qualified name segments
// Any segment can have template parameters, not just the last one!
// This allows names like "Container<T>.Iterator<Forward>" where intermediate segments are templated.
// Examples:
//   "Int"                                              - simple qualified name
//   "Std.String"                                       - multi-segment qualified name
//   "Std.Collections.Array<T>"                         - qualified with template on last segment
//   "Std.Collections.Map<Key.Type, Value>"             - qualified segment with qualified template param
//   "Container<Int>.Iterator<Forward>"                 - multiple segments with templates
//   "Db.Table<User>.Column<Name>.Validator"            - templates in middle segments
//   "Parser<Token>.Result<AST>.Error<String>"          - multiple templated segments in chain
//   "Network.Protocol<Http.Request, Http.Response>"    - deeply nested qualified templates
//   "IO.Result<Data.Error, Parser.AST>"                - multiple qualified params
auto const k_type_name_rule_def =
    (k_type_name_segment_rule %
     '.')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_type_name(std::move(x3::_attr(a_ctx))); })];
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

// Parse variable name segment with optional template parameters
// Examples: "foo", "map<Int, String>", "Vec<T>"
auto const k_variable_name_segment_rule_def = (k_segment_name >> -k_template_params)[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_variable_name_segment(
      std::move(boost::fusion::at_c<0>(attr)), boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Type_Name>{})
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
     '.')[([](auto& a_ctx) { x3::_val(a_ctx) = ast::make_variable_name(std::move(x3::_attr(a_ctx))); })];
BOOST_SPIRIT_DEFINE(k_qualified_variable_name_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_qualified_variable_name_rule), Iterator_Type, Context_Type)

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
    (((x3::matches[k_kw_mut] >> k_param_name) > ':') > k_param_type)[([](auto& a_ctx) {
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
auto const k_function_call_expr_rule_def = (((k_call_name >> '(') > -k_call_args) > ')')[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  x3::_val(a_ctx) = ast::make_function_call_expr(
      std::move(boost::fusion::at_c<0>(attr)), boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Expr>{})
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
auto const k_field_initializer_rule_def = ((k_field_init_name > ':') > k_expr_rule)[([](auto& a_ctx) {
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

// Parse struct literal: "TypeName { fields }"
// Type name: any identifier (naming convention checked at semantic analysis)
x3::rule<struct struct_literal_type_tag, std::string> const k_struct_literal_type = "struct type name";
auto const k_struct_literal_type_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_struct_literal_type)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_literal_type), Iterator_Type, Context_Type)

auto const k_struct_literal_rule_def =
    (((k_struct_literal_type >> '{') > -k_field_initializers) > '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_struct_literal(
          std::move(boost::fusion::at_c<0>(attr)),
          boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Field_Initializer>{})
      );
    })];
BOOST_SPIRIT_DEFINE(k_struct_literal_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_literal_rule), Iterator_Type, Context_Type)

// Parse field name in field access: any identifier (naming convention checked at semantic analysis)
// Rule wrapper needed for proper attribute extraction in semantic action
x3::rule<struct field_access_name_tag, std::string> const k_field_access_name = "field name";
auto const k_field_access_name_def = k_segment_name;
BOOST_SPIRIT_DEFINE(k_field_access_name)
BOOST_SPIRIT_INSTANTIATE(decltype(k_field_access_name), Iterator_Type, Context_Type)

// Primary expressions (before field access)
x3::rule<struct primary_expr_tag, ast::Expr> const k_primary_expr = "primary expression";
auto const k_primary_expr_def =
    k_struct_literal_rule | k_function_call_expr_rule | k_string_rule | k_variable_name_rule | k_integer_rule;
BOOST_SPIRIT_DEFINE(k_primary_expr)
BOOST_SPIRIT_INSTANTIATE(decltype(k_primary_expr), Iterator_Type, Context_Type)

// Postfix expression: primary followed by optional field access chain
// Handles both plain primaries (variables, literals) and field access (obj.field.nested)
// Uses * (zero or more) to accept plain expressions, validates fields when dot present
auto const k_postfix_expr_def = (k_primary_expr >> *('.' > k_field_access_name))[([](auto& a_ctx) {
  auto& attr = x3::_attr(a_ctx);
  ast::Expr expr = std::move(boost::fusion::at_c<0>(attr));
  auto const& fields = boost::fusion::at_c<1>(attr);

  // Build left-associative chain: ((expr.f1).f2).f3...
  for (auto const& field : fields) {
    expr = ast::make_expr(ast::make_field_access_expr(std::move(expr), std::string(field)));
  }
  x3::_val(a_ctx) = std::move(expr);
})];
struct Postfix_Expr_Tag : x3::annotate_on_success {};
x3::rule<Postfix_Expr_Tag, ast::Expr> const k_postfix_expr = "postfix_expr";
BOOST_SPIRIT_DEFINE(k_postfix_expr)

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

// Level 3: Comparison (<, >, <=, >=)
auto const k_comparison_expr_def = (k_additive_expr >> *(k_comparison_op >> k_additive_expr))[k_build_binary_expr];
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

// Parse expression: start with lowest precedence (logical OR)
auto const k_expr_rule_def = k_logical_or_expr;
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
struct Struct_Field_Tag : x3::annotate_on_success, Error_Handler {};
struct Struct_Definition_Tag : x3::annotate_on_success, Error_Handler {};
struct Module_Tag : x3::annotate_on_success, Error_Handler {};
x3::rule<Statement_Tag, ast::Statement> const k_statement_rule = "statement";
x3::rule<Block_Tag, ast::Block> const k_block_rule = "code block";
x3::rule<Function_Definition_Tag, ast::Function_Definition> const k_function_definition_rule = "function definition";
x3::rule<Struct_Field_Tag, ast::Struct_Field> const k_struct_field_rule = "struct field";
x3::rule<Struct_Definition_Tag, ast::Struct_Definition> const k_struct_definition_rule = "struct definition";
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
auto const k_struct_field_rule_def = ((k_struct_field_name > ':') > k_type_name_rule)[([](auto& a_ctx) {
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

// Parse struct definition: "struct Name { fields }"
// Examples:
//   struct Point { x: I32, y: I32 }
//   struct Empty { }
//   struct Complex { field1: Type1, field2: Type2, field3: Type3 }
auto const k_struct_definition_rule_def =
    ((((k_kw_struct > k_struct_name) > '{') > -k_struct_fields) > '}')[([](auto& a_ctx) {
      auto& attr = x3::_attr(a_ctx);
      x3::_val(a_ctx) = ast::make_struct_definition(
          std::move(boost::fusion::at_c<0>(attr)),
          boost::fusion::at_c<1>(attr).value_or(std::vector<ast::Struct_Field>{})
      );
    })];
BOOST_SPIRIT_DEFINE(k_struct_definition_rule)
BOOST_SPIRIT_INSTANTIATE(decltype(k_struct_definition_rule), Iterator_Type, Context_Type)

// Parse statement: variant of different statement types
// Order matters: try function definition first (longest match), then others
auto const k_statement_rule_def = k_function_definition_rule | k_struct_definition_rule |
                                  k_function_call_statement_rule | k_block_rule | k_return_statement_rule;
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
        parser::k_##fn_name##_rule, a_begin, a_end, {a_begin, a_end}                                                 \
    );                                                                                                               \
  }

// Exposed test API - semantic boundaries only
PARSE_FN_IMPL(Module, module)                            // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Function_Definition, function_definition)  // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Struct_Definition, struct_definition)      // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Statement, statement)                      // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Block, block)                              // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Expr, expr)                                // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Type_Name, type_name)                      // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(Integer, integer)                          // NOLINT(misc-use-internal-linkage)
PARSE_FN_IMPL(String, string)                            // NOLINT(misc-use-internal-linkage)
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