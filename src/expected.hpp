#pragma once

#include <format>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

namespace life_lang {

// ============================================================================
// Internal helper for error reporting
// ============================================================================
[[noreturn]] inline void abort_with_message(char const* message_) {
  std::cerr << std::format("\nFATAL ERROR: {}\n", message_);
  std::cerr << std::flush;
  std::abort();
}

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

// Deduction guide for Unexpected
template <typename E>
Unexpected(E) -> Unexpected<std::decay_t<E>>;

// ============================================================================
// Expected - LLVM-style result type with mandatory error checking
// ============================================================================
// Key features:
// 1. Move-only semantics (cannot be copied)
// 2. Asserts if error is not explicitly checked or consumed
// 3. Explicit checking via operator bool() or has_value()
// 4. Explicit ignoring via consume_error()
// 5. Explicit error extraction via take_error()
//
// Usage patterns:
//   auto result = foo();
//   if (result) {
//     use(*result);  // Safe - checked first
//   } else {
//     auto err = std::move(result).take_error();
//     handle(std::move(err));
//   }
//
// Or propagate:
//   auto result = foo();
//   if (!result) {
//     return std::move(result).take_error();
//   }
//   use(*result);
//
// Or explicitly ignore:
//   auto result = foo();
//   result.consume_error();  // Explicitly discard
// ============================================================================
template <typename T, typename E>
class Expected {
private:
  // Error message constants
  static constexpr char const* k_err_unchecked_destruction =
      "Expected value must be explicitly checked or consumed before destruction";
  static constexpr char const* k_err_unchecked_assignment = "Expected value must be checked before assignment";
  static constexpr char const* k_err_unchecked_value = "Must check Expected before accessing value";
  static constexpr char const* k_err_unchecked_error = "Must check Expected before accessing error";
  static constexpr char const* k_err_unchecked_deref = "Must check Expected before dereferencing";

public:
  // Constructors for success case
  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor) - ergonomic error handling
  Expected(T value_) : m_data(std::move(value_)), m_checked(false) {}

  // Constructors for error case
  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor) - ergonomic error handling
  Expected(Unexpected<E> error_) : m_data(std::move(error_.value())), m_checked(false) {}

  // Disable copy - force move semantics
  Expected(Expected const& other_) = delete;
  Expected& operator=(Expected const& other_) = delete;

  // Move constructor
  Expected(Expected&& other_) noexcept : m_data(std::move(other_.m_data)), m_checked(other_.m_checked) {
    other_.m_checked = true;  // Transfer ownership - other is now checked
  }

  // Move assignment
  Expected& operator=(Expected&& other_) noexcept {
    if (this != &other_) {
      if (!m_checked) {
        abort_with_message(k_err_unchecked_assignment);
      }
      m_data = std::move(other_.m_data);
      m_checked = other_.m_checked;
      other_.m_checked = true;  // Transfer ownership
    }
    return *this;
  }

  // Destructor - aborts if error was never checked
  ~Expected() {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_destruction);
    }
  }

  // Check if contains a value (marks as checked)
  [[nodiscard]] bool has_value() const {
    m_checked = true;
    return std::holds_alternative<T>(m_data);
  }

  // Explicit bool conversion (marks as checked)
  explicit operator bool() const {
    m_checked = true;
    return std::holds_alternative<T>(m_data);
  }

  // Access value (requires prior checking via has_value() or operator bool())
  [[nodiscard]] T& value() & {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_value);
    }
    return std::get<T>(m_data);
  }

  [[nodiscard]] T const& value() const& {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_value);
    }
    return std::get<T>(m_data);
  }

  [[nodiscard]] T&& value() && {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_value);
    }
    return std::get<T>(std::move(m_data));
  }

  // Take error (moves error out, marks as checked)
  // Can only be called on rvalue (must move the Expected)
  [[nodiscard]] E take_error() && {
    m_checked = true;
    return std::get<E>(std::move(m_data));
  }

  // Peek at error without taking ownership (requires prior checking)
  [[nodiscard]] E const& error() const& {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_error);
    }
    return std::get<E>(m_data);
  }

  [[nodiscard]] E& error() & {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_error);
    }
    return std::get<E>(m_data);
  }

  // Explicitly consume/ignore error (marks as checked without handling)
  void consume_error() const { m_checked = true; }

  // Dereference operators (requires prior checking)
  [[nodiscard]] T& operator*() & {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_deref);
    }
    return std::get<T>(m_data);
  }

  [[nodiscard]] T const& operator*() const& {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_deref);
    }
    return std::get<T>(m_data);
  }

  [[nodiscard]] T&& operator*() && {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_deref);
    }
    return std::get<T>(std::move(m_data));
  }

  [[nodiscard]] T* operator->() {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_deref);
    }
    return &std::get<T>(m_data);
  }

  [[nodiscard]] T const* operator->() const {
    if (!m_checked) {
      abort_with_message(k_err_unchecked_deref);
    }
    return &std::get<T>(m_data);
  }

private:
  std::variant<T, E> m_data;
  mutable bool m_checked;  // Tracks whether error has been explicitly checked or consumed
};

}  // namespace life_lang
