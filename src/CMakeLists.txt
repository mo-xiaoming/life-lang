# Generate version header from template
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
  @ONLY
)

# Library with public headers
add_library(life-lang
  diagnostics.cpp
  parser/parser.cpp
  parser/sexp.cpp
  semantic/module_loader.cpp
  semantic/semantic_context.cpp
)
target_sources(life-lang
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  FILES
    parser/ast.hpp
    parser/parser.hpp
    parser/sexp.hpp
    diagnostics.hpp
    expected.hpp
    semantic/semantic_context.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
)
target_link_libraries(life-lang
  PUBLIC
    project_defaults
)

# Compiler executable
add_executable(lifec main.cpp)
target_link_libraries(lifec PRIVATE
  life-lang
)
