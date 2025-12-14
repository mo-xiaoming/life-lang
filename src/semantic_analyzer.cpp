#include "semantic_analyzer.hpp"
#include <fmt/format.h>
#include <cctype>
#include <ranges>

namespace life_lang {

namespace x3 = boost::spirit::x3;

// ============================================================================
// Declaration Collection Visitor
// ============================================================================

struct Declaration_Collector : boost::static_visitor<void> {
 private:
  Semantic_Analyzer* m_analyzer;

 public:
  explicit Declaration_Collector(Semantic_Analyzer* a_analyzer) : m_analyzer(a_analyzer) {}

  void operator()(x3::forward_ast<ast::Func_Def> const& a_item) const { m_analyzer->collect_func_def(a_item.get()); }
  void operator()(x3::forward_ast<ast::Struct_Def> const& a_item) const {
    m_analyzer->collect_struct_def(a_item.get());
  }
  void operator()(x3::forward_ast<ast::Enum_Def> const& a_item) const { m_analyzer->collect_enum_def(a_item.get()); }
  void operator()(x3::forward_ast<ast::Trait_Def> const& a_item) const { m_analyzer->collect_trait_def(a_item.get()); }
  void operator()(x3::forward_ast<ast::Type_Alias> const& a_item) const {
    m_analyzer->collect_type_alias(a_item.get());
  }
  void operator()(x3::forward_ast<ast::Impl_Block> const& a_item) const {
    m_analyzer->collect_impl_block(a_item.get());
  }
  void operator()(x3::forward_ast<ast::Trait_Impl> const& a_item) const {
    m_analyzer->collect_trait_impl(a_item.get());
  }

