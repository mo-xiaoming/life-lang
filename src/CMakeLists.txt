add_executable(lifec main.cpp)
target_link_libraries(lifec PRIVATE project_defaults Boost::spirit fmt::fmt)