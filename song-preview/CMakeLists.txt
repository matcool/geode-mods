add_library(song-preview SHARED main.cpp)

target_link_libraries(song-preview UIBuilder)
setup_geode_mod(song-preview)