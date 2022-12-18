add_library(rgb-color-input SHARED main.cpp)

target_link_libraries(rgb-color-input geode-sdk)
create_geode_file(rgb-color-input)