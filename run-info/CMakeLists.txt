add_library(run-info SHARED main.cpp)

target_link_libraries(run-info geode-sdk nodes)
create_geode_file(run-info)