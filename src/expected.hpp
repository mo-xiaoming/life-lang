#pragma once

#include <type_traits>
#include <utility>
#include <variant>

namespace life_lang {

// ============================================================================
// Unexpected - Represents an error value in Expected
// ============================================================================
template <typename E>
class Unexpected {
public:
  explicit Unexpected(E error_) : m_error(std::move(error_)) {}

  [[nodiscard]] E const& value() const { return m_error; }
  [[nodiscard]] E& value() { return m_error; }

private:
  E m_error;
};

// Helper function to create Unexpected values
template <typename E>
auto unexpected(E&& error_) -> Unexpected<std::decay_t<E>> {
  return Unexpected<std::decay_t<E>>{std::forward<E>(error_)};
}

// ============================================================================
// Expected - Lightweight result type (similar to tl::expected)
// ============================================================================
template <typename T, typename E>
class Expected {
public:
  // Constructors for success case
  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor) - ergonomic error handling
  Expected(T value_) : m_data(std::move(value_)) {}

  // Constructors for error case
  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor) - ergonomic error handling
  Expected(Unexpected<E> error_) : m_data(std::move(error_.value())) {}

  // Copy/move constructors
  Expected(Expected const& other_) = default;
  Expected(Expected&& other_) noexcept = default;
  Expected& operator=(Expected const& other_) = default;
  Expected& operator=(Expected&& other_) noexcept = default;

  // Destructor
  ~Expected() = default;

  // Check if contains a value
  [[nodiscard]] bool has_value() const { return std::holds_alternative<T>(m_data); }

  // Explicit bool conversion
  explicit operator bool() const { return has_value(); }

  // Access value (undefined behavior if contains error)
  [[nodiscard]] T& value() & { return std::get<T>(m_data); }
  [[nodiscard]] T const& value() const& { return std::get<T>(m_data); }
  [[nodiscard]] T&& value() && { return std::get<T>(std::move(m_data)); }
  [[nodiscard]] T const&& value() const&& { return std::get<T>(std::move(m_data)); }

  // Access error (undefined behavior if contains value)
  [[nodiscard]] E& error() & { return std::get<E>(m_data); }
  [[nodiscard]] E const& error() const& { return std::get<E>(m_data); }
  [[nodiscard]] E&& error() && { return std::get<E>(std::move(m_data)); }
  [[nodiscard]] E const&& error() const&& { return std::get<E>(std::move(m_data)); }

  // Dereference operators (for convenience)
  [[nodiscard]] T& operator*() & { return value(); }
  [[nodiscard]] T const& operator*() const& { return value(); }
  [[nodiscard]] T&& operator*() && { return std::move(*this).value(); }
  [[nodiscard]] T const&& operator*() const&& { return std::move(*this).value(); }

  [[nodiscard]] T* operator->() { return &value(); }
  [[nodiscard]] T const* operator->() const { return &value(); }

private:
  std::variant<T, E> m_data;
};

}  // namespace life_lang