  // Ignore other statement types for declaration collection
  void operator()(x3::forward_ast<ast::Let_Statement> const& /*a_let*/) const {}
  void operator()(ast::Func_Call_Statement const& /*a_call*/) const {}
  void operator()(x3::forward_ast<ast::Expr_Statement> const& /*a_expr*/) const {}
  void operator()(ast::Return_Statement const& /*a_return*/) const {}
  void operator()(ast::Break_Statement const& /*a_break*/) const {}
  void operator()(ast::Continue_Statement const& /*a_continue*/) const {}
  void operator()(x3::forward_ast<ast::If_Statement> const& /*a_if*/) const {}
  void operator()(x3::forward_ast<ast::While_Statement> const& /*a_while*/) const {}
  void operator()(x3::forward_ast<ast::For_Statement> const& /*a_for*/) const {}
  void operator()(x3::forward_ast<ast::Block> const& /*a_block*/) const {}
};

// ============================================================================
// Constructor
// ============================================================================

Semantic_Analyzer::Semantic_Analyzer(Diagnostic_Engine& a_diagnostics) : m_diagnostics(a_diagnostics) {}

// ============================================================================
// Main Entry Point
// ============================================================================

bool Semantic_Analyzer::analyze(ast::Module const& a_module) {
  m_has_errors = false;

  // Collect all declarations
  collect_declarations(a_module);

  // Resolve all name references
  if (!m_has_errors) {
    resolve_names(a_module);
  }

  return !m_has_errors;
}

// ============================================================================
// Declaration Collection
// ============================================================================

void Semantic_Analyzer::collect_declarations(ast::Module const& a_module) {
  Declaration_Collector const visitor(this);
  for (auto const& stmt : a_module.statements) {
    boost::apply_visitor(visitor, stmt);
  }
}

void Semantic_Analyzer::collect_func_def(ast::Func_Def const& a_func) {
  auto const loc = get_location(a_func);

  // Validate naming convention
  if (!validate_value_name_convention(a_func.declaration.name, loc)) {
    m_has_errors = true;
    return;
  }

  // Extract generic parameters
  std::vector<std::string> generic_params;
  generic_params.reserve(a_func.declaration.type_params.size());
  for (auto const& param : a_func.declaration.type_params) {
    generic_params.push_back(type_to_string(param.name));
  }

  // Create symbol
  Symbol const symbol{
      .name = a_func.declaration.name,
      .kind = Symbol_Kind::Function,
      .location = loc,
      .type_annotation = type_to_string(a_func.declaration.return_type),
      .generic_params = std::move(generic_params),
  };

  if (!m_symbol_table.insert(symbol)) {
    m_diagnostics.add_error(Source_Range{}, fmt::format("duplicate function definition '{}'", a_func.declaration.name));
    m_has_errors = true;
  }
}

void Semantic_Analyzer::collect_struct_def(ast::Struct_Def const& a_struct) {
  auto const loc = get_location(a_struct);

  // Validate naming convention
  if (!validate_type_name_convention(a_struct.name, loc)) {
    m_has_errors = true;
    return;
  }

  // Extract generic parameters
  std::vector<std::string> generic_params;
  generic_params.reserve(a_struct.type_params.size());
  for (auto const& param : a_struct.type_params) {
    generic_params.push_back(type_to_string(param.name));
  }

  Symbol const symbol{
      .name = a_struct.name,
      .kind = Symbol_Kind::Type,
      .location = loc,
      .type_annotation = "struct",
      .generic_params = std::move(generic_params),
  };

  if (!m_symbol_table.insert(symbol)) {
    m_diagnostics.add_error(Source_Range{}, fmt::format("duplicate type definition '{}'", a_struct.name));
    m_has_errors = true;
  }

  // TODO(mo-xiaoming): Collect fields into a nested scope for field access validation
}

void Semantic_Analyzer::collect_enum_def(ast::Enum_Def const& a_enum) {
  auto const loc = get_location(a_enum);

  // Validate naming convention
  if (!validate_type_name_convention(a_enum.name, loc)) {
    m_has_errors = true;
    return;
  }

  // Extract generic parameters
  std::vector<std::string> generic_params;
  generic_params.reserve(a_enum.type_params.size());
  for (auto const& param : a_enum.type_params) {
    generic_params.push_back(type_to_string(param.name));
  }

  Symbol const symbol{
      .name = a_enum.name,
      .kind = Symbol_Kind::Type,
      .location = loc,
      .type_annotation = "enum",
      .generic_params = std::move(generic_params),
  };

  if (!m_symbol_table.insert(symbol)) {
    m_diagnostics.add_error(Source_Range{}, fmt::format("duplicate type definition '{}'", a_enum.name));
    m_has_errors = true;
  }

  // TODO(mo-xiaoming): Collect variants as symbols
}

void Semantic_Analyzer::collect_trait_def(ast::Trait_Def const& a_trait) {
  auto const loc = get_location(a_trait);

  // Validate naming convention
  if (!validate_type_name_convention(a_trait.name, loc)) {
    m_has_errors = true;
    return;
  }

  // Extract generic parameters
  std::vector<std::string> generic_params;
  generic_params.reserve(a_trait.type_params.size());
  for (auto const& param : a_trait.type_params) {
    generic_params.push_back(type_to_string(param.name));
  }

  Symbol const symbol{
      .name = a_trait.name,
      .kind = Symbol_Kind::Trait,
      .location = loc,
      .type_annotation = "trait",
      .generic_params = std::move(generic_params),
  };

  if (!m_symbol_table.insert(symbol)) {
    m_diagnostics.add_error(Source_Range{}, fmt::format("duplicate trait definition '{}'", a_trait.name));
    m_has_errors = true;
  }

  // TODO(mo-xiaoming): Collect trait methods
}

void Semantic_Analyzer::collect_type_alias(ast::Type_Alias const& a_alias) {
  auto const loc = get_location(a_alias);

  // Validate naming convention
  if (!validate_type_name_convention(a_alias.name, loc)) {
    m_has_errors = true;
    return;
  }

  Symbol const symbol{
      .name = a_alias.name,
      .kind = Symbol_Kind::Type,
      .location = loc,
      .type_annotation = type_to_string(a_alias.aliased_type),
      .generic_params = {},
  };

  if (!m_symbol_table.insert(symbol)) {
    m_diagnostics.add_error(Source_Range{}, fmt::format("duplicate type alias '{}'", a_alias.name));
    m_has_errors = true;
  }
}

void Semantic_Analyzer::collect_impl_block([[maybe_unused]] ast::Impl_Block const& a_impl) {
  // TODO(mo-xiaoming): Collect methods from impl block
  // Need to handle method scoping and 'self' parameter
}

void Semantic_Analyzer::collect_trait_impl([[maybe_unused]] ast::Trait_Impl const& a_impl) {
  // TODO(mo-xiaoming): Collect methods from trait impl
}

// ============================================================================
// Name Resolution
// ============================================================================

void Semantic_Analyzer::resolve_names([[maybe_unused]] ast::Module const& a_module) {
  // TODO(mo-xiaoming): Implement name resolution
  // - Resolve type names in function signatures
  // - Resolve variable references in function bodies
  // - Validate 'self' only in impl blocks
}

void Semantic_Analyzer::resolve_func_body([[maybe_unused]] ast::Func_Def const& a_func) {
  // TODO(mo-xiaoming): Implement function body resolution
}

bool Semantic_Analyzer::resolve_type_name(ast::Type_Name const& a_type) {
  return boost::apply_visitor(
      [](auto const& a_variant) -> bool {
        using T = std::decay_t<decltype(a_variant)>;
        if constexpr (std::is_same_v<T, ast::Path_Type>) {
          return resolve_path_type(a_variant);
        } else if constexpr (std::is_same_v<T, ast::Function_Type>) {
          return resolve_function_type(a_variant);
        }
        return false;
      },
      a_type
  );
}

bool Semantic_Analyzer::resolve_path_type([[maybe_unused]] ast::Path_Type const& a_type) {
  // TODO(mo-xiaoming): Resolve path type
  return true;
}

bool Semantic_Analyzer::resolve_function_type([[maybe_unused]] ast::Function_Type const& a_type) {
  // TODO(mo-xiaoming): Resolve function type
  return true;
}

void Semantic_Analyzer::resolve_expr([[maybe_unused]] ast::Expr const& a_expr) {
  // TODO(mo-xiaoming): Implement expression resolution
}

void Semantic_Analyzer::resolve_var_name([[maybe_unused]] ast::Var_Name const& a_name) {
  // TODO(mo-xiaoming): Implement variable name resolution
}

void Semantic_Analyzer::resolve_func_call([[maybe_unused]] ast::Func_Call_Expr const& a_call) {
  // TODO(mo-xiaoming): Implement function call resolution
}

void Semantic_Analyzer::resolve_binary_expr([[maybe_unused]] ast::Binary_Expr const& a_expr) {
  // TODO(mo-xiaoming): Implement binary expression resolution
}

void Semantic_Analyzer::resolve_if_expr([[maybe_unused]] ast::If_Expr const& a_expr) {
  // TODO(mo-xiaoming): Implement if expression resolution
}

void Semantic_Analyzer::resolve_match_expr([[maybe_unused]] ast::Match_Expr const& a_expr) {
  // TODO(mo-xiaoming): Implement match expression resolution
}

void Semantic_Analyzer::resolve_block([[maybe_unused]] ast::Block const& a_block) {
  // TODO(mo-xiaoming): Implement block resolution
}

void Semantic_Analyzer::resolve_stmt([[maybe_unused]] ast::Statement const& a_stmt) {
  // TODO(mo-xiaoming): Implement statement resolution
}

void Semantic_Analyzer::resolve_let_statement([[maybe_unused]] ast::Let_Statement const& a_let) {
  // TODO(mo-xiaoming): Implement let statement resolution
}

void Semantic_Analyzer::resolve_pattern([[maybe_unused]] ast::Pattern const& a_pattern) {
  // TODO(mo-xiaoming): Implement pattern resolution
}

// ============================================================================
// Validation Helpers
// ============================================================================

bool Semantic_Analyzer::validate_type_name_convention(std::string const& a_name, Source_Location const& a_loc) {
  // Types should use Camel_Snake_Case
  // First char must be uppercase
  if (a_name.empty() || (std::isupper(static_cast<unsigned char>(a_name[0])) == 0)) {
    m_diagnostics.add_error(
        Source_Range{},
        fmt::format(
            "type name '{}' should start with uppercase letter (Camel_Snake_Case) at {}",
            a_name,
            to_string(a_loc)
        )
    );
    return false;
  }
  return true;
}

bool Semantic_Analyzer::validate_value_name_convention(std::string const& a_name, Source_Location const& a_loc) {
  // Values should use snake_case
  // First char must be lowercase or underscore
  if (a_name.empty() || ((std::islower(static_cast<unsigned char>(a_name[0])) == 0) && a_name[0] != '_')) {
    m_diagnostics.add_error(
        Source_Range{},
        fmt::format(
            "value name '{}' should start with lowercase letter or underscore (snake_case) at {}",
            a_name,
            to_string(a_loc)
        )
    );
    return false;
  }
  return true;
}

bool Semantic_Analyzer::validate_self_usage(Source_Location const& a_loc) {
  if (!m_symbol_table.in_impl_scope()) {
    m_diagnostics.add_error(
        Source_Range{},
        fmt::format("'self' can only be used in impl block methods at {}", to_string(a_loc))
    );
    return false;
  }
  return true;
}

std::string Semantic_Analyzer::type_to_string(ast::Type_Name const& a_type) {
  // Simple string representation for now - will be replaced with proper type system
  return boost::apply_visitor(
      [](auto const& a_variant) -> std::string {
        using T = std::decay_t<decltype(a_variant)>;
        if constexpr (std::is_same_v<T, ast::Path_Type>) {
          if (a_variant.segments.empty()) {
            return "<unknown>";
          }
          return a_variant.segments[0].value;  // Simplified
        } else if constexpr (std::is_same_v<T, ast::Function_Type>) {
          return "fn(...)";  // Simplified
        }
        return "<unknown>";
      },
      a_type
  );
}

}  // namespace life_lang
