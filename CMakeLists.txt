cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(geode-mods)

if (NOT DEFINED ENV{GEODE_SDK})
	message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
	message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

add_subdirectory(nodes)

if (WIN32)
# 	# add_subdirectory(fast-format)
	add_subdirectory(draw-divide)
# 	add_subdirectory(jump-key)

# 	# no android yet, maybe i could add pinch to zoom or smth
# 	add_subdirectory(scroll-zoom)

# 	add_subdirectory(max-window)
endif()

# add_subdirectory(physics-btns)
# add_subdirectory(circle-tool)
# add_subdirectory(reference-image)
add_subdirectory(run-info)
# add_subdirectory(song-search)
