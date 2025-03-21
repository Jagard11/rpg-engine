# CMakeLists.txt - Oobabooga RPG Arena Project
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/models
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/manager
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llm
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arena/game
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arena/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arena/player
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arena/skybox
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arena/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arena/voxels
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Define the main source file
set(MAIN_SOURCE src/main.cpp)

# Arena Module Sources and Headers
set(ARENA_CORE_SOURCES
    src/arena/core/arena_core.cpp
    src/arena/core/arena_render.cpp
    src/arena/core/rendering/arena_renderer_characters.cpp
    src/arena/core/rendering/arena_renderer_scene.cpp
    src/arena/core/rendering/arena_renderer_stubs.cpp
    src/arena/core/rendering/arena_renderer_webgl.cpp
)

set(ARENA_GAME_SOURCES
    src/arena/game/game_scene.cpp
    src/arena/game/inventory.cpp
    src/arena/game/player_controller_core.cpp
    src/arena/game/player_controller_input.cpp
    src/arena/game/player_controller_movement.cpp
    src/arena/game/player_controller_stance.cpp
)

set(ARENA_PLAYER_SOURCES
    src/arena/player/controls/camera.cpp
    src/arena/player/controls/crouching.cpp
    src/arena/player/controls/movement.cpp
    src/arena/player/inventory/inventory_core.cpp
    src/arena/player/player_core.cpp
    src/arena/player/ui/inventory_ui.cpp
)

set(ARENA_SKYBOX_SOURCES
    src/arena/skybox/astronomy/astronomy_calc.cpp
    src/arena/skybox/astronomy/sun_position.cpp
    src/arena/skybox/skybox_color.cpp
    src/arena/skybox/skybox_core.cpp
    src/arena/skybox/skybox_geometry.cpp
    src/arena/skybox/skybox_render.cpp
    src/arena/skybox/skybox_shaders.cpp
)

set(ARENA_UI_SOURCES
    src/arena/ui/gl_widgets/gl_arena_character.cpp
    src/arena/ui/gl_widgets/gl_arena_direct_rendering.cpp
    src/arena/ui/gl_widgets/gl_arena_highlighting.cpp
    src/arena/ui/gl_widgets/gl_arena_inventory.cpp
    src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
    src/arena/ui/gl_widgets/gl_arena_widget_geometry.cpp
    src/arena/ui/gl_widgets/gl_arena_widget_input.cpp
    src/arena/ui/gl_widgets/gl_arena_widget_rendering.cpp
    src/arena/ui/gl_widgets/gl_arena_widget_shaders.cpp
    src/arena/ui/gl_widgets/gl_arena_widget_stub.cpp
    src/arena/ui/views/arena_view.cpp
    src/arena/ui/voxel_highlight_renderer.cpp
)

set(ARENA_VOXELS_SOURCES
    src/arena/voxels/voxel_highlight.cpp
    src/arena/voxels/voxel_render.cpp
    src/arena/voxels/voxel_system_integration.cpp
    src/arena/voxels/voxel_world.cpp
)

# Character Module Sources and Headers
set(CHARACTER_CORE_SOURCES
    src/character/core/character_core.cpp
    src/character/core/character_manager_context.cpp
    src/character/core/character_manager_memory.cpp
    src/character/core/character_manager_personality.cpp
)

set(CHARACTER_EDITOR_SOURCES
    src/character/editor/appearance_page.cpp
    src/character/editor/basic_info_page.cpp
    src/character/editor/editor_core.cpp
    src/character/editor/memories_page.cpp
    src/character/editor/personality_page.cpp
)

set(CHARACTER_MANAGER_SOURCES
    src/character/manager/character_manager_appearance.cpp
    src/character/manager/character_manager_core.cpp
)

set(CHARACTER_MEMORY_SOURCES
    src/character/memory/memory_core.cpp
    src/character/memory/memory_retrieval.cpp
)

