cmake_minimum_required(VERSION 3.10)
project(VoxelGlobe)

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)

add_executable(VoxelGlobe
    src/main.cpp
    src/Player.cpp
    src/Renderer.cpp
    src/World.cpp
    src/Chunk.cpp
    src/Block.cpp
    src/stb_image_impl.cpp
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_impl_glfw.cpp
    include/imgui/imgui_impl_opengl3.cpp
)

target_link_libraries(VoxelGlobe OpenGL::GL GLEW::GLEW glfw)