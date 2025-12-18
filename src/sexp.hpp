#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "ast.hpp"

namespace life_lang::ast {

// ============================================================================
// S-Expression Printer for AST Nodes
// ============================================================================
// Lightweight alternative to JSON serialization for debugging and testing.
// Produces formatted Lisp-style syntax for easy visual inspection.
//
// Format: Indented, multi-line S-expressions with proper nesting.
// Each nested list is indented by 2 spaces.
//
// Example output:
//   (func_def
//     (func_decl "main" () ()
//       (path
//         ((type_segment "I32"))))
//     (block
//       ((return
//         (integer "42")))))
//
// Grammar documentation: See doc/SEXP_GRAMMAR.md

namespace detail {

// Helper to escape strings for S-expression output
inline std::string escape_string(std::string_view str_) {
  std::ostringstream oss;
  oss << '"';
  for (char const ch : str_) {
    switch (ch) {
      case '"':
        oss << "\\\"";
        break;
      case '\\':
        oss << "\\\\";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;
      default:
        oss << ch;
        break;
    }
  }
  oss << '"';
  return oss.str();
}

// Forward declarations for visitor
class Sexp_Printer;
void print_sexp(Sexp_Printer& p_, Type_Name const& type_);
void print_sexp(Sexp_Printer& p_, Expr const& expr_);
void print_sexp(Sexp_Printer& p_, Pattern const& pattern_);
void print_sexp(Sexp_Printer& p_, Statement const& stmt_);

class Sexp_Printer {
public:
  explicit Sexp_Printer(int indent_ = 2) : m_indent_size(indent_) {}

  [[nodiscard]] std::string str() const { return m_oss.str(); }

  void write(std::string_view text_) {
    maybe_indent();
    m_oss << text_;
  }

  void maybe_indent() {
    if (m_needs_indent && m_indent_size > 0) {
      m_oss << '\n';
      for (int i = 0; i < m_depth * m_indent_size; ++i) {
        m_oss << ' ';
      }
      m_needs_indent = false;
    }
  }

  void begin_list(std::string_view tag_) {
    maybe_indent();
    m_oss << '(' << tag_;
    m_depth++;
    m_needs_indent = (m_indent_size > 0);
  }

  void end_list() {
    m_depth--;
    m_oss << ')';
    m_needs_indent = false;
  }

  void space() {
    if (m_indent_size > 0) {
      m_needs_indent = true;
    } else {
      m_oss << ' ';
    }
  }

  void write_quoted(std::string_view str_) {
    maybe_indent();
    m_oss << escape_string(str_);
  }

  void write_bool(bool value_) {
    maybe_indent();
    m_oss << (value_ ? "true" : "false");
  }

  template <typename T>
  void write_optional(std::optional<T> const& opt_, auto&& print_fn_) {
    if (opt_) {
      print_fn_(*opt_);
    } else {
      write("nil");
    }
  }

  template <typename T>
  void write_vector(std::vector<T> const& vec_, auto&& print_fn_) {
    if (vec_.empty()) {
      maybe_indent();
      write("()");
      return;
    }
    maybe_indent();
    m_oss << "(";
    for (std::size_t i = 0; i < vec_.size(); ++i) {
      if (i > 0) {
        space();
      }
      print_fn_(vec_[i]);
    }
    m_oss << ")";
  }

