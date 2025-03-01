cmake_minimum_required(VERSION 3.10)
project(VoxelGlobe)

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)
add_executable(VoxelGlobe
    src/main.cpp
    src/Block.cpp
    src/Chunk.cpp
    src/Player.cpp
    src/Renderer.cpp
    src/World.cpp
    src/stb_image_impl.cpp
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(VoxelGlobe PRIVATE include include/imgui include/imgui/backends)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
target_link_libraries(VoxelGlobe OpenGL::GL GLEW::GLEW glfw)