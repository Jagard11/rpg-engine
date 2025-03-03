# ./GameFPS/VoxelGlobe/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(VoxelGlobe)

set(OpenGL_GL_PREFERENCE "GLVND")

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/Core)
include_directories(${CMAKE_SOURCE_DIR}/include/World)
include_directories(${CMAKE_SOURCE_DIR}/include/Player)
include_directories(${CMAKE_SOURCE_DIR}/include/Rendering)
include_directories(${CMAKE_SOURCE_DIR}/include/UI)
include_directories(${CMAKE_SOURCE_DIR}/include/UI/Inventory)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui)
include_directories(${CMAKE_SOURCE_DIR}/third_party/stb)

# Define for ImGui
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)

# Source files with updated paths
add_executable(VoxelGlobe
    src/Core/main.cpp
    src/Player/Player.cpp
    src/Player/Movement.cpp
    src/UI/Inventory/Inventory.cpp
    src/UI/Inventory/InventoryUI.cpp
    src/UI/VoxelHighlightUI.cpp  # Added this line
    src/Rendering/Renderer.cpp
    src/World/World.cpp
    src/World/Chunk.cpp
    src/World/Block.cpp
    src/VoxelManipulator.cpp
    third_party/stb/stb_image_impl.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_impl_glfw.cpp
    third_party/imgui/imgui_impl_opengl3.cpp
)

# Link libraries
target_link_libraries(VoxelGlobe OpenGL::GL GLEW::GLEW glfw)