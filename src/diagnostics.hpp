#pragma once

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
  Source_Position end{};

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
struct Diagnostic_Engine {
  explicit Diagnostic_Engine(std::string filename_, std::string_view source_)
      : m_filename(std::move(filename_)), m_source(source_) {
    // Pre-compute line offsets for fast line lookup
    build_line_index();
  }

  // Add a diagnostic
  void add_error(Source_Range range_, std::string message_) {
    m_diagnostics.push_back(
        Diagnostic{.level = Diagnostic_Level::Error, .range = range_, .message = std::move(message_), .notes = {}}
    );
  }

  void add_warning(Source_Range range_, std::string message_) {
    m_diagnostics.push_back(
        Diagnostic{.level = Diagnostic_Level::Warning, .range = range_, .message = std::move(message_), .notes = {}}
    );
  }

  // Check if any errors were reported
  [[nodiscard]] bool has_errors() const;

  [[nodiscard]] std::vector<Diagnostic> const& diagnostics() const { return m_diagnostics; }
  [[nodiscard]] std::string const& filename() const { return m_filename; }
  [[nodiscard]] std::string_view source() const { return m_source; }

  // Get source line by line number (1-indexed)
  [[nodiscard]] std::string_view get_line(std::size_t line_number_) const;

  // Convert byte offset to line/column position (1-indexed)
  [[nodiscard]] Source_Position offset_to_position(std::size_t offset_) const;

  // Format all diagnostics in clang style
  void print(std::ostream& out_) const;

  friend std::ostream& operator<<(std::ostream& out_, Diagnostic_Engine const& engine_) {
    engine_.print(out_);
    return out_;
  }

private:
  std::string m_filename;
  std::string_view m_source;
  std::vector<Diagnostic> m_diagnostics;
  std::vector<std::size_t> m_line_offsets;  // Start offset of each line

  void build_line_index();

  // Print single diagnostic
  void print_diagnostic(std::ostream& out_, Diagnostic const& diag_) const;
};

}  // namespace life_lang