set(CHARACTER_UI_SOURCES
    src/character/ui/character_editor.cpp
    src/character/ui/character_editor_appearance.cpp
    src/character/ui/character_editor_basic.cpp
    src/character/ui/character_editor_memory.cpp
    src/character/ui/character_editor_personality.cpp
    src/character/ui/character_manager_dialog.cpp
)

# LLM Module Sources and Headers
set(LLM_SOURCES
    src/llm/api/api_caller.cpp
    src/llm/memory/memory_system.cpp
    src/llm/memory/memory_system_helpers.cpp
    src/llm/ui/oobabooga_bridge.cpp
)

# Splash Module Sources and Headers
set(SPLASH_SOURCES
    src/splash/dialogs/location_dialog.cpp
)

# Utils Module Sources and Headers
set(UTILS_SOURCES
    src/utils/opengl/opengl_debug.cpp
)

# Manually select only non-duplicate headers
set(SELECTED_HEADERS
    # Arena Core
    include/arena/core/rendering/arena_renderer.h
    
    # Arena Game - excluded duplicates
    # include/arena/game/game_scene.h  
    # include/arena/game/player_controller.h
    
    # Arena Player
    include/arena/player/inventory/inventory.h
    include/arena/player/inventory/inventory_ui.h
    # include/arena/player/player_core.h  # Duplicate
    # include/arena/player/ui/player_ui.h # Duplicate
    
    # Arena Skybox
    include/arena/skybox/astronomy/astronomy.h
    # include/arena/skybox/sky_system.h   # Duplicate
    include/arena/skybox/sky_system_helpers.h
    # include/arena/skybox/skybox_core.h  # Duplicate
    
    # Arena UI
    include/arena/ui/gl_widgets/gl_arena_widget.h
    include/arena/ui/views/arena_view.h
    include/arena/ui/voxel_highlight_renderer.h
    
    # Arena Voxels
    include/arena/voxels/types/voxel_types.h
    # include/arena/voxels/voxel_core.h   # Duplicate
    include/arena/voxels/voxel_renderer.h
    include/arena/voxels/voxel_system_integration.h
    # include/arena/voxels/voxel_world.h  # Duplicate
    # include/arena/voxels/voxel_placement.h # Duplicate
    
    # Character Core
    # include/character/core/character_core.h # Duplicate
    include/character/core/character_persistence.h
    
    # Character Editor
    include/character/editor/character_editor.h
    
    # Character Manager - excluded duplicates
    # include/character/manager/character_manager.h
    
    # Character Memory
    include/character/memory/memory_system.h
    
    # Character Models
    include/character/models/all_models.h
    include/character/models/appearance.h
    include/character/models/memory.h
    include/character/models/personality.h
    include/character/models/stats.h
    
    # Character UI
    include/character/ui/character_editor_ui.h
    
    # LLM Module
    # include/llm/api_core.h  # Duplicate
    include/llm/oobabooga_bridge.h
    
    # Splash Module
    include/splash/ui/location_dialog.h
    
    # Utils Module
    include/utils/crash_handler.h
    include/utils/error/error_handling.h
    include/utils/opengl/opengl_utils.h
    include/utils/opengl_debug.h
)

# Combine all sources
set(ALL_SOURCES 
    ${MAIN_SOURCE}
    ${ARENA_CORE_SOURCES}
    ${ARENA_GAME_SOURCES}
    ${ARENA_PLAYER_SOURCES}
    ${ARENA_SKYBOX_SOURCES}
    ${ARENA_UI_SOURCES}
    ${ARENA_VOXELS_SOURCES}
    ${CHARACTER_CORE_SOURCES}
    ${CHARACTER_EDITOR_SOURCES}
    ${CHARACTER_MANAGER_SOURCES}
    ${CHARACTER_MEMORY_SOURCES}
    ${CHARACTER_UI_SOURCES}
    ${LLM_SOURCES}
    ${SPLASH_SOURCES}
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
    ${SELECTED_HEADERS}
    ${ALL_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc
)

# Allow duplicate symbols for now (temporary fix for function redefinition issues during refactoring)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
endif()

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