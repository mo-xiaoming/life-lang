// Type representation for semantic analysis
// Represents all types in the life-lang type system

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace life_lang::semantic {

struct Type;

// ============================================================================
// Type Kinds
// ============================================================================

struct Primitive_Type {
  static constexpr std::string_view k_name = "Primitive_Type";

  enum class Kind : std::uint8_t {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    Bool,
    Char,
    String,
  };

  Kind kind;

  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] bool operator==(Primitive_Type const& other_) const = default;
};

// Unit type: ()
struct Unit_Type {
  static constexpr std::string_view k_name = "Unit_Type";
  [[nodiscard]] auto operator==(Unit_Type const& /*other_*/) const -> bool = default;
};

// User-defined struct type
struct Struct_Type {
  static constexpr std::string_view k_name = "Struct_Type";

  std::string name;                                  // Struct name
  std::vector<std::string> type_params;              // Generic parameters: T, U, etc.
  std::vector<std::pair<std::string, Type>> fields;  // Field name -> type

  [[nodiscard]] bool operator==(Struct_Type const& other_) const;
};

// User-defined enum type
struct Enum_Type {
  static constexpr std::string_view k_name = "Enum_Type";

  std::string name;                      // Enum name
  std::vector<std::string> type_params;  // Generic parameters
  std::vector<std::string> variants;     // Variant names

  [[nodiscard]] bool operator==(Enum_Type const& other_) const;
};

// Function type: fn(T, U): R
struct Function_Type {
  static constexpr std::string_view k_name = "Function_Type";

  std::vector<Type> param_types;
  std::shared_ptr<Type> return_type;

  [[nodiscard]] bool operator==(Function_Type const& other_) const;
};

// Array type: [T; N]
// TODO(mx): Parser needs to support unsized arrays (no size expression)
struct Array_Type {
  static constexpr std::string_view k_name = "Array_Type";

  std::shared_ptr<Type> element_type;
  std::optional<size_t> size;  // None for unsized arrays (future parser support)

  [[nodiscard]] bool operator==(Array_Type const& other_) const;
};

// Tuple type: (T, U, V)
struct Tuple_Type {
  static constexpr std::string_view k_name = "Tuple_Type";

  std::vector<Type> element_types;

  [[nodiscard]] bool operator==(Tuple_Type const& other_) const;
};

// Generic type parameter: T, U, Key, Value
// Used during generic function/struct type checking to represent unresolved type parameters.
// Example: In `fn foo<T>(x: T)`, the parameter `x` has type `Generic_Type{"T"}`
struct Generic_Type {
  static constexpr std::string_view k_name = "Generic_Type";

  std::string name;  // Type parameter name (e.g., "T", "U", "Key")
  // Future: trait bounds (e.g., T: Display + Clone)

  [[nodiscard]] bool operator==(Generic_Type const& other_) const;
};

// Error type - sentinel value for error recovery during type checking
// When type checking fails, we assign Error_Type to allow analysis to continue
// and report multiple errors instead of aborting on the first error.
// Example: `let x: UnknownType = 5;` -> x gets Error_Type, analysis continues
struct Error_Type {
  static constexpr std::string_view k_name = "Error_Type";
  [[nodiscard]] bool operator==(Error_Type const& /*other_*/) const = default;
};

// ============================================================================
// Main Type Variant
// ============================================================================

struct Type : std::variant<
                  Primitive_Type,
                  Unit_Type,
                  Struct_Type,
                  Enum_Type,
                  Function_Type,
                  Array_Type,
                  Tuple_Type,
                  Generic_Type,
                  Error_Type> {
  using Base_Type = std::variant<
      Primitive_Type,
      Unit_Type,
      Struct_Type,
      Enum_Type,
      Function_Type,
      Array_Type,
      Tuple_Type,
      Generic_Type,
      Error_Type>;
  using Base_Type::Base_Type;  // NOLINT(modernize-use-equals-default,hicpp-use-equals-default)
  using Base_Type::operator=;

  // Type checking utilities - cover all variant types
  [[nodiscard]] bool is_primitive() const;
  [[nodiscard]] bool is_unit() const;
  [[nodiscard]] bool is_struct() const;
  [[nodiscard]] bool is_enum() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_array() const;
  [[nodiscard]] bool is_tuple() const;
  [[nodiscard]] bool is_generic() const;
  [[nodiscard]] bool is_error() const;

  // Specialized predicates for primitives
  [[nodiscard]] bool is_numeric() const;
  [[nodiscard]] bool is_integral() const;
  [[nodiscard]] bool is_floating() const;
  [[nodiscard]] bool is_signed_int() const;
  [[nodiscard]] bool is_unsigned_int() const;

  // String representation for diagnostics
  [[nodiscard]] std::string to_string() const;

  // Type equality
  [[nodiscard]] bool operator==(Type const& other_) const;
};

