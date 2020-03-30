cmake_minimum_required(VERSION 2.8)
project(peeper)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE RELEASE)
endif(NOT CMAKE_BUILD_TYPE)

set(src_main
        ./src/Main.cpp
        ./ext/imgui/examples/imgui_impl_glfw.cpp
        ./ext/imgui/examples/imgui_impl_opengles3.cpp
        ./ext/imgui/imgui.cpp
        ./ext/imgui/imgui_demo.cpp
        ./ext/imgui/imgui_draw.cpp
        ./ext/imgui/examples/libs/gl3w/GL/gl3w.c
)

include_directories("${CMAKE_SOURCE_DIR}/ext/imgui/")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui/examples/")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui/examples/libs/")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui/examples/libs/gl3w/")

add_definitions(-D__EGL__)
add_library(peeper SHARED ${src_main})
target_compile_options(peeper PRIVATE -O3 -g -std=c++17 -Wall -ldl -fpic -shared)
target_link_libraries(peeper SDL2 GL dl EGL glfw rt)

add_executable(tester ./src/Tester.cpp)
target_link_libraries(tester rt pthread)

