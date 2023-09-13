add_library(song-search SHARED main.cpp)

target_compile_definitions(song-search PUBLIC NOMINMAX)

target_link_libraries(song-search nodes)
setup_geode_mod(song-search)