// ============================================================================
// Builtin Types Registry
// ============================================================================

// Registry of all builtin/primitive types
// Singleton struct with inline static constexpr members for builtin types
struct Builtin_Types {
  inline static Type const k_i8 = Type{Primitive_Type{Primitive_Type::Kind::I8}};
  inline static Type const k_i16 = Type{Primitive_Type{Primitive_Type::Kind::I16}};
  inline static Type const k_i32 = Type{Primitive_Type{Primitive_Type::Kind::I32}};
  inline static Type const k_i64 = Type{Primitive_Type{Primitive_Type::Kind::I64}};
  inline static Type const k_u8 = Type{Primitive_Type{Primitive_Type::Kind::U8}};
  inline static Type const k_u16 = Type{Primitive_Type{Primitive_Type::Kind::U16}};
  inline static Type const k_u32 = Type{Primitive_Type{Primitive_Type::Kind::U32}};
  inline static Type const k_u64 = Type{Primitive_Type{Primitive_Type::Kind::U64}};
  inline static Type const k_f32 = Type{Primitive_Type{Primitive_Type::Kind::F32}};
  inline static Type const k_f64 = Type{Primitive_Type{Primitive_Type::Kind::F64}};
  inline static Type const k_bool_type = Type{Primitive_Type{Primitive_Type::Kind::Bool}};
  inline static Type const k_char_type = Type{Primitive_Type{Primitive_Type::Kind::Char}};
  inline static Type const k_string = Type{Primitive_Type{Primitive_Type::Kind::String}};
  inline static Type const k_unit = Type{Unit_Type{}};
  inline static Type const k_error = Type{Error_Type{}};  // For error recovery

  // Lookup builtin type by name
  [[nodiscard]] static std::optional<Type> lookup(std::string_view name_);
};

// ============================================================================
// Helper Functions
// ============================================================================

// Create type instances
inline auto make_primitive_type(Primitive_Type::Kind kind_) -> Type {
  return Type{Primitive_Type{kind_}};
}

inline auto make_unit_type() -> Type {
  return Type{Unit_Type{}};
}

inline auto make_error_type() -> Type {
  return Type{Error_Type{}};
}

inline auto make_struct_type(
    std::string name_,
    std::vector<std::string> type_params_ = {},
    std::vector<std::pair<std::string, Type>> fields_ = {}
) -> Type {
  return Type{
      Struct_Type{.name = std::move(name_), .type_params = std::move(type_params_), .fields = std::move(fields_)}
  };
}

inline auto
make_enum_type(std::string name_, std::vector<std::string> type_params_ = {}, std::vector<std::string> variants_ = {})
    -> Type {
  return Type{
      Enum_Type{.name = std::move(name_), .type_params = std::move(type_params_), .variants = std::move(variants_)}
  };
}

[[nodiscard]] inline Type make_function_type(std::vector<Type> param_types_, Type return_type_) {
  return Type{Function_Type{
      .param_types = std::move(param_types_),
      .return_type = std::make_shared<Type>(std::move(return_type_))
  }};
}

[[nodiscard]] inline Type make_array_type(Type element_type_, std::optional<size_t> size_ = std::nullopt) {
  return Type{Array_Type{.element_type = std::make_shared<Type>(std::move(element_type_)), .size = size_}};
}

[[nodiscard]] inline Type make_tuple_type(std::vector<Type> element_types_) {
  return Type{Tuple_Type{.element_types = std::move(element_types_)}};
}

[[nodiscard]] inline Type make_generic_type(std::string name_) {
  return Type{Generic_Type{.name = std::move(name_)}};
}

}  // namespace life_lang::semantic
