add_library(circle-tool SHARED main.cpp)

target_compile_definitions(circle-tool PUBLIC NOMINMAX)

target_link_libraries(circle-tool nodes)
setup_geode_mod(circle-tool)