  template <typename T>
  void write_shared_ptr_vector(std::vector<std::shared_ptr<T>> const& vec_, auto&& print_fn_) {
    if (vec_.empty()) {
      maybe_indent();
      write("()");
      return;
    }
    maybe_indent();
    m_oss << "(";
    for (std::size_t i = 0; i < vec_.size(); ++i) {
      if (i > 0) {
        space();
      }
      if (vec_[i]) {
        print_fn_(*vec_[i]);
      } else {
        maybe_indent();
        m_oss << "nil";
      }
    }
    m_oss << ")";
  }

private:
  std::ostringstream m_oss;
  int m_indent_size{2};
  int m_depth{0};
  bool m_needs_indent{false};
};

// Type system S-expression printers
inline void print_sexp(Sexp_Printer& p_, Type_Name_Segment const& seg_) {
  p_.begin_list("type_segment");
  p_.space();
  p_.write_quoted(seg_.value);
  if (!seg_.type_params.empty()) {
    p_.space();
    p_.write_vector(seg_.type_params, [&](auto const& t_) { print_sexp(p_, t_); });
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Path_Type const& path_) {
  p_.begin_list("path");
  if (!path_.segments.empty()) {
    p_.space();
    p_.write_vector(path_.segments, [&](auto const& s_) { print_sexp(p_, s_); });
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Function_Type const& func_) {
  p_.begin_list("func_type");
  p_.space();
  p_.write_shared_ptr_vector(func_.param_types, [&](auto const& t_) { print_sexp(p_, t_); });
  p_.space();
  print_sexp(p_, *func_.return_type);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Array_Type const& arr_) {
  p_.begin_list("array_type");
  p_.space();
  print_sexp(p_, *arr_.element_type);
  p_.space();
  p_.write_quoted(arr_.size);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Tuple_Type const& tuple_) {
  p_.begin_list("tuple_type");
  p_.space();
  p_.write_vector(tuple_.element_types, [&](auto const& t_) { print_sexp(p_, t_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Type_Name const& type_) {
  std::visit([&](auto const& t_) { print_sexp(p_, t_); }, type_);
}

inline void print_sexp(Sexp_Printer& p_, Trait_Bound const& bound_) {
  p_.begin_list("trait_bound");
  p_.space();
  print_sexp(p_, bound_.trait_name);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Type_Param const& param_) {
  p_.begin_list("type_param");
  p_.space();
  print_sexp(p_, param_.name);
  if (!param_.bounds.empty()) {
    p_.space();
    p_.write_vector(param_.bounds, [&](auto const& b_) { print_sexp(p_, b_); });
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Where_Predicate const& pred_) {
  p_.begin_list("where_pred");
  p_.space();
  print_sexp(p_, pred_.type_name);
  p_.space();
  p_.write_vector(pred_.bounds, [&](auto const& b_) { print_sexp(p_, b_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Where_Clause const& where_) {
  p_.begin_list("where");
  if (!where_.predicates.empty()) {
    p_.space();
    p_.write_vector(where_.predicates, [&](auto const& pr_) { print_sexp(p_, pr_); });
  }
  p_.end_list();
}

// Variable name S-expression printers
inline void print_sexp(Sexp_Printer& p_, Var_Name_Segment const& seg_) {
  p_.begin_list("var_segment");
  p_.space();
  p_.write_quoted(seg_.value);
  if (!seg_.type_params.empty()) {
    p_.space();
    p_.write_vector(seg_.type_params, [&](auto const& t_) { print_sexp(p_, t_); });
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Var_Name const& var_) {
  p_.begin_list("var");
  if (!var_.segments.empty()) {
    p_.space();
    p_.write_vector(var_.segments, [&](auto const& s_) { print_sexp(p_, s_); });
  }
  p_.end_list();
}

// Literal S-expression printers
inline void print_sexp(Sexp_Printer& p_, String const& str_) {
  p_.begin_list("string");
  p_.space();
  p_.write_quoted(str_.value);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Integer const& i_) {
  p_.begin_list("integer");
  p_.space();
  p_.write_quoted(i_.value);
  if (i_.suffix) {
    p_.space();
    p_.write_quoted(*i_.suffix);
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Float const& f_) {
  p_.begin_list("float");
  p_.space();
  p_.write_quoted(f_.value);
  if (f_.suffix) {
    p_.space();
    p_.write_quoted(*f_.suffix);
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Char const& ch_) {
  p_.begin_list("char");
  p_.space();
  p_.write_quoted(ch_.value);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Unit_Literal const& /*unused*/) {
  p_.write("unit");
}

inline void print_sexp(Sexp_Printer& p_, Field_Initializer const& field_) {
  p_.begin_list("field_init");
  p_.space();
  p_.write_quoted(field_.name);
  p_.space();
  print_sexp(p_, *field_.value);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Struct_Literal const& lit_) {
  p_.begin_list("struct_lit");
  p_.space();
  p_.write_quoted(lit_.type_name);
  p_.space();
  p_.write_vector(lit_.fields, [&](auto const& f_) { print_sexp(p_, f_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Array_Literal const& lit_) {
  p_.begin_list("array_lit");
  p_.space();
  p_.write_vector(lit_.elements, [&](auto const& e_) { print_sexp(p_, e_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Tuple_Literal const& lit_) {
  p_.begin_list("tuple_lit");
  p_.space();
  p_.write_vector(lit_.elements, [&](auto const& e_) { print_sexp(p_, e_); });
  p_.end_list();
}

// Operator S-expression printers
inline void print_sexp(Sexp_Printer& p_, Binary_Op op_) {
  switch (op_) {
    case Binary_Op::Add:
      p_.write("+");
      break;
    case Binary_Op::Sub:
      p_.write("-");
      break;
    case Binary_Op::Mul:
      p_.write("*");
      break;
    case Binary_Op::Div:
      p_.write("/");
      break;
    case Binary_Op::Mod:
      p_.write("%");
      break;
    case Binary_Op::Eq:
      p_.write("==");
      break;
    case Binary_Op::Ne:
      p_.write("!=");
      break;
    case Binary_Op::Lt:
      p_.write("<");
      break;
    case Binary_Op::Gt:
      p_.write(">");
      break;
    case Binary_Op::Le:
      p_.write("<=");
      break;
    case Binary_Op::Ge:
      p_.write(">=");
      break;
    case Binary_Op::And:
      p_.write("&&");
      break;
    case Binary_Op::Or:
      p_.write("||");
      break;
  }
}

inline void print_sexp(Sexp_Printer& p_, Unary_Op op_) {
  switch (op_) {
    case Unary_Op::Neg:
      p_.write("-");
      break;
    case Unary_Op::Pos:
      p_.write("+");
      break;
    case Unary_Op::Not:
      p_.write("!");
      break;
    case Unary_Op::BitNot:
      p_.write("~");
      break;
  }
}

inline void print_sexp(Sexp_Printer& p_, Binary_Expr const& expr_) {
  p_.begin_list("binary");
  p_.space();
  print_sexp(p_, expr_.op);
  p_.space();
  print_sexp(p_, *expr_.lhs);
  p_.space();
  print_sexp(p_, *expr_.rhs);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Unary_Expr const& expr_) {
  p_.begin_list("unary");
  p_.space();
  print_sexp(p_, expr_.op);
  p_.space();
  print_sexp(p_, *expr_.operand);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Range_Expr const& expr_) {
  p_.begin_list(expr_.inclusive ? "range_inclusive" : "range");
  p_.space();
  print_sexp(p_, *expr_.start);
  p_.space();
  print_sexp(p_, *expr_.end);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Cast_Expr const& expr_) {
  p_.begin_list("cast");
  p_.space();
  print_sexp(p_, *expr_.expr);
  p_.space();
  print_sexp(p_, expr_.target_type);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Func_Call_Expr const& call_);
inline void print_sexp(Sexp_Printer& p_, Field_Access_Expr const& access_);
inline void print_sexp(Sexp_Printer& p_, Index_Expr const& index_);
inline void print_sexp(Sexp_Printer& p_, Assignment_Expr const& assign_);
inline void print_sexp(Sexp_Printer& p_, If_Expr const& if_expr_);
inline void print_sexp(Sexp_Printer& p_, While_Expr const& while_expr_);
inline void print_sexp(Sexp_Printer& p_, For_Expr const& for_expr_);
inline void print_sexp(Sexp_Printer& p_, Match_Expr const& match_expr_);
inline void print_sexp(Sexp_Printer& p_, Block const& block_);

inline void print_sexp(Sexp_Printer& p_, Expr const& expr_) {
  std::visit(
      [&](auto const& e_) {
        using T = std::decay_t<decltype(e_)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Func_Call_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<Field_Access_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<Index_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<Binary_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<Unary_Expr>> || std::is_same_v<T, std::shared_ptr<Cast_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<If_Expr>> || std::is_same_v<T, std::shared_ptr<While_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<For_Expr>> || std::is_same_v<T, std::shared_ptr<Match_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<Block>> || std::is_same_v<T, std::shared_ptr<Range_Expr>> ||
                      std::is_same_v<T, std::shared_ptr<Assignment_Expr>>) {
          if (e_) {
            print_sexp(p_, *e_);
          } else {
            p_.write("nil");
          }
        } else {
          print_sexp(p_, e_);
        }
      },
      expr_
  );
}

inline void print_sexp(Sexp_Printer& p_, Func_Call_Expr const& call_) {
  p_.begin_list("call");
  p_.space();
  print_sexp(p_, call_.name);
  p_.space();
  p_.write_vector(call_.params, [&](auto const& param_) { print_sexp(p_, param_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Field_Access_Expr const& access_) {
  p_.begin_list("field_access");
  p_.space();
  print_sexp(p_, *access_.object);
  p_.space();
  p_.write_quoted(access_.field_name);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Index_Expr const& index_) {
  p_.begin_list("index");
  p_.space();
  print_sexp(p_, *index_.object);
  p_.space();
  print_sexp(p_, *index_.index);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Assignment_Expr const& assign_) {
  p_.begin_list("assign");
  p_.space();
  print_sexp(p_, *assign_.target);
  p_.space();
  print_sexp(p_, *assign_.value);
  p_.end_list();
}

// Pattern S-expression printers
inline void print_sexp(Sexp_Printer& p_, Wildcard_Pattern const& /*unused*/) {
  p_.write("_");
}

inline void print_sexp(Sexp_Printer& p_, Literal_Pattern const& lit_) {
  p_.begin_list("lit_pattern");
  p_.space();
  print_sexp(p_, *lit_.value);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Simple_Pattern const& simple_) {
  p_.begin_list("pattern");
  p_.space();
  p_.write_quoted(simple_.name);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Field_Pattern const& field_);
inline void print_sexp(Sexp_Printer& p_, Struct_Pattern const& struct_pat_);
inline void print_sexp(Sexp_Printer& p_, Tuple_Pattern const& tuple_);
inline void print_sexp(Sexp_Printer& p_, Enum_Pattern const& enum_pat_);

inline void print_sexp(Sexp_Printer& p_, Pattern const& pattern_) {
  std::visit([&](auto const& pat_) { print_sexp(p_, pat_); }, pattern_);
}

inline void print_sexp(Sexp_Printer& p_, Field_Pattern const& field_) {
  p_.begin_list("field_pattern");
  p_.space();
  p_.write_quoted(field_.name);
  p_.space();
  print_sexp(p_, *field_.pattern);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Struct_Pattern const& struct_pat_) {
  p_.begin_list("struct_pattern");
  p_.space();
  print_sexp(p_, struct_pat_.type_name);
  p_.space();
  p_.write_vector(struct_pat_.fields, [&](auto const& f_) { print_sexp(p_, f_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Tuple_Pattern const& tuple_) {
  p_.begin_list("tuple_pattern");
  if (!tuple_.elements.empty()) {
    p_.space();
    p_.write_shared_ptr_vector(tuple_.elements, [&](auto const& e_) { print_sexp(p_, e_); });
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Enum_Pattern const& enum_pat_) {
  p_.begin_list("enum_pattern");
  p_.space();
  print_sexp(p_, enum_pat_.type_name);
  if (!enum_pat_.patterns.empty()) {
    p_.space();
    p_.write_shared_ptr_vector(enum_pat_.patterns, [&](auto const& pat_) { print_sexp(p_, pat_); });
  }
  p_.end_list();
}

// Statement S-expression printers
inline void print_sexp(Sexp_Printer& p_, Let_Statement const& let_) {
  p_.begin_list("let");
  p_.space();
  p_.write_bool(let_.is_mut);
  p_.space();
  print_sexp(p_, let_.pattern);
  p_.space();
  p_.write_optional(let_.type, [&](auto const& t_) { print_sexp(p_, t_); });
  p_.space();
  print_sexp(p_, *let_.value);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Func_Call_Statement const& call_) {
  p_.begin_list("call_stmt");
  p_.space();
  print_sexp(p_, call_.expr);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Expr_Statement const& expr_) {
  p_.begin_list("expr_stmt");
  p_.space();
  print_sexp(p_, *expr_.expr);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Return_Statement const& ret_) {
  p_.begin_list("return");
  p_.space();
  print_sexp(p_, ret_.expr);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Break_Statement const& brk_) {
  p_.begin_list("break");
  if (brk_.value) {
    p_.space();
    print_sexp(p_, *brk_.value);
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Continue_Statement const& /*unused*/) {
  p_.write("continue");
}

inline void print_sexp(Sexp_Printer& p_, If_Statement const& if_stmt_) {
  p_.begin_list("if_stmt");
  p_.space();
  print_sexp(p_, *if_stmt_.expr);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, While_Statement const& while_stmt_) {
  p_.begin_list("while_stmt");
  p_.space();
  print_sexp(p_, *while_stmt_.expr);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, For_Statement const& for_stmt_) {
  p_.begin_list("for_stmt");
  p_.space();
  print_sexp(p_, *for_stmt_.expr);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Else_If_Clause const& clause_) {
  p_.begin_list("else_if");
  p_.space();
  print_sexp(p_, *clause_.condition);
  p_.space();
  print_sexp(p_, *clause_.then_block);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, If_Expr const& if_expr_) {
  p_.begin_list("if");
  p_.space();
  print_sexp(p_, *if_expr_.condition);
  p_.space();
  print_sexp(p_, *if_expr_.then_block);
  if (!if_expr_.else_ifs.empty()) {
    p_.space();
    p_.write_vector(if_expr_.else_ifs, [&](auto const& ei_) { print_sexp(p_, ei_); });
  }
  if (if_expr_.else_block) {
    p_.space();
    print_sexp(p_, **if_expr_.else_block);
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, While_Expr const& while_expr_) {
  p_.begin_list("while");
  p_.space();
  print_sexp(p_, *while_expr_.condition);
  p_.space();
  print_sexp(p_, *while_expr_.body);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, For_Expr const& for_expr_) {
  p_.begin_list("for");
  p_.space();
  print_sexp(p_, for_expr_.pattern);
  p_.space();
  print_sexp(p_, *for_expr_.iterator);
  p_.space();
  print_sexp(p_, *for_expr_.body);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Match_Arm const& arm_) {
  p_.begin_list("arm");
  p_.space();
  print_sexp(p_, arm_.pattern);
  p_.space();
  p_.write_optional(arm_.guard, [&](auto const& g_) { print_sexp(p_, *g_); });
  p_.space();
  print_sexp(p_, *arm_.result);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Match_Expr const& match_expr_) {
  p_.begin_list("match");
  p_.space();
  print_sexp(p_, *match_expr_.scrutinee);
  p_.space();
  p_.write_vector(match_expr_.arms, [&](auto const& a_) { print_sexp(p_, a_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Block const& block_) {
  p_.begin_list("block");
  if (!block_.statements.empty()) {
    p_.space();
    p_.write_vector(block_.statements, [&](auto const& s_) { print_sexp(p_, s_); });
  }
  if (block_.trailing_expr) {
    p_.space();
    print_sexp(p_, **block_.trailing_expr);
  }
  p_.end_list();
}

// Function definition S-expression printers
inline void print_sexp(Sexp_Printer& p_, Func_Param const& param_) {
  p_.begin_list("param");
  p_.space();
  p_.write_bool(param_.is_mut);
  p_.space();
  p_.write_quoted(param_.name);
  p_.space();
  p_.write_optional(param_.type, [&](auto const& t_) { print_sexp(p_, t_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Func_Decl const& decl_) {
  p_.begin_list("func_decl");
  p_.space();
  p_.write_quoted(decl_.name);
  p_.space();
  p_.write_vector(decl_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  p_.write_vector(decl_.func_params, [&](auto const& param_) { print_sexp(p_, param_); });
  p_.space();
  print_sexp(p_, decl_.return_type);
  if (decl_.where_clause) {
    p_.space();
    print_sexp(p_, *decl_.where_clause);
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Func_Def const& def_) {
  p_.begin_list("func_def");
  p_.space();
  p_.write_bool(def_.is_pub);
  p_.space();
  print_sexp(p_, def_.declaration);
  p_.space();
  print_sexp(p_, def_.body);
  p_.end_list();
}

// Struct definition S-expression printers
inline void print_sexp(Sexp_Printer& p_, Struct_Field const& field_) {
  p_.begin_list("field");
  p_.space();
  p_.write_bool(field_.is_pub);
  p_.space();
  p_.write_quoted(field_.name);
  p_.space();
  print_sexp(p_, field_.type);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Struct_Def const& def_) {
  p_.begin_list("struct_def");
  p_.space();
  p_.write_quoted(def_.name);
  p_.space();
  p_.write_vector(def_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  p_.write_vector(def_.fields, [&](auto const& f_) { print_sexp(p_, f_); });
  if (def_.where_clause) {
    p_.space();
    print_sexp(p_, *def_.where_clause);
  }
  p_.end_list();
}

// Enum definition S-expression printers
inline void print_sexp(Sexp_Printer& p_, Unit_Variant const& variant_) {
  p_.begin_list("unit_variant");
  p_.space();
  p_.write_quoted(variant_.name);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Tuple_Variant const& variant_) {
  p_.begin_list("tuple_variant");
  p_.space();
  p_.write_quoted(variant_.name);
  p_.space();
  p_.write_vector(variant_.tuple_fields, [&](auto const& t_) { print_sexp(p_, t_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Struct_Variant const& variant_) {
  p_.begin_list("struct_variant");
  p_.space();
  p_.write_quoted(variant_.name);
  p_.space();
  p_.write_vector(variant_.struct_fields, [&](auto const& f_) { print_sexp(p_, f_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Enum_Variant const& variant_) {
  std::visit([&](auto const& v_) { print_sexp(p_, v_); }, variant_);
}

inline void print_sexp(Sexp_Printer& p_, Enum_Def const& def_) {
  p_.begin_list("enum_def");
  p_.space();
  p_.write_quoted(def_.name);
  p_.space();
  p_.write_vector(def_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  p_.write_vector(def_.variants, [&](auto const& v_) { print_sexp(p_, v_); });
  if (def_.where_clause) {
    p_.space();
    print_sexp(p_, *def_.where_clause);
  }
  p_.end_list();
}

// Impl block S-expression printer
inline void print_sexp(Sexp_Printer& p_, Impl_Block const& impl_) {
  p_.begin_list("impl");
  p_.space();
  print_sexp(p_, impl_.type_name);
  p_.space();
  p_.write_vector(impl_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  p_.write_vector(impl_.methods, [&](auto const& m_) { print_sexp(p_, m_); });
  if (impl_.where_clause) {
    p_.space();
    print_sexp(p_, *impl_.where_clause);
  }
  p_.end_list();
}

// Trait definition S-expression printers
inline void print_sexp(Sexp_Printer& p_, Assoc_Type_Decl const& assoc_) {
  p_.begin_list("assoc_type_decl");
  p_.space();
  p_.write_quoted(assoc_.name);
  p_.space();
  p_.write_vector(assoc_.bounds, [&](auto const& b_) { print_sexp(p_, b_); });
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Assoc_Type_Impl const& assoc_) {
  p_.begin_list("assoc_type_impl");
  p_.space();
  p_.write_quoted(assoc_.name);
  p_.space();
  print_sexp(p_, assoc_.type_value);
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Trait_Def const& def_) {
  p_.begin_list("trait_def");
  p_.space();
  p_.write_quoted(def_.name);
  p_.space();
  p_.write_vector(def_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  p_.write_vector(def_.assoc_types, [&](auto const& at_) { print_sexp(p_, at_); });
  p_.space();
  p_.write_vector(def_.methods, [&](auto const& m_) { print_sexp(p_, m_); });
  if (def_.where_clause) {
    p_.space();
    print_sexp(p_, *def_.where_clause);
  }
  p_.end_list();
}

inline void print_sexp(Sexp_Printer& p_, Trait_Impl const& impl_) {
  p_.begin_list("trait_impl");
  p_.space();
  print_sexp(p_, impl_.trait_name);
  p_.space();
  print_sexp(p_, impl_.type_name);
  p_.space();
  p_.write_vector(impl_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  p_.write_vector(impl_.assoc_type_impls, [&](auto const& ai_) { print_sexp(p_, ai_); });
  p_.space();
  p_.write_vector(impl_.methods, [&](auto const& m_) { print_sexp(p_, m_); });
  if (impl_.where_clause) {
    p_.space();
    print_sexp(p_, *impl_.where_clause);
  }
  p_.end_list();
}

// Type alias S-expression printer
inline void print_sexp(Sexp_Printer& p_, Type_Alias const& alias_) {
  p_.begin_list("type_alias");
  p_.space();
  p_.write_quoted(alias_.name);
  p_.space();
  p_.write_vector(alias_.type_params, [&](auto const& tp_) { print_sexp(p_, tp_); });
  p_.space();
  print_sexp(p_, alias_.aliased_type);
  p_.end_list();
}

// Statement printer (top-level)
inline void print_sexp(Sexp_Printer& p_, Statement const& stmt_) {
  std::visit(
      [&](auto const& s_) {
        using T = std::decay_t<decltype(s_)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Func_Def>> || std::is_same_v<T, std::shared_ptr<Struct_Def>> ||
                      std::is_same_v<T, std::shared_ptr<Enum_Def>> || std::is_same_v<T, std::shared_ptr<Impl_Block>> ||
                      std::is_same_v<T, std::shared_ptr<Trait_Def>> || std::is_same_v<T, std::shared_ptr<Trait_Impl>> ||
                      std::is_same_v<T, std::shared_ptr<Type_Alias>> ||
                      std::is_same_v<T, std::shared_ptr<Let_Statement>> ||
                      std::is_same_v<T, std::shared_ptr<Expr_Statement>> ||
                      std::is_same_v<T, std::shared_ptr<If_Statement>> ||
                      std::is_same_v<T, std::shared_ptr<While_Statement>> ||
                      std::is_same_v<T, std::shared_ptr<For_Statement>> || std::is_same_v<T, std::shared_ptr<Block>>) {
          if (s_) {
            print_sexp(p_, *s_);
          } else {
            p_.write("nil");
          }
        } else {
          print_sexp(p_, s_);
        }
      },
      stmt_
  );
}

// Import_Item S-expression printer
inline void print_sexp(Sexp_Printer& p_, Import_Item const& item_) {
  if (item_.alias.has_value()) {
    p_.begin_list("as");
    p_.space();
    p_.write_quoted(item_.name);
    p_.space();
    p_.write_quoted(*item_.alias);
    p_.end_list();
  } else {
    p_.write_quoted(item_.name);
  }
}

// Import_Statement S-expression printer
inline void print_sexp(Sexp_Printer& p_, Import_Statement const& import_) {
  p_.begin_list("import");
  p_.space();

  // Print module path as list of strings
  p_.begin_list("path");
  for (std::size_t i = 0; i < import_.module_path.size(); ++i) {
    if (i > 0) {
      p_.space();
    }
    p_.write_quoted(import_.module_path[i]);
  }
  p_.end_list();

  p_.space();

  // Print items as list (with optional aliases)
  p_.begin_list("items");
  for (std::size_t i = 0; i < import_.items.size(); ++i) {
    if (i > 0) {
      p_.space();
    }
    print_sexp(p_, import_.items[i]);
  }
  p_.end_list();

  p_.end_list();
}

// Item S-expression printer
inline void print_sexp(Sexp_Printer& p_, Item const& item_) {
  p_.begin_list("item");
  p_.space();
  p_.write_bool(item_.is_pub);
  p_.space();
  print_sexp(p_, item_.item);
  p_.end_list();
}

// Module S-expression printer
inline void print_sexp(Sexp_Printer& p_, Module const& mod_) {
  p_.begin_list("module");

  // Print imports
  if (!mod_.imports.empty()) {
    p_.space();
    p_.begin_list("imports");
    for (std::size_t i = 0; i < mod_.imports.size(); ++i) {
      if (i > 0) {
        p_.space();
      }
      print_sexp(p_, mod_.imports[i]);
    }
    p_.end_list();
  }

  // Print items
  if (!mod_.items.empty()) {
    p_.space();
    p_.begin_list("items");
    for (std::size_t i = 0; i < mod_.items.size(); ++i) {
      if (i > 0) {
        p_.space();
      }
      print_sexp(p_, mod_.items[i]);
    }
    p_.end_list();
  }

  p_.end_list();
}

}  // namespace detail

// Public API for converting AST nodes to S-expression strings
// indent_: number of spaces per indentation level (0 = compact, no newlines)
template <typename T>
std::string to_sexp_string(T const& node_, int indent_ = 2) {
  detail::Sexp_Printer printer(indent_);
  detail::print_sexp(printer, node_);
  return printer.str();
}

}  // namespace life_lang::ast
