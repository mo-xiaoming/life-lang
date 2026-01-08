#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace life_lang {

// ============================================================================
// File ID - Compact identifier for source files
// ============================================================================

using File_Id = std::uint32_t;
constexpr File_Id k_invalid_file_id = 0;

// ============================================================================
// Source Position and Range
// ============================================================================

// Position in source code (line and column, 1-indexed)
struct Source_Position {
  std::size_t line{1};
  std::size_t column{1};

  [[nodiscard]] auto operator<=>(Source_Position const&) const = default;
};

// Source range with file information for error reporting
// Every AST node stores a span that includes the file it came from
struct Source_Range {
  File_Id file{k_invalid_file_id};
  Source_Position start{};
  Source_Position end{};

  [[nodiscard]] bool is_single_line() const { return start.line == end.line; }
  [[nodiscard]] std::size_t line_count() const { return end.line - start.line + 1; }
};

// ============================================================================
// Diagnostic types
// ============================================================================

enum class Diagnostic_Level : std::uint8_t { Error, Warning, Note };

// A single diagnostic message with location
struct Diagnostic {
  Diagnostic_Level level = Diagnostic_Level::Error;
  Source_Range range;
  std::string message;

  // Optional: related diagnostics (e.g., "note: declared here")
  std::vector<Diagnostic> notes;
};

// ============================================================================
// Source_File - Source text with line indexing
// ============================================================================

class Source_File {
public:
  Source_File() = default;
  explicit Source_File(std::string source_);
  Source_File(std::string path_, std::string source_);

  void set_source(std::string source_);

  [[nodiscard]] std::string const& path() const { return m_path; }
  [[nodiscard]] std::string const& source() const { return m_source; }
  [[nodiscard]] bool empty() const { return m_source.empty(); }

  // Get source line by line number (1-indexed)
  [[nodiscard]] std::string_view get_line(std::size_t line_number_) const;

  // Convert byte offset to line/column position (1-indexed)
  [[nodiscard]] Source_Position offset_to_position(std::size_t offset_) const;

private:
  std::string m_path;
  std::string m_source;
  std::vector<std::size_t> m_line_offsets;

  void build_line_index();
};

// ============================================================================
// Source_File_Registry - Central registry for all source files
// ============================================================================
// Maps File_Id to source file information. Shared between parser and semantic analysis.

struct Source_File_Registry {
  Source_File_Registry() = default;

  // Non-copyable, movable
  Source_File_Registry(Source_File_Registry const&) = delete;
  Source_File_Registry& operator=(Source_File_Registry const&) = delete;
  Source_File_Registry(Source_File_Registry&&) = default;
  Source_File_Registry& operator=(Source_File_Registry&&) = default;
  ~Source_File_Registry() = default;

  // Register a new source file, returns its File_Id
  // File_Id starts at 1 (0 is k_invalid_file_id)
  [[nodiscard]] File_Id register_file(std::string path_, std::string source_);

  // Get file information by ID
  // Returns nullptr if ID is invalid or not found
  [[nodiscard]] Source_File const* get_file(File_Id id_) const;

  // Get file path by ID (convenience method)
  // Returns empty string if ID is invalid
  [[nodiscard]] std::string_view get_path(File_Id id_) const;

  // Get source line by file ID and line number
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  [[nodiscard]] std::string_view get_line(File_Id id_, std::size_t line_number_) const;

  // Get number of registered files
  [[nodiscard]] std::size_t file_count() const { return m_files.size(); }

private:
  std::vector<Source_File> m_files;  // Index = File_Id - 1
};

// ============================================================================
// Diagnostic printing utilities
// ============================================================================

// Print a single diagnostic with source context in clang style
void print_diagnostic(std::ostream& out_, Source_File_Registry const& registry_, Diagnostic const& diag_);

// ============================================================================
// Diagnostic_Engine - Single-file diagnostic collection (for parser)
// ============================================================================
// Used by the parser for single-file parsing. Takes a file_id from the registry.

struct Diagnostic_Engine {
  // Construct with registry and file_id (file must already be registered)
  Diagnostic_Engine(Source_File_Registry const& registry_, File_Id file_id_);

  // Add diagnostics (automatically uses this file's ID)
  void add_error(Source_Range range_, std::string message_);
  void add_warning(Source_Range range_, std::string message_);

  [[nodiscard]] bool has_errors() const;

  [[nodiscard]] std::vector<Diagnostic> const& diagnostics() const { return m_diagnostics; }
  [[nodiscard]] File_Id file_id() const { return m_file_id; }
  [[nodiscard]] Source_File const& file() const;

  // Convenience accessors delegating to Source_File
  [[nodiscard]] std::string_view source() const;
  [[nodiscard]] std::string_view get_line(std::size_t line_number_) const;
  [[nodiscard]] Source_Position offset_to_position(std::size_t offset_) const;

  // Create a Source_Range for this file from start to current position
  [[nodiscard]] Source_Range make_range(Source_Position start_, Source_Position end_) const;

  void print(std::ostream& out_) const;

  friend std::ostream& operator<<(std::ostream& out_, Diagnostic_Engine const& engine_) {
    engine_.print(out_);
    return out_;
  }

private:
  Source_File_Registry const* m_registry;
  File_Id m_file_id;
  std::vector<Diagnostic> m_diagnostics;
};

// ============================================================================
// Diagnostic_Manager - Multi-file diagnostic collection (for semantic analysis)
// ============================================================================
// Owns its own registry and collects diagnostics across multiple files.

class Diagnostic_Manager {
public:
  Diagnostic_Manager() = default;

  // Non-copyable, movable
  Diagnostic_Manager(Diagnostic_Manager const&) = delete;
  Diagnostic_Manager& operator=(Diagnostic_Manager const&) = delete;
  Diagnostic_Manager(Diagnostic_Manager&&) = default;
  Diagnostic_Manager& operator=(Diagnostic_Manager&&) = default;
  ~Diagnostic_Manager() = default;

  // Access to the underlying registry
  [[nodiscard]] Source_File_Registry& registry() { return m_registry; }
  [[nodiscard]] Source_File_Registry const& registry() const { return m_registry; }

  // Register a source file (delegates to registry)
  [[nodiscard]] File_Id register_file(std::string file_path_, std::string source_);

  // Add diagnostics (uses file from span)
  void add_error(Source_Range range_, std::string message_);
  void add_warning(Source_Range range_, std::string message_);

  // Legacy API for compatibility - uses file path lookup
  void add_error(std::string const& file_path_, Source_Range range_, std::string message_);

  [[nodiscard]] bool has_errors() const;
  [[nodiscard]] std::size_t error_count() const;
  [[nodiscard]] std::size_t diagnostic_count() const { return m_diagnostics.size(); }

  [[nodiscard]] std::vector<Diagnostic> const& all_diagnostics() const { return m_diagnostics; }

  void print(std::ostream& out_) const;

  friend std::ostream& operator<<(std::ostream& out_, Diagnostic_Manager const& manager_) {
    manager_.print(out_);
    return out_;
  }

  void clear_diagnostics() { m_diagnostics.clear(); }
  void clear_all();

private:
  Source_File_Registry m_registry;
  std::vector<Diagnostic> m_diagnostics;
};

}  // namespace life_lang
