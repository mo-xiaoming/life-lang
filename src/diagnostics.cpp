#include "diagnostics.hpp"

#include "utils.hpp"

#include <algorithm>
#include <format>

namespace life_lang {

// ============================================================================
// Source_File implementation
// ============================================================================

Source_File::Source_File(std::string source_) : m_source(std::move(source_)) {
  build_line_index();
}

Source_File::Source_File(std::string path_, std::string source_)
    : m_path(std::move(path_)), m_source(std::move(source_)) {
  build_line_index();
}

void Source_File::set_source(std::string source_) {
  m_source = std::move(source_);
  build_line_index();
}

void Source_File::build_line_index() {
  m_line_offsets.clear();
  m_line_offsets.push_back(0);  // Line 1 starts at offset 0
  for (std::size_t i = 0; i < m_source.size(); ++i) {
    if (m_source[i] == '\n') {
      m_line_offsets.push_back(i + 1);
    } else if (m_source[i] == '\r') {
      if (i + 1 < m_source.size() && m_source[i + 1] == '\n') {
        continue;  // CRLF - let \n handling record the line
      }
      m_line_offsets.push_back(i + 1);  // Old Mac CR
    }
  }
}

std::string_view Source_File::get_line(std::size_t line_number_) const {
  if (line_number_ == 0 || line_number_ > m_line_offsets.size() || m_source.empty()) {
    return {};
  }

  std::size_t const start = m_line_offsets[line_number_ - 1];
  std::size_t const end = (line_number_ < m_line_offsets.size()) ? m_line_offsets[line_number_] : m_source.size();

  std::size_t length = end - start;
  if (length > 0 && m_source[end - 1] == '\n') {
    --length;
  }
  return {m_source.data() + start, length};
}

Source_Position Source_File::offset_to_position(std::size_t offset_) const {
  auto const it = std::ranges::upper_bound(m_line_offsets, offset_);
  auto const line = static_cast<std::size_t>(std::distance(m_line_offsets.begin(), it));
  std::size_t const line_start = (line > 0) ? m_line_offsets[line - 1] : 0;
  std::size_t const column = offset_ - line_start + 1;
  return Source_Position{.line = line, .column = column};
}

// ============================================================================
// Source_File_Registry implementation
// ============================================================================

File_Id Source_File_Registry::register_file(std::string path_, std::string source_) {
  m_files.emplace_back(std::move(path_), std::move(source_));
  return static_cast<File_Id>(m_files.size());  // ID = index + 1
}

Source_File const* Source_File_Registry::get_file(File_Id id_) const {
  if (id_ == k_invalid_file_id || id_ > m_files.size()) {
    return nullptr;
  }
  return &m_files[id_ - 1];
}

std::string_view Source_File_Registry::get_path(File_Id id_) const {
  auto const* file = get_file(id_);
  return (file != nullptr) ? file->path() : std::string_view{};
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::string_view Source_File_Registry::get_line(File_Id id_, std::size_t line_number_) const {
  auto const* file = get_file(id_);
  return (file != nullptr) ? file->get_line(line_number_) : std::string_view{};
}

// ============================================================================
// Diagnostic printing utilities
// ============================================================================

namespace {

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

void print_source_context(std::ostream& out_, Source_File const* source_, Diagnostic const& diag_) {
  if (source_ == nullptr || source_->empty()) {
    return;
  }

  if (diag_.range.is_single_line()) {
    auto const line = source_->get_line(diag_.range.start.line);
    if (line.empty()) {
      return;
    }

    out_ << std::format("    {}\n", line);

    std::size_t const start_col = visual_column(line, diag_.range.start.column);
    std::size_t end_col = visual_column(line, diag_.range.end.column);
    if (end_col <= start_col) {
      end_col = start_col + 1;
    }
    std::size_t const highlight_len = end_col - start_col;
    out_ << std::format("    {}^{}\n", std::string(start_col, ' '), std::string(highlight_len - 1, '~'));
  } else {
    auto const first_line = source_->get_line(diag_.range.start.line);
    auto const last_line = source_->get_line(diag_.range.end.line);

    if (!first_line.empty()) {
      out_ << std::format("    {}\n", first_line);
      std::size_t const start_col = visual_column(first_line, diag_.range.start.column);
      std::size_t const rest_of_line = first_line.size() > start_col ? first_line.size() - start_col : 1;
      out_ << std::format("    {}^{}\n", std::string(start_col, ' '), std::string(rest_of_line - 1, '~'));
    }

    if (diag_.range.end.line > diag_.range.start.line + 1) {
      out_ << "    ...\n";
    }

    if (!last_line.empty() && diag_.range.end.line != diag_.range.start.line) {
      out_ << std::format("    {}\n", last_line);
      std::size_t const end_col = visual_column(last_line, diag_.range.end.column);
      out_ << std::format("    {}^\n", std::string(end_col > 0 ? end_col - 1 : 0, '~'));
    }
  }
}

}  // namespace

void print_diagnostic(std::ostream& out_, Source_File_Registry const& registry_, Diagnostic const& diag_) {
  auto const* source = registry_.get_file(diag_.range.file);
  std::string const path = (source != nullptr) ? source->path() : std::string{"<unknown>"};

  out_ << std::format(
      "{}:{}:{}: {}: {}\n",
      path,
      diag_.range.start.line,
      diag_.range.start.column,
      level_string(diag_.level),
      diag_.message
  );

  print_source_context(out_, source, diag_);

  for (auto const& note: diag_.notes) {
    out_ << "  ";
    print_diagnostic(out_, registry_, note);
  }
}

// ============================================================================
// Diagnostic_Engine implementation
// ============================================================================

Diagnostic_Engine::Diagnostic_Engine(Source_File_Registry const& registry_, File_Id file_id_)
    : m_registry(&registry_), m_file_id(file_id_) {}

void Diagnostic_Engine::add_error(Source_Range range_, std::string message_) {
  // Ensure the file ID matches
  Source_Range actual_range = range_;
  actual_range.file = m_file_id;
  m_diagnostics.push_back(
      Diagnostic{.level = Diagnostic_Level::Error, .range = actual_range, .message = std::move(message_), .notes = {}}
  );
}

void Diagnostic_Engine::add_warning(Source_Range range_, std::string message_) {
  Source_Range actual_range = range_;
  actual_range.file = m_file_id;
  m_diagnostics.push_back(
      Diagnostic{.level = Diagnostic_Level::Warning, .range = actual_range, .message = std::move(message_), .notes = {}}
  );
}

bool Diagnostic_Engine::has_errors() const {
  return std::ranges::any_of(m_diagnostics, [](auto const& diag_) { return diag_.level == Diagnostic_Level::Error; });
}

Source_File const& Diagnostic_Engine::file() const {
  auto const* f = m_registry->get_file(m_file_id);
  if (f == nullptr) {
    static Source_File const s_empty_file;
    return s_empty_file;
  }
  return *f;
}

std::string_view Diagnostic_Engine::source() const {
  return file().source();
}

std::string_view Diagnostic_Engine::get_line(std::size_t line_number_) const {
  return file().get_line(line_number_);
}

Source_Position Diagnostic_Engine::offset_to_position(std::size_t offset_) const {
  return file().offset_to_position(offset_);
}

Source_Range Diagnostic_Engine::make_range(Source_Position start_, Source_Position end_) const {
  return Source_Range{.file = m_file_id, .start = start_, .end = end_};
}

void Diagnostic_Engine::print(std::ostream& out_) const {
  for (auto const& diag: m_diagnostics) {
    print_diagnostic(out_, *m_registry, diag);
  }
}

// ============================================================================
// Diagnostic_Manager implementation
// ============================================================================

File_Id Diagnostic_Manager::register_file(std::string file_path_, std::string source_) {
  return m_registry.register_file(std::move(file_path_), std::move(source_));
}

void Diagnostic_Manager::add_error(Source_Range range_, std::string message_) {
  m_diagnostics.push_back(
      Diagnostic{.level = Diagnostic_Level::Error, .range = range_, .message = std::move(message_), .notes = {}}
  );
}

void Diagnostic_Manager::add_warning(Source_Range range_, std::string message_) {
  m_diagnostics.push_back(
      Diagnostic{.level = Diagnostic_Level::Warning, .range = range_, .message = std::move(message_), .notes = {}}
  );
}

void Diagnostic_Manager::add_error(std::string const& file_path_, Source_Range range_, std::string message_) {
  // Legacy API - the file should already be registered, but the range's file ID takes precedence
  // This is for backward compatibility during migration
  (void)file_path_;  // Unused - we trust the range.file
  add_error(range_, std::move(message_));
}

bool Diagnostic_Manager::has_errors() const {
  return std::ranges::any_of(m_diagnostics, [](auto const& diag_) { return diag_.level == Diagnostic_Level::Error; });
}

std::size_t Diagnostic_Manager::error_count() const {
  return static_cast<std::size_t>(std::ranges::count_if(m_diagnostics, [](auto const& diag_) {
    return diag_.level == Diagnostic_Level::Error;
  }));
}

void Diagnostic_Manager::print(std::ostream& out_) const {
  for (auto const& diag: m_diagnostics) {
    print_diagnostic(out_, m_registry, diag);
  }
}

void Diagnostic_Manager::clear_all() {
  m_diagnostics.clear();
  m_registry = Source_File_Registry{};
}

}  // namespace life_lang
