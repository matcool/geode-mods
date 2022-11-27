add_library(circle-tool SHARED main.cpp)

target_compile_definitions(circle-tool PUBLIC NOMINMAX)

target_link_libraries(circle-tool geode-sdk nodes)
create_geode_file(circle-tool)