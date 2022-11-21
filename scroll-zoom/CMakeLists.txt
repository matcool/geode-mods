add_library(scroll-zoom SHARED main.cpp)

target_compile_definitions(scroll-zoom PUBLIC NOMINMAX)

target_link_libraries(scroll-zoom geode-sdk)
create_geode_file(scroll-zoom)