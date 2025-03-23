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

# Include directories for header files - organized by module
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Define the main source file
set(MAIN_SOURCE src/main.cpp)

# Function to collect source files from directory with exclusions
function(collect_sources VAR DIR)
    file(GLOB_RECURSE SOURCES "${DIR}/*.cpp")
    # Filter out duplicate implementation files
    list(FILTER SOURCES EXCLUDE REGEX "player_core\\.cpp$")
    set(${VAR} ${SOURCES} PARENT_SCOPE)
endfunction()

# Function to collect header files from directory with exclusions
function(collect_headers VAR DIR)
    file(GLOB_RECURSE HEADERS "${DIR}/*.h")
    # Filter out duplicate header files that cause MOC conflicts
    list(FILTER HEADERS EXCLUDE REGEX "player_core\\.h$")
    list(FILTER HEADERS EXCLUDE REGEX "character_core\\.h$")
    set(${VAR} ${HEADERS} PARENT_SCOPE)
endfunction()

# Collect sources by module
collect_sources(ARENA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/arena")
collect_sources(CHARACTER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/character")
collect_sources(LLM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/llm")
collect_sources(SPLASH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/splash")
collect_sources(UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils")
collect_sources(DEBUG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/arena/debug")

# Collect headers by module (only for IDE integration)
collect_headers(ARENA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/arena")
collect_headers(CHARACTER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/character")
collect_headers(LLM_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/llm")
collect_headers(SPLASH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/splash")
collect_headers(UTILS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/utils")
collect_headers(DEBUG_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/arena/debug")

# Combine all sources
set(ALL_SOURCES 
    ${MAIN_SOURCE}
    ${ARENA_SOURCES}
    ${CHARACTER_SOURCES}
    ${LLM_SOURCES}
    ${SPLASH_SOURCES}
    ${UTILS_SOURCES}
    ${DEBUG_SOURCES}
)

# For IDE organization, add headers to a separate source group
source_group("Arena Headers" FILES ${ARENA_HEADERS})
source_group("Character Headers" FILES ${CHARACTER_HEADERS})
source_group("LLM Headers" FILES ${LLM_HEADERS})
source_group("Splash Headers" FILES ${SPLASH_HEADERS})
source_group("Utils Headers" FILES ${UTILS_HEADERS})
source_group("Debug Headers" FILES ${DEBUG_HEADERS})

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
        <file>fonts/consolas.ttf</file>
    </qresource>
</RCC>
")

# Create font directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fonts)

# Generate consolas.ttf placeholder (this should be replaced with a real font)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fonts/consolas.ttf "placeholder")

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

# Define specific exclusion patterns for AUTOMOC processing
set(EXCLUDED_MOC_FILES
    "include/arena/player/player_core.h"
    "include/character/core/character_core.h"
)

# Create the executable with resources
add_executable(${PROJECT_NAME} 
    ${ALL_SOURCES}
    ${ARENA_HEADERS}
    ${CHARACTER_HEADERS}
    ${LLM_HEADERS}
    ${SPLASH_HEADERS}
    ${UTILS_HEADERS}
    ${DEBUG_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc
)

# Set specific files to be excluded from AUTOMOC
foreach(FILE ${EXCLUDED_MOC_FILES})
    set_property(SOURCE ${FILE} PROPERTY SKIP_AUTOMOC ON)
endforeach()

# Handle duplicate definitions - we need to be more aggressive
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Allow duplicate symbols for linker
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
endif()

# Force exclude problem files from MOC processing
set_property(GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP FALSE)
set(CMAKE_AUTOMOC_MACRO_NAMES "Q_OBJECT" "Q_GADGET" "Q_NAMESPACE")

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