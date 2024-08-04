add_library(life-lang-lib lib.cpp)
target_link_libraries(life-lang-lib PRIVATE project_defaults)
target_include_directories(life-lang-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lifec main.cpp)
target_link_libraries(lifec PRIVATE project_defaults Boost::spirit fmt::fmt)