add_library(song-preview SHARED main.cpp samples.cpp waveform.cpp)

target_link_libraries(song-preview UIBuilder)
setup_geode_mod(song-preview)

target_compile_definitions(song-preview PRIVATE _CRT_SECURE_NO_WARNINGS=1)
target_compile_options(song-preview PRIVATE -Wno-c23-extensions)