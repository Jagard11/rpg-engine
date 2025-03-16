# CMakeLists.txt - Updated for OpenGL rendering
cmake_minimum_required(VERSION 3.14)
project(OobaboogaRPGArena LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Improved Qt5 detection
# First try to find Qt5 with default config
find_package(Qt5 COMPONENTS Core Widgets Network OpenGL Gui QUIET)

# If Qt5 wasn't found, try common locations
if(NOT Qt5_FOUND)
    message(STATUS "Qt5 not found with default search. Trying common locations...")
    
    # Common installation paths
    set(QT_PATH_HINTS
        "/usr/local/Qt-5.15.2"
        "/usr/local/Qt5"
        "/opt/Qt5"
        "/opt/Qt"
        "/usr/lib/qt5"
        "/usr/lib/x86_64-linux-gnu/qt5"
        "/usr/lib/qt"
        "$ENV{HOME}/Qt/5.15.2/gcc_64"
        "$ENV{HOME}/Qt5"
    )
    
    # Try each path
    foreach(QT_PATH ${QT_PATH_HINTS})
        if(EXISTS "${QT_PATH}")
            message(STATUS "Trying Qt in: ${QT_PATH}")
            set(CMAKE_PREFIX_PATH "${QT_PATH}")
            find_package(Qt5 COMPONENTS Core Widgets Network OpenGL Gui QUIET)
            if(Qt5_FOUND)
                break()
            endif()
        endif()
    endforeach()
endif()

# Final check if Qt was found
if(NOT Qt5_FOUND)
    message(FATAL_ERROR "
    Qt5 not found. Please install Qt5 or set the correct path.
    
    On Ubuntu/Debian: sudo apt-get install qtbase5-dev libqt5opengl5-dev
    On Fedora: sudo dnf install qt5-qtbase-devel qt5-qtbase-private-devel
    
    If Qt5 is installed in a custom location, set CMAKE_PREFIX_PATH:
    cmake -DCMAKE_PREFIX_PATH=/path/to/qt5 ..
    ")
endif()

# Make sure MOC is run on our header files
set(CMAKE_AUTOMOC ON)

# Source files
set(SOURCES
    src/main.cpp
)

# Check if source files exist before adding them
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/character_manager.cpp")
    list(APPEND SOURCES src/character_manager.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/character_editor.cpp")
    list(APPEND SOURCES src/character_editor.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/memory_system.cpp")
    list(APPEND SOURCES src/memory_system.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/oobabooga_bridge.cpp")
    list(APPEND SOURCES src/oobabooga_bridge.cpp)
endif()

# Add OpenGL rendering files
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/gl_arena_widget.cpp")
    list(APPEND SOURCES src/gl_arena_widget.cpp)
endif()

# We don't need the signals implementation file - MOC handles signal implementations
# The file is causing duplicate symbol definitions
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/gl_arena_widget_signals.cpp")
#     list(APPEND SOURCES src/gl_arena_widget_signals.cpp)
# endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/arena_view.cpp")
    list(APPEND SOURCES src/arena_view.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/game_scene.cpp")
    list(APPEND SOURCES src/game_scene.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/player_controller.cpp")
    list(APPEND SOURCES src/player_controller.cpp)
endif()

# Add moc_registration.cpp for Qt meta-object handling (signals/slots)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/moc_registration.cpp")
    list(APPEND SOURCES src/moc_registration.cpp)
endif()

# Include directories
include_directories(include)

# Check for resources directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    # Resource files
    set(RESOURCES
        resources.qrc
    )

    # Generate resource file
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

    # Create shader directory and files if they don't exist
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    # Basic vertex shader
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

    # Basic fragment shader
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
        // Ambient lighting
        float ambientStrength = 0.3;
        vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
        
        // Diffuse lighting
        vec3 norm = normalize(fragNormal);
        vec3 lightDir = normalize(lightPos - fragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
        
        // Specular lighting
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
        
        vec3 result = (ambient + diffuse + specular) * objectColor;
        fragColor = vec4(result, 1.0);
    }
    ")

    # Billboard vertex shader
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/billboard.vert
    "#version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec2 texCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform vec3 cameraRight;
    uniform vec3 cameraUp;
    uniform vec3 billboardPos;
    uniform vec2 billboardSize;
    out vec2 fragTexCoord;
    void main() {
        vec3 vertPos = billboardPos + cameraRight * position.x * billboardSize.x + cameraUp * position.y * billboardSize.y;
        gl_Position = projection * view * vec4(vertPos, 1.0);
        fragTexCoord = texCoord;
    }
    ")

    # Billboard fragment shader
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/billboard.frag
    "#version 330 core
    in vec2 fragTexCoord;
    uniform sampler2D textureSampler;
    out vec4 fragColor;
    void main() {
        vec4 texColor = texture(textureSampler, fragTexCoord);
        if(texColor.a < 0.1) discard;
        fragColor = texColor;
    }
    ")
    
    # Add the executable with resources
    add_executable(${PROJECT_NAME} 
        ${SOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc
    )
else()
    # Add the executable without resources
    add_executable(${PROJECT_NAME} 
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
    ${OPENGL_LIBRARIES}
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# If resources exist, install them
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/
            DESTINATION share/${PROJECT_NAME})
endif()