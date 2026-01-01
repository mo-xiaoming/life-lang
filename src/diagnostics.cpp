#include "diagnostics.hpp"

#include "utils.hpp"

#include <algorithm>

namespace life_lang {

bool Diagnostic_Engine::has_errors() const {
  return std::ranges::any_of(m_diagnostics, [](auto const& diag_) { return diag_.level == Diagnostic_Level::Error; });
}

std::string_view Diagnostic_Engine::get_line(std::size_t line_number_) const {
  verify(
      line_number_ > 0 && line_number_ <= m_line_offsets.size(),
      std::format("Invalid line number {} in get_line()", line_number_)
  );

  std::size_t const start = m_line_offsets[line_number_ - 1];
  std::size_t const end = (line_number_ < m_line_offsets.size()) ? m_line_offsets[line_number_] : m_source.size();

  // Trim trailing newline if present
  std::size_t length = end - start;
  if (length > 0 && m_source[end - 1] == '\n') {
    --length;
  }
  return {m_source.data() + start, length};
}

void Diagnostic_Engine::build_line_index() {
  m_line_offsets.push_back(0);  // Line 1 starts at offset 0
  for (std::size_t i = 0; i < m_source.size(); ++i) {
    // Handle all line ending conventions:
    // - Unix/Linux: \n (LF)
    // - Windows: \r\n (CRLF)
    // - Old Mac: \r (CR)
    if (m_source[i] == '\n') {
      m_line_offsets.push_back(i + 1);
    } else if (m_source[i] == '\r') {
      // Check if it's \r\n (Windows) or standalone \r (old Mac)
      if (i + 1 < m_source.size() && m_source[i + 1] == '\n') {
        // Windows CRLF: skip the \r, let \n handling record the line start
        continue;
      }
      // Old Mac CR: treat as line ending
      m_line_offsets.push_back(i + 1);
    }
  }
}

Source_Position Diagnostic_Engine::offset_to_position(std::size_t offset_) const {
  // Binary search to find which line contains this offset
  auto const it = std::ranges::upper_bound(m_line_offsets, offset_);

  // it points to the first line start that is > offset, so the line before it contains offset
  auto const line = static_cast<std::size_t>(std::distance(m_line_offsets.begin(), it));

  // Column is offset from the start of the line (1-indexed)
  std::size_t const line_start = (line > 0) ? m_line_offsets[line - 1] : 0;
  std::size_t const column = offset_ - line_start + 1;

  return Source_Position{.line = line, .column = column};
}

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
  unreachable();
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
  out_ << std::format(
      "{}:{}:{}: {}: {}\n",
      m_filename,
      diag_.range.start.line,
      diag_.range.start.column,
      level_string(diag_.level),
      diag_.message
  );

  // Show source lines with highlighting
  if (diag_.range.is_single_line()) {
    // Single-line error: show line with ~~~^ highlighting
    auto const line = get_line(diag_.range.start.line);
    if (line.empty()) {
      return;  // No source available
    }

    // Show the source line
    out_ << std::format("    {}\n", line);

    // Calculate highlight position and length
    std::size_t const start_col = visual_column(line, diag_.range.start.column);
    std::size_t end_col = visual_column(line, diag_.range.end.column);

    // Ensure we have at least one character highlighted
    if (end_col <= start_col) {
      end_col = start_col + 1;
    }

    std::size_t const highlight_len = end_col - start_col;

    // Show highlighting with caret at start (GCC/Clang style): ^~~~
    out_ << std::format("    {}^{}\n", std::string(start_col, ' '), std::string(highlight_len - 1, '~'));
  } else {
    // Multi-line error: show first and last line with markers
    auto const first_line = get_line(diag_.range.start.line);
    auto const last_line = get_line(diag_.range.end.line);

    if (!first_line.empty()) {
      out_ << std::format("    {}\n", first_line);
      std::size_t const start_col = visual_column(first_line, diag_.range.start.column);
      // Multi-line: caret at start, tildes extending to end of line
      std::size_t const rest_of_line = first_line.size() > start_col ? first_line.size() - start_col : 1;
      out_ << std::format("    {}^{}\n", std::string(start_col, ' '), std::string(rest_of_line - 1, '~'));
    }

    // Show ellipsis if there are intermediate lines
    if (diag_.range.end.line > diag_.range.start.line + 1) {
      out_ << "    ...\n";
    }

    if (!last_line.empty() && diag_.range.end.line != diag_.range.start.line) {
      out_ << std::format("    {}\n", last_line);
      std::size_t const end_col = visual_column(last_line, diag_.range.end.column);
      // Last line: tildes from start to the end position, caret at end
      out_ << std::format("    {}^\n", std::string(end_col > 0 ? end_col - 1 : 0, '~'));
    }
  }

  // Print notes (indented)
  for (auto const& note: diag_.notes) {
    out_ << "  ";  // Indent notes
    print_diagnostic(out_, note);
  }
}

}  // namespace life_lang
