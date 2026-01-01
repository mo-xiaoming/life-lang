#pragma once

#include <cstdlib>
#include <format>
#include <iostream>
#include <string_view>

namespace life_lang {

// Verify internal compiler invariants
// Always aborts on failure (debug and release) with a clear error message
// Use for conditions that should NEVER fail if the compiler is correct
inline void verify(bool condition_, std::string_view message_) {
  if (!condition_) {
    std::cerr << std::format("\nINTERNAL COMPILER ERROR: {}\n", message_) << std::flush;
    std::abort();
  }
}

// Mark code path as unreachable
// Use only when the compiler guarantees this path cannot be reached (e.g., exhaustive enum switch)
// Triggers UB if actually reached - use verify() for runtime invariants instead
[[noreturn]] inline void unreachable() {
  __builtin_unreachable();
}

}  // namespace life_lang
