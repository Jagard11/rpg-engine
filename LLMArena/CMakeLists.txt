# CMakeLists.txt - Fixed for Qt MOC processing
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/character
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llm
    ${CMAKE_CURRENT_SOURCE_DIR}/include/game
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/memory
)

# Explicitly list all header files that contain Q_OBJECT macro
set(HEADERS
    include/ui/arena_view.h
    include/ui/character_editor_ui.h
    include/character/character_persistence.h
    include/llm/oobabooga_bridge.h
    include/game/game_scene.h
    include/game/player_controller.h
    include/rendering/arena_renderer.h
    include/rendering/gl_arena_widget.h
)

# Main source file
set(SOURCES
    src/main.cpp
)

# Character module
set(CHARACTER_SOURCES
    src/character/character_manager.cpp
)

# Memory module
set(MEMORY_SOURCES
    src/memory/memory_system.cpp
)

# UI module
set(UI_SOURCES
    src/ui/arena_view.cpp
    src/ui/character_editor.cpp
)

# LLM module
set(LLM_SOURCES
    src/llm/oobabooga_bridge.cpp
)

# Rendering module
set(RENDERING_SOURCES
    src/rendering/arena_renderer.cpp
    src/rendering/gl_arena_widget.cpp
    src/rendering/gl_arena_widget_signals.cpp
)

# Game module
set(GAME_SOURCES
    src/game/game_scene.cpp
    src/game/player_controller.cpp
)

# Combine all sources
list(APPEND SOURCES 
    ${CHARACTER_SOURCES}
    ${MEMORY_SOURCES}
    ${UI_SOURCES}
    ${LLM_SOURCES}
    ${RENDERING_SOURCES}
    ${GAME_SOURCES}
)

# Check for resources directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    # Create resources
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc 
    "<!DOCTYPE RCC>
    <RCC>
        <qresource prefix=\"/\">
            <file>shaders/basic.vert</file>
            <file>shaders/basic.frag</file>
            <file>shaders/billboard.vert</file>
            <file>shaders/billboard.frag</file>
        </qresource>
    </RCC>
    ")

    # Create shader directory and files
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    # Shader generation logic here (kept from previous version)
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
        fragColor = texColor;
    }
    ")
    
    # Add the executable with resources
    add_executable(${PROJECT_NAME} 
        ${HEADERS}
        ${SOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc
    )
else()
    # Add the executable without resources
    add_executable(${PROJECT_NAME} 
        ${HEADERS}
        ${SOURCES}
    )
endif()

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

# If resources exist, install them
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/
            DESTINATION share/${PROJECT_NAME})
endif()