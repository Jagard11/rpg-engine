cmake_minimum_required(VERSION 3.10)
project(VoxelGame VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)

# Download stb_image
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# Add source files
set(SOURCES
    src/main.cpp
    src/core/Game.cpp
    src/core/Window.cpp
    src/core/StackTrace.cpp
    src/world/World.cpp
    src/world/Chunk.cpp
    src/world/WorldGenerator.cpp
    src/player/Player.cpp
    src/physics/CollisionSystem.cpp
    src/renderer/Renderer.cpp
    src/renderer/TextureManager.cpp
    src/ui/SplashScreen.cpp
    src/utils/Random.cpp
    src/render/TextRenderer.cpp
    src/debug/DebugMenu.cpp
    src/debug/DebugUtils.cpp
    src/stb_image_impl.cpp
    src/texture/TextureManager.cpp
)

# Add header files
set(HEADERS
    include/core/Game.hpp
    include/core/Window.hpp
    include/core/StackTrace.hpp
    include/world/World.hpp
    include/world/Chunk.hpp
    include/world/WorldGenerator.hpp
    include/player/Player.hpp
    include/physics/CollisionSystem.hpp
    include/renderer/Renderer.hpp
    include/renderer/TextureManager.hpp
    include/ui/SplashScreen.hpp
    include/utils/Random.hpp
    include/render/TextRenderer.hpp
    include/debug/DebugMenu.hpp
    include/debug/DebugUtils.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    ${GLM_LIBRARIES}
    Freetype::Freetype
    GLU # Add GLU library for gluLookAt
)

# Copy resources to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
) 