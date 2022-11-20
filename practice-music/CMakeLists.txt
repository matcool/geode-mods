add_library(practice-music SHARED main.cpp)

target_link_libraries(practice-music geode-sdk)
create_geode_file(practice-music)