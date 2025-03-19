# CMakeLists.txt - Modular Arena Renderer Structure
cmake_minimum_required(VERSION 3.14)
project(OobaboogaRPGArena LANGUAGES CXX)

# Ensure CMake handles Qt properly
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the required Qt packages
find_package(Qt5 COMPONENTS 
    Core 
    Widgets 
    Network 
    OpenGL 
    Gui 
    WebEngine 
    WebEngineWidgets 
    REQUIRED
)

# Include directories for header files
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/models
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/manager
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llm
    ${CMAKE_CURRENT_SOURCE_DIR}/include/game
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/include/voxel
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/include/debug  # Keep for backward compatibility
)

# List all header files that contain Q_OBJECT macro
set(HEADERS
    # Character Module
    include/character/manager/character_manager.h
    include/character/ui/character_editor.h
    include/character/ui/character_editor_ui.h
    
    # LLM Module
    include/llm/oobabooga_bridge.h
    
    # Game Module
    include/game/game_scene.h
    include/game/player_controller.h
    include/game/inventory.h
    
    # Rendering Module
    include/rendering/arena_renderer.h
    include/rendering/gl_arena_widget.h
    include/rendering/voxel_highlight_renderer.h
    
    # UI Module
    include/ui/arena_view.h
    include/ui/location_dialog.h
    include/ui/inventory_ui.h
    
    # Voxel Module
    include/voxel/voxel_world.h
    include/voxel/voxel_renderer.h
    include/voxel/sky_system.h
    include/voxel/voxel_system_integration.h
    
    # Utils Module
    include/utils/crash_handler.h
)

# Main source file
set(SOURCES
    src/main.cpp
)

# Character Module sources
set(CHARACTER_SOURCES
    # Manager
    src/character/manager/character_manager_core.cpp
    src/character/manager/character_manager_appearance.cpp
    src/character/manager/character_manager_personality.cpp
    src/character/manager/character_manager_memory.cpp
    src/character/manager/character_manager_context.cpp
    
    # UI
    src/character/ui/character_editor.cpp
    src/character/ui/character_editor_basic.cpp
    src/character/ui/character_editor_appearance.cpp
    src/character/ui/character_editor_personality.cpp
    src/character/ui/character_editor_memory.cpp
    src/character/ui/character_manager_dialog.cpp
)

# Memory Module sources
set(MEMORY_SOURCES
    src/memory/memory_system.cpp
    src/memory/memory_system_helpers.cpp
)

# UI Module sources
set(UI_SOURCES
    src/ui/arena_view.cpp
    src/ui/location_dialog.cpp
    src/ui/inventory_ui.cpp
)

# LLM Module sources
set(LLM_SOURCES
    src/llm/oobabooga_bridge.cpp
)

# Rendering Module sources
set(RENDERING_SOURCES
    # GL Arena Widget
    src/rendering/gl_arena/gl_arena_widget_core.cpp
    src/rendering/gl_arena/gl_arena_character.cpp
    src/rendering/gl_arena/gl_arena_widget_rendering.cpp
    src/rendering/gl_arena/gl_arena_widget_shaders.cpp
    src/rendering/gl_arena/gl_arena_widget_geometry.cpp
    src/rendering/gl_arena/gl_arena_widget_input.cpp
    src/rendering/gl_arena/gl_arena_widget_stub.cpp
    src/rendering/gl_arena/gl_arena_direct_rendering.cpp
    src/rendering/gl_arena/gl_arena_highlighting.cpp
    src/rendering/gl_arena/gl_arena_inventory.cpp
    
    # Arena Renderer
    src/rendering/arena_renderer/arena_renderer_core.cpp
    src/rendering/arena_renderer/arena_renderer_webgl.cpp
    src/rendering/arena_renderer/arena_renderer_scene.cpp
    src/rendering/arena_renderer/arena_renderer_characters.cpp
    src/rendering/arena_renderer/arena_renderer_stubs.cpp
    
    # Voxel Highlighting
    src/rendering/voxel_highlight_renderer.cpp
)

# Game Module sources
set(GAME_SOURCES
    src/game/game_scene.cpp
    src/game/player_controller_core.cpp
    src/game/player_controller_input.cpp
    src/game/player_controller_movement.cpp
    src/game/player_controller_stance.cpp
    src/game/inventory.cpp
)

# Voxel Module sources
set(VOXEL_SOURCES
    src/voxel/voxel_world.cpp
    src/voxel/voxel_renderer.cpp
    src/voxel/sky_system.cpp
    src/voxel/sky_system_shaders.cpp
    src/voxel/sky_system_geometry.cpp
    src/voxel/sky_system_render.cpp
    src/voxel/sky_basic_calculations.cpp
    src/voxel/sky_astronomical.cpp
    src/voxel/sky_astronomical_calculations.cpp
    src/voxel/voxel_system_integration.cpp
)

# Utils Module sources
set(UTILS_SOURCES
    src/utils/opengl_debug.cpp
)

# Combine all sources
list(APPEND SOURCES 
    ${CHARACTER_SOURCES}
    ${MEMORY_SOURCES}
    ${UI_SOURCES}
    ${LLM_SOURCES}
    ${RENDERING_SOURCES}
    ${GAME_SOURCES}
    ${VOXEL_SOURCES}
    ${UTILS_SOURCES}
)

# Create resources directory if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()

# Ensure resources directory exists in build dir
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")

# Define texture files
set(TEXTURE_FILES
    "cobblestone.png"
    "grass.png"
    "dirt.png"
)

# Copy each texture file to the build directory
foreach(TEXTURE_FILE ${TEXTURE_FILES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/${TEXTURE_FILE}")
        message(STATUS "Copying texture: ${TEXTURE_FILE}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/${TEXTURE_FILE}" 
             DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/resources")
    else()
        message(WARNING "Texture file not found: ${TEXTURE_FILE}")
    endif()
endforeach()

# Generate resource file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc 
"<!DOCTYPE RCC>
<RCC>
    <qresource prefix=\"/\">
        <file>shaders/basic.vert</file>
        <file>shaders/basic.frag</file>
        <file>shaders/billboard.vert</file>
        <file>shaders/billboard.frag</file>
        <file>shaders/highlight.vert</file>
        <file>shaders/highlight.frag</file>
        <file>resources/cobblestone.png</file>
        <file>resources/grass.png</file>
        <file>resources/dirt.png</file>
    </qresource>
</RCC>
")

# Create shader directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# Generate shader files
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/basic.vert
"#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 fragPos;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragNormal = mat3(transpose(inverse(model))) * normal;
    fragTexCoord = texCoord;
    fragPos = vec3(model * vec4(position, 1.0));
}
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/basic.frag
"#version 330 core
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec3 fragPos;
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
out vec4 fragColor;
void main() {
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    fragColor = vec4(result, 1.0);
}
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/billboard.vert
"#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform vec3 billboardPos;
uniform vec2 billboardSize;
out vec2 fragTexCoord;
void main() {
    vec3 vertPos = billboardPos;
    vertPos += cameraRight * position.x * billboardSize.x;
    vertPos += cameraUp * position.y * billboardSize.y;
    gl_Position = projection * view * vec4(vertPos, 1.0);
    fragTexCoord = texCoord;
}
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/billboard.frag
"#version 330 core
in vec2 fragTexCoord;
uniform sampler2D textureSampler;
out vec4 fragColor;
void main() {
    vec4 texColor = texture(textureSampler, fragTexCoord);
    if (texColor.a < 0.01) discard;
    fragColor = texColor;
}
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/highlight.vert
"#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
}
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/highlight.frag
"#version 330 core
uniform vec4 highlightColor;
out vec4 fragColor;
void main() {
    fragColor = highlightColor;
}
")

# Create the executable with resources
add_executable(${PROJECT_NAME} 
    ${HEADERS}
    ${SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Link Qt and OpenGL libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::OpenGL
    Qt5::Gui
    Qt5::WebEngine
    Qt5::WebEngineWidgets
    ${OPENGL_LIBRARIES}
)

# Force the build system to regenerate Qt moc files
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC TRUE
    AUTOMOC_MOC_OPTIONS "--no-notes"
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Install resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/
        DESTINATION share/${PROJECT_NAME})