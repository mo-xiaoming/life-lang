add_library(life-lang rules.cpp)
target_link_libraries(life-lang PRIVATE project_defaults Boost::spirit Boost::fusion fmt::fmt nlohmann_json::nlohmann_json)
target_include_directories(life-lang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lifec main.cpp)
target_link_libraries(lifec PRIVATE project_defaults Boost::spirit Boost::fusion fmt::fmt life-lang)