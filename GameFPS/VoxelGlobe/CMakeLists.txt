cmake_minimum_required(VERSION 3.10)
project(VoxelGlobe)

# Prefer GLVND over legacy OpenGL to suppress the warning (optional but cleaner)
set(OpenGL_GL_PREFERENCE "GLVND")

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Set include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)

# Define the macro globally for all source files
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)

# Assuming your executable target is named "VoxelGlobe"
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

# Add the compile definition for your target
target_compile_definitions(VoxelGlobe PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)

# Link libraries (adjust as per your setup)
target_link_libraries(VoxelGlobe PRIVATE GLEW OpenGL glfw)