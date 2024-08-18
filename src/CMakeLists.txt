add_library(life-lang main.cpp lib.cpp rules.cpp)
target_link_libraries(life-lang PRIVATE project_defaults Boost::spirit Boost::hana Boost::fusion fmt::fmt)
target_include_directories(life-lang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lifec main.cpp)
target_link_libraries(lifec PRIVATE project_defaults Boost::spirit Boost::hana Boost::fusion fmt::fmt life-lang)