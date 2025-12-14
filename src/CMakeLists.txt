# Generate version header from template
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
  @ONLY
)

# Library with public headers
add_library(life-lang diagnostics.cpp parser.cpp)
target_sources(life-lang
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  FILES
    ast.hpp
    diagnostics.hpp
    expected.hpp
    parser.hpp
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