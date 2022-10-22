add_library(fps-bypass SHARED main.cpp)

target_link_libraries(fps-bypass geode-sdk)
create_geode_file(fps-bypass)