cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(geode-mods)

if (NOT DEFINED ENV{GEODE_SDK})
	message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
	message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

add_subdirectory(nodes)

if (WIN32)
	add_subdirectory(fps-bypass)
endif()
add_subdirectory(scroll-zoom)
add_subdirectory(physics-btns)
add_subdirectory(practice-music)
add_subdirectory(circle-tool)
add_subdirectory(rgb-color-input)
add_subdirectory(run-info)
add_subdirectory(song-search)