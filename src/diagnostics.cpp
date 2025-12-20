#include "diagnostics.hpp"

#include <algorithm>

namespace life_lang {

namespace {
// Get level string for output
constexpr std::string_view level_string(Diagnostic_Level level_) {
  switch (level_) {
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
std::size_t visual_column(std::string_view line_, std::size_t column_) {
  std::size_t visual = 0;
  for (std::size_t i = 0; i < std::min(column_ - 1, line_.size()); ++i) {
    if (line_[i] == '\t') {
      visual += 8 - (visual % 8);
    } else {
      ++visual;
    }
  }
  return visual;
}
}  // namespace

void Diagnostic_Engine::print(std::ostream& out_) const {
  for (auto const& diag: m_diagnostics) {
    print_diagnostic(out_, diag);
  }
}

void Diagnostic_Engine::print_diagnostic(std::ostream& out_, Diagnostic const& diag_) const {
  // Format: file:line:column: level: message
  out_ << m_filename << ":" << diag_.range.start.line << ":" << diag_.range.start.column << ": "
       << level_string(diag_.level) << ": " << diag_.message << "\n";

  // Show source lines with highlighting
  if (diag_.range.is_single_line()) {
    // Single-line error: show line with ~~~^ highlighting
    auto const line = get_line(diag_.range.start.line);
    if (line.empty()) {
      return;  // No source available
    }

    // Show the source line
    out_ << "    " << line << "\n";

    // Calculate highlight position and length
    std::size_t const start_col = visual_column(line, diag_.range.start.column);
    std::size_t end_col = visual_column(line, diag_.range.end.column);

    // Ensure we have at least one character highlighted
    if (end_col <= start_col) {
      end_col = start_col + 1;
    }

    std::size_t const highlight_len = end_col - start_col;

    // Show highlighting with caret at start (GCC/Clang style): ^~~~
    out_ << "    " << std::string(start_col, ' ') << "^" << std::string(highlight_len - 1, '~') << "\n";
  } else {
    // Multi-line error: show first and last line with markers
    auto const first_line = get_line(diag_.range.start.line);
    auto const last_line = get_line(diag_.range.end.line);

    if (!first_line.empty()) {
      out_ << "    " << first_line << "\n";
      std::size_t const start_col = visual_column(first_line, diag_.range.start.column);
      // Multi-line: caret at start, tildes extending to end of line
      std::size_t const rest_of_line = first_line.size() > start_col ? first_line.size() - start_col : 1;
      out_ << "    " << std::string(start_col, ' ') << "^" << std::string(rest_of_line - 1, '~') << "\n";
    }

    // Show ellipsis if there are intermediate lines
    if (diag_.range.end.line > diag_.range.start.line + 1) {
      out_ << "    ...\n";
    }

    if (!last_line.empty() && diag_.range.end.line != diag_.range.start.line) {
      out_ << "    " << last_line << "\n";
      std::size_t const end_col = visual_column(last_line, diag_.range.end.column);
      // Last line: tildes from start to the end position, caret at end
      out_ << "    " << std::string(end_col > 0 ? end_col - 1 : 0, '~') << "^\n";
    }
  }

  // Print notes (indented)
  for (auto const& note: diag_.notes) {
    out_ << "  ";  // Indent notes
    print_diagnostic(out_, note);
  }
}

}  // namespace life_lang
