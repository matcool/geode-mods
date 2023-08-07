add_library(physics-btns SHARED main.cpp)

target_compile_definitions(physics-btns PUBLIC NOMINMAX GEODE_DEBUG)

CPMAddPackage("gh:erincatto/box2d#v2.4.1")

target_link_libraries(physics-btns box2d)
setup_geode_mod(physics-btns)