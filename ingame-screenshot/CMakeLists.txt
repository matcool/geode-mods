add_library(ingame-screenshot SHARED main.cpp RenderTexture.cpp)

# target_link_libraries(ingame-screenshot UIBuilder)
setup_geode_mod(ingame-screenshot)