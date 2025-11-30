# Generate version header from template
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
  @ONLY
)

# Library with public headers
add_library(life-lang rules.cpp)
target_sources(life-lang
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  FILES
    ast.hpp
    rules.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
)
target_link_libraries(life-lang
  PUBLIC
    Boost::spirit
    nlohmann_json::nlohmann_json
    project_defaults
    fmt::fmt
)

# Compiler executable
add_executable(lifec main.cpp)
target_link_libraries(lifec PRIVATE
  life-lang
)