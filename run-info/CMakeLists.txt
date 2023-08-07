add_library(run-info SHARED main.cpp)

target_link_libraries(run-info nodes)
setup_geode_mod(run-info)