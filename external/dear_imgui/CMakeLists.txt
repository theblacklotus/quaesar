cmake_minimum_required(VERSION 3.10)
project(imgui)

if (APPLE OR LINUX)
    find_package(SDL2 REQUIRED)
endif()

# Specify C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add ImGui source files
file(GLOB IMGUI_SOURCES *.cpp)
list(APPEND IMGUI_SOURCES backends/imgui_impl_sdl2.cpp)
list(APPEND IMGUI_SOURCES backends/imgui_impl_sdlrenderer2.cpp)

# Include directories for ImGui (SDL2 include paths should be handled in the main CMakeLists.txt)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# Create static library
add_library(imgui STATIC ${IMGUI_SOURCES})

if (APPLE OR LINUX)
    target_link_libraries(imgui PRIVATE ${SDL2_LIBRARIES})
else()
    target_include_directories(imgui PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/include")
endif()

