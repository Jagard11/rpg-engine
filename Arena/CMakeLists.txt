cmake_minimum_required(VERSION 3.10)
project(VoxelGame VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

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
    src/world/World.cpp
    src/world/Chunk.cpp
    src/world/WorldGenerator.cpp
    src/player/Player.cpp
    src/physics/CollisionSystem.cpp
    src/renderer/Renderer.cpp
    src/renderer/TextureManager.cpp
    src/ui/SplashScreen.cpp
    src/utils/Random.cpp
)

# Add header files
set(HEADERS
    include/core/Game.hpp
    include/core/Window.hpp
    include/world/World.hpp
    include/world/Chunk.hpp
    include/world/WorldGenerator.hpp
    include/player/Player.hpp
    include/physics/CollisionSystem.hpp
    include/renderer/Renderer.hpp
    include/renderer/TextureManager.hpp
    include/ui/SplashScreen.hpp
    include/utils/Random.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# STB implementation
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image_impl.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    ${GLM_LIBRARIES}
)

# Copy resources to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
) 