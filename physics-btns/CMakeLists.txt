add_library(physics-btns SHARED main.cpp)

set(BOX2D_BUILD_UNIT_TESTS OFF)
set(BOX2D_BUILD_TESTBED OFF)
CPMAddPackage("gh:erincatto/box2d#v2.4.1")

target_link_libraries(physics-btns box2d)
setup_geode_mod(physics-btns)