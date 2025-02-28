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
    src/stb_image_impl.cpp  # Add this
)
target_include_directories(VoxelGlobe PRIVATE include)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
target_link_libraries(VoxelGlobe OpenGL::GL GLEW::GLEW glfw)