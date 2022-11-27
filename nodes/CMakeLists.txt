add_library(nodes INTERFACE)

target_sources(nodes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/nodes.cpp)

target_include_directories(nodes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})