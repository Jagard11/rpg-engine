cmake_minimum_required(VERSION 3.10)
project(VoxelGlobe)

set(OpenGL_GL_PREFERENCE "GLVND")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug mode configuration
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DENABLE_PROFILING)
    message(STATUS "Debug build - Profiling enabled")
endif()

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
include_directories(${CMAKE_SOURCE_DIR}/include/Debug)
include_directories(${CMAKE_SOURCE_DIR}/include/Graphics)
include_directories(${CMAKE_SOURCE_DIR}/include/Utils)      # Added for SphereUtils
include_directories(${CMAKE_SOURCE_DIR}/include/Physics)    # Added for CollisionSystem
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui)
include_directories(${CMAKE_SOURCE_DIR}/third_party/stb)
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)

add_executable(VoxelGlobe
    # Core
    src/Core/main.cpp
    src/Core/InputHandler.cpp             # Added
    src/Core/GameStateManager.cpp         # Added
    
    # Player
    src/Player/Player.cpp
    src/Player/Movement.cpp
    
    # UI
    src/UI/Inventory/Inventory.cpp
    src/UI/Inventory/InventoryUI.cpp
    src/UI/VoxelHighlightUI.cpp
    
    # Rendering
    src/Rendering/Renderer.cpp
    
    # World
    src/World/World.cpp
    src/World/Chunk.cpp
    src/World/Block.cpp
    
    # Utilities
    src/VoxelManipulator.cpp
    
    # Debug - Updated with new components
    src/Debug/DebugManager.cpp
    src/Debug/DebugWindow.cpp
    src/Debug/Logger.cpp                  # New file
    src/Debug/DebugSystem.cpp             # New file
    src/Debug/Profiler.cpp                # New file
    src/Debug/GodViewDebugTool.cpp
    src/Debug/GodViewWindow.cpp
    # src/Debug/GlobeUpdater.cpp
    src/Debug/DebugWindowGodViewPanel.cpp

    # Graphics
    src/Graphics/GraphicsSettings.cpp
    src/Graphics/ShaderManager.cpp         # Added
    src/Graphics/ResourceManager.cpp       # Added
    src/Graphics/MeshGenerator.cpp         # Added new file
    
    # Third-party libraries
    third_party/stb/stb_image_impl.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_impl_glfw.cpp
    third_party/imgui/imgui_impl_opengl3.cpp
)

# Create directory structure
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/include/Utils)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/include/Physics)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/Core)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/Graphics)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/logs)  # Create logs directory for log files

target_link_libraries(VoxelGlobe OpenGL::GL GLEW::GLEW glfw)

# Copy shader and texture assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})

# Post-build message
message(STATUS "Build Completed!")