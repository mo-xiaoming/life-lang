#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
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

std::string escape_string(std::string_view str_);

class Sexp_Printer {
public:
  explicit Sexp_Printer(int indent_ = 2) : m_indent_size(indent_) {}

  [[nodiscard]] std::string str() const { return m_oss.str(); }

  void write(std::string_view text_);
  void maybe_indent();
  void begin_list(std::string_view tag_);
  void end_list();
  void space();
  void write_quoted(std::string_view str_);
  void write_bool(bool value_);

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

}  // namespace detail

// Public API for converting AST nodes to S-expression strings
// indent_: number of spaces per indentation level (0 = compact, no newlines)
template <typename T>
std::string to_sexp_string(T const& node_, int indent_ = 2);

// Explicit instantiation declarations (defined in sexp.cpp)
extern template std::string to_sexp_string(Module const&, int);
extern template std::string to_sexp_string(Item const&, int);
extern template std::string to_sexp_string(Func_Def const&, int);
extern template std::string to_sexp_string(Struct_Def const&, int);
extern template std::string to_sexp_string(Enum_Def const&, int);
extern template std::string to_sexp_string(Impl_Block const&, int);
extern template std::string to_sexp_string(Trait_Def const&, int);
extern template std::string to_sexp_string(Trait_Impl const&, int);
extern template std::string to_sexp_string(Type_Alias const&, int);
extern template std::string to_sexp_string(Expr const&, int);
extern template std::string to_sexp_string(Statement const&, int);
extern template std::string to_sexp_string(Pattern const&, int);
extern template std::string to_sexp_string(Type_Name const&, int);
extern template std::string to_sexp_string(Import_Statement const&, int);
extern template std::string to_sexp_string(Let_Statement const&, int);
extern template std::string to_sexp_string(Array_Type const&, int);
extern template std::string to_sexp_string(Block const&, int);
extern template std::string to_sexp_string(Bool_Literal const&, int);
extern template std::string to_sexp_string(Char const&, int);
extern template std::string to_sexp_string(Integer const&, int);
extern template std::string to_sexp_string(Float const&, int);
extern template std::string to_sexp_string(Function_Type const&, int);
extern template std::string to_sexp_string(String const&, int);

}  // namespace life_lang::ast
