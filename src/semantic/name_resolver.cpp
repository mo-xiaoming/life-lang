// Name resolution implementation

#include "name_resolver.hpp"

#include <format>

namespace life_lang::semantic {

Expected<Symbol, Diagnostic_Engine> Name_Resolver::resolve_var_name(ast::Var_Name const& name_) {
  // For now, only handle simple single-segment names
  // Multi-segment names (qualified paths) will be added with module system
  if (name_.segments.empty()) {
    m_diag->add_error({}, "Empty variable name");
    return Unexpected(*m_diag);
  }

  if (name_.segments.size() > 1) {
    m_diag->add_error({}, std::format("Qualified names not yet supported: {}", name_.segments[0].value));
    return Unexpected(*m_diag);
  }

  // Lookup in current scope and parent chain
  auto const& segment = name_.segments[0];
  auto result = m_symtab->lookup(segment.value);

  if (!result.has_value()) {
    m_diag->add_error({}, std::format("Undefined variable or function: {}", segment.value));
    return Unexpected(*m_diag);
  }

  return result.value();
}

Expected<Type, Diagnostic_Engine> Name_Resolver::resolve_type_name(ast::Type_Name const& name_) {
  // Type_Name is a variant - need to handle different cases
  return std::visit(
      [this](auto const& type_variant_) -> Expected<Type, Diagnostic_Engine> {
        using T = std::decay_t<decltype(type_variant_)>;

        // Simple type path (single identifier like "I32" or qualified like "Std.String")
        if constexpr (std::is_same_v<T, ast::Path_Type>) {
          if (type_variant_.segments.empty()) {
            m_diag->add_error({}, "Empty type name");
            return Unexpected(*m_diag);
          }

          // For simple single-segment types, lookup as type symbol
          if (type_variant_.segments.size() == 1) {
            auto const& segment = type_variant_.segments[0];
            auto result = m_symtab->lookup(segment.value);

            if (!result.has_value()) {
              m_diag->add_error({}, std::format("Undefined type: {}", segment.value));
              return Unexpected(*m_diag);
            }

            // Return the type from the symbol
            return result.value().type;
          }

          // Multi-segment paths (qualified names) - not yet implemented
          m_diag->add_error(
              {},
              std::format("Qualified type names not yet supported: {}", type_variant_.segments[0].value)
          );
          return Unexpected(*m_diag);
        }

        // For other type forms (arrays, tuples, functions, etc.), just return Error_Type for now
        // Full type resolution will be implemented in type checking phase
        else {
          return Type{Error_Type{}};
        }
      },
      static_cast<ast::Type_Name::Base_Type const&>(name_)
  );
}

}  // namespace life_lang::semantic
