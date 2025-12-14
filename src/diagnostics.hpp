#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace life_lang {

// Position in source code
struct Source_Position {
  std::size_t line{1};
  std::size_t column{1};

  [[nodiscard]] auto operator<=>(Source_Position const&) const = default;
};

// Source range for highlighting
struct Source_Range {
  Source_Position start{};
  Source_Position end{};  // Exclusive

  [[nodiscard]] bool is_single_line() const { return start.line == end.line; }
  [[nodiscard]] std::size_t line_count() const { return end.line - start.line + 1; }
};

// Diagnostic severity levels
enum class Diagnostic_Level : std::uint8_t { Error, Warning, Note };

// A single diagnostic message with location
struct Diagnostic {
  Diagnostic_Level level = Diagnostic_Level::Error;
  Source_Range range;
  std::string message;

  // Optional: related diagnostics (e.g., "note: declared here")
  std::vector<Diagnostic> notes;
};

// Collection of diagnostics with source text access
class Diagnostic_Engine {
 public:
  explicit Diagnostic_Engine(std::string a_filename = "<input>", std::string a_source = "")
      : m_filename(std::move(a_filename)), m_source(std::move(a_source)) {
    // Pre-compute line offsets for fast line lookup
    build_line_index();
  }

  // Add a diagnostic
  void add_error(Source_Range a_range, std::string a_message) {
    m_diagnostics.push_back(
        Diagnostic{.level = Diagnostic_Level::Error, .range = a_range, .message = std::move(a_message), .notes = {}}
    );
  }

  void add_warning(Source_Range a_range, std::string a_message) {
    m_diagnostics.push_back(
        Diagnostic{.level = Diagnostic_Level::Warning, .range = a_range, .message = std::move(a_message), .notes = {}}
    );
  }

  // Check if any errors were reported
  [[nodiscard]] bool has_errors() const {
    return std::ranges::any_of(m_diagnostics, [](auto const& a_diag) {
      return a_diag.level == Diagnostic_Level::Error;
    });
  }

  [[nodiscard]] std::vector<Diagnostic> const& diagnostics() const { return m_diagnostics; }
  [[nodiscard]] std::string const& filename() const { return m_filename; }
  [[nodiscard]] std::string const& source() const { return m_source; }

  // Get source line by line number (1-indexed)
  [[nodiscard]] std::string_view get_line(std::size_t a_line_number) const {
    if (a_line_number == 0 || a_line_number > m_line_offsets.size()) {
      return "";
    }
    std::size_t const start = m_line_offsets[a_line_number - 1];
    std::size_t const end = (a_line_number < m_line_offsets.size()) ? m_line_offsets[a_line_number] : m_source.size();

    // Trim trailing newline if present
    std::size_t length = end - start;
    if (length > 0 && m_source[end - 1] == '\n') {
      --length;
    }
    return {m_source.data() + start, length};
  }

  // Format all diagnostics in clang style
  void print(std::ostream& a_out) const;

  friend std::ostream& operator<<(std::ostream& a_out, Diagnostic_Engine const& a_engine) {
    a_engine.print(a_out);
    return a_out;
  }

 private:
  std::string m_filename;
  std::string m_source;
  std::vector<Diagnostic> m_diagnostics;
  std::vector<std::size_t> m_line_offsets;  // Start offset of each line

  void build_line_index() {
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

  // Print single diagnostic
  void print_diagnostic(std::ostream& a_out, Diagnostic const& a_diag) const;
};

}  // namespace life_lang
