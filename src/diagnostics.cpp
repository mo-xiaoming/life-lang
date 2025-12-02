#include "diagnostics.hpp"

#include <algorithm>

namespace life_lang {

namespace {
// Get level string for output
constexpr std::string_view level_string(Diagnostic_Level a_level) {
  switch (a_level) {
    case Diagnostic_Level::Error:
      return "error";
    case Diagnostic_Level::Warning:
      return "warning";
    case Diagnostic_Level::Note:
      return "note";
  }
  return "error";  // Unreachable but silences warnings
}

// Calculate visual column width (accounting for tabs as 8 spaces)
std::size_t visual_column(std::string_view a_line, std::size_t a_column) {
  std::size_t visual = 0;
  for (std::size_t i = 0; i < std::min(a_column - 1, a_line.size()); ++i) {
    if (a_line[i] == '\t') {
      visual += 8 - (visual % 8);
    } else {
      ++visual;
    }
  }
  return visual;
}
}  // namespace

void Diagnostic_Engine::print(std::ostream& a_out) const {
  for (auto const& diag : m_diagnostics) {
    print_diagnostic(a_out, diag);
  }
}

void Diagnostic_Engine::print_diagnostic(std::ostream& a_out, Diagnostic const& a_diag) const {
  // Format: file:line:column: level: message
  a_out << m_filename << ":" << a_diag.range.start.line << ":" << a_diag.range.start.column << ": "
        << level_string(a_diag.level) << ": " << a_diag.message << "\n";

  // Show source lines with highlighting
  if (a_diag.range.is_single_line()) {
    // Single-line error: show line with ~~~^ highlighting
    auto const line = get_line(a_diag.range.start.line);
    if (line.empty()) {
      return;  // No source available
    }

    // Show the source line
    a_out << "    " << line << "\n";

    // Calculate highlight position and length
    std::size_t const start_col = visual_column(line, a_diag.range.start.column);
    std::size_t end_col = visual_column(line, a_diag.range.end.column);

    // Ensure we have at least one character highlighted
    if (end_col <= start_col) {
      end_col = start_col + 1;
    }

    std::size_t const highlight_len = end_col - start_col;

    // Show highlighting with caret at start (GCC/Clang style): ^~~~
    a_out << "    " << std::string(start_col, ' ') << "^" << std::string(highlight_len - 1, '~') << "\n";
  } else {
    // Multi-line error: show first and last line with markers
    auto const first_line = get_line(a_diag.range.start.line);
    auto const last_line = get_line(a_diag.range.end.line);

    if (!first_line.empty()) {
      a_out << "    " << first_line << "\n";
      std::size_t const start_col = visual_column(first_line, a_diag.range.start.column);
      // Multi-line: caret at start, tildes extending to end of line
      std::size_t const rest_of_line = first_line.size() > start_col ? first_line.size() - start_col : 1;
      a_out << "    " << std::string(start_col, ' ') << "^" << std::string(rest_of_line - 1, '~') << "\n";
    }

    // Show ellipsis if there are intermediate lines
    if (a_diag.range.end.line > a_diag.range.start.line + 1) {
      a_out << "    ...\n";
    }

    if (!last_line.empty() && a_diag.range.end.line != a_diag.range.start.line) {
      a_out << "    " << last_line << "\n";
      std::size_t const end_col = visual_column(last_line, a_diag.range.end.column);
      // Last line: tildes from start to the end position, caret at end
      a_out << "    " << std::string(end_col > 0 ? end_col - 1 : 0, '~') << "^\n";
    }
  }

  // Print notes (indented)
  for (auto const& note : a_diag.notes) {
    a_out << "  ";  // Indent notes
    print_diagnostic(a_out, note);
  }
}

}  // namespace life_lang
