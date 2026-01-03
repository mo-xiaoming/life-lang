// Implementation of type system

#include "type.hpp"

#include <algorithm>
#include <sstream>
#include "utils.hpp"

namespace life_lang::semantic {

// ============================================================================
// Primitive_Type Implementation
// ============================================================================

std::string Primitive_Type::to_string() const {
  switch (kind) {
    case Kind::I8:
      return "I8";
    case Kind::I16:
      return "I16";
    case Kind::I32:
      return "I32";
    case Kind::I64:
      return "I64";
    case Kind::U8:
      return "U8";
    case Kind::U16:
      return "U16";
    case Kind::U32:
      return "U32";
    case Kind::U64:
      return "U64";
    case Kind::F32:
      return "F32";
    case Kind::F64:
      return "F64";
    case Kind::Bool:
      return "Bool";
    case Kind::Char:
      return "Char";
    case Kind::String:
      return "String";
  }
  unreachable();
}

// ============================================================================
// Type Equality Implementation
// ============================================================================

bool Struct_Type::operator==(Struct_Type const& other_) const {
  if (name != other_.name || type_params.size() != other_.type_params.size()) {
    return false;
  }
  return std::equal(type_params.begin(), type_params.end(), other_.type_params.begin());
}

bool Enum_Type::operator==(Enum_Type const& other_) const {
  if (name != other_.name || type_params.size() != other_.type_params.size()) {
    return false;
  }
  return std::equal(type_params.begin(), type_params.end(), other_.type_params.begin());
}

bool Function_Type::operator==(Function_Type const& other_) const {
  if (param_types.size() != other_.param_types.size()) {
    return false;
  }
  if (*return_type != *other_.return_type) {
    return false;
  }
  return std::equal(param_types.begin(), param_types.end(), other_.param_types.begin());
}

bool Array_Type::operator==(Array_Type const& other_) const {
  if (size != other_.size) {
    return false;
  }
  return *element_type == *other_.element_type;
}

bool Tuple_Type::operator==(Tuple_Type const& other_) const {
  if (element_types.size() != other_.element_types.size()) {
    return false;
  }
  return std::equal(element_types.begin(), element_types.end(), other_.element_types.begin());
}

bool Generic_Type::operator==(Generic_Type const& other_) const {
  return name == other_.name;
}

// ============================================================================
// Type Utilities
// ============================================================================

bool Type::is_primitive() const {
  return std::holds_alternative<Primitive_Type>(*this);
}

bool Type::is_unit() const {
  return std::holds_alternative<Unit_Type>(*this);
}

bool Type::is_struct() const {
  return std::holds_alternative<Struct_Type>(*this);
}

bool Type::is_enum() const {
  return std::holds_alternative<Enum_Type>(*this);
}

bool Type::is_function() const {
  return std::holds_alternative<Function_Type>(*this);
}

bool Type::is_array() const {
  return std::holds_alternative<Array_Type>(*this);
}

bool Type::is_tuple() const {
  return std::holds_alternative<Tuple_Type>(*this);
}

bool Type::is_generic() const {
  return std::holds_alternative<Generic_Type>(*this);
}

bool Type::is_error() const {
  return std::holds_alternative<Error_Type>(*this);
}

bool Type::is_numeric() const {
  auto const* prim = std::get_if<Primitive_Type>(this);
  if (prim == nullptr) {
    return false;
  }
  using Kind = Primitive_Type::Kind;
  return prim->kind == Kind::I8 || prim->kind == Kind::I16 || prim->kind == Kind::I32 || prim->kind == Kind::I64 ||
         prim->kind == Kind::U8 || prim->kind == Kind::U16 || prim->kind == Kind::U32 || prim->kind == Kind::U64 ||
         prim->kind == Kind::F32 || prim->kind == Kind::F64;
}

bool Type::is_integral() const {
  auto const* prim = std::get_if<Primitive_Type>(this);
  if (prim == nullptr) {
    return false;
  }
  using Kind = Primitive_Type::Kind;
  return prim->kind == Kind::I8 || prim->kind == Kind::I16 || prim->kind == Kind::I32 || prim->kind == Kind::I64 ||
         prim->kind == Kind::U8 || prim->kind == Kind::U16 || prim->kind == Kind::U32 || prim->kind == Kind::U64;
}

bool Type::is_floating() const {
  auto const* prim = std::get_if<Primitive_Type>(this);
  if (prim == nullptr) {
    return false;
  }
  using Kind = Primitive_Type::Kind;
  return prim->kind == Kind::F32 || prim->kind == Kind::F64;
}

bool Type::is_signed_int() const {
  auto const* prim = std::get_if<Primitive_Type>(this);
  if (prim == nullptr) {
    return false;
  }
  using Kind = Primitive_Type::Kind;
  return prim->kind == Kind::I8 || prim->kind == Kind::I16 || prim->kind == Kind::I32 || prim->kind == Kind::I64;
}

bool Type::is_unsigned_int() const {
  auto const* prim = std::get_if<Primitive_Type>(this);
  if (prim == nullptr) {
    return false;
  }
  using Kind = Primitive_Type::Kind;
  return prim->kind == Kind::U8 || prim->kind == Kind::U16 || prim->kind == Kind::U32 || prim->kind == Kind::U64;
}

std::string Type::to_string() const {
  return std::visit(
      [](auto const& t_) -> std::string {
        using T = std::decay_t<decltype(t_)>;

        if constexpr (std::same_as<T, Primitive_Type>) {
          return t_.to_string();
        } else if constexpr (std::same_as<T, Unit_Type>) {
          return "()";
        } else if constexpr (std::same_as<T, Struct_Type>) {
          // Print struct with generic params if present: Point<T, U>
          std::ostringstream oss;
          oss << t_.name;
          if (!t_.type_params.empty()) {
            oss << "<";
            for (size_t i = 0; i < t_.type_params.size(); ++i) {
              if (i > 0) {
                oss << ", ";
              }
              oss << t_.type_params[i];
            }
            oss << ">";
          }
          return oss.str();
        } else if constexpr (std::same_as<T, Enum_Type>) {
          // Print enum with generic params if present: Option<T>
          std::ostringstream oss;
          oss << t_.name;
          if (!t_.type_params.empty()) {
            oss << "<";
            for (size_t i = 0; i < t_.type_params.size(); ++i) {
              if (i > 0) {
                oss << ", ";
              }
              oss << t_.type_params[i];
            }
            oss << ">";
          }
          return oss.str();
        } else if constexpr (std::same_as<T, Function_Type>) {
          std::ostringstream oss;
          oss << "fn(";
          for (size_t i = 0; i < t_.param_types.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << t_.param_types[i].to_string();
          }
          oss << "): " << t_.return_type->to_string();
          return oss.str();
        } else if constexpr (std::same_as<T, Array_Type>) {
          std::ostringstream oss;
          oss << "[" << t_.element_type->to_string();
          if (t_.size) {
            oss << "; " << *t_.size;
          }
          oss << "]";
          return oss.str();
        } else if constexpr (std::same_as<T, Tuple_Type>) {
          std::ostringstream oss;
          oss << "(";
          for (size_t i = 0; i < t_.element_types.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << t_.element_types[i].to_string();
          }
          oss << ")";
          return oss.str();
        } else if constexpr (std::same_as<T, Generic_Type>) {
          return t_.name;
        } else if constexpr (std::same_as<T, Error_Type>) {
          return "<error>";
        }
      },
      *this
  );
}

bool Type::operator==(Type const& other_) const {
  if (index() != other_.index()) {
    return false;
  }

  return std::visit(
      [&other_](auto const& t_) -> bool {
        using T = std::decay_t<decltype(t_)>;
        auto const* other_t = std::get_if<T>(&other_);
        return other_t && t_ == *other_t;
      },
      *this
  );
}

// ============================================================================
// Builtin_Types Implementation
// ============================================================================

std::optional<Type> Builtin_Types::lookup(std::string_view name_) {
  if (name_ == "I8") {
    return k_i8;
  }
  if (name_ == "I16") {
    return k_i16;
  }
  if (name_ == "I32") {
    return k_i32;
  }
  if (name_ == "I64") {
    return k_i64;
  }
  if (name_ == "U8") {
    return k_u8;
  }
  if (name_ == "U16") {
    return k_u16;
  }
  if (name_ == "U32") {
    return k_u32;
  }
  if (name_ == "U64") {
    return k_u64;
  }
  if (name_ == "F32") {
    return k_f32;
  }
  if (name_ == "F64") {
    return k_f64;
  }
  if (name_ == "Bool") {
    return k_bool_type;
  }
  if (name_ == "Char") {
    return k_char_type;
  }
  if (name_ == "String") {
    return k_string;
  }
  return std::nullopt;
}

}  // namespace life_lang::semantic
