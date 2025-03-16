# CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(OobaboogaRPGArena LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Improved Qt5 detection
# First try to find Qt5 with default config
find_package(Qt5 COMPONENTS Core Widgets WebEngineWidgets WebChannel Network QUIET)

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
            find_package(Qt5 COMPONENTS Core Widgets WebEngineWidgets WebChannel Network QUIET)
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
    
    On Ubuntu/Debian: sudo apt-get install qtbase5-dev qtwebengine5-dev libqt5webchannel5-dev
    On Fedora: sudo dnf install qt5-qtbase-devel qt5-qtwebengine-devel qt5-qtwebchannel-devel
    
    If Qt5 is installed in a custom location, set CMAKE_PREFIX_PATH:
    cmake -DCMAKE_PREFIX_PATH=/path/to/qt5 ..
    ")
endif()

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
            <file>rpg-mechanics.js</file>
            <file>3d-arena.js</file>
            <file>integration-example.html</file>
        </qresource>
    </RCC>
    ")

    # Copy resources if they exist
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/rpg-mechanics.js")
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/rpg-mechanics.js
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/3d-arena.js")
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/3d-arena.js
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/integration-example.html")
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/integration-example.html
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    
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

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::WebEngineWidgets
    Qt5::WebChannel
    Qt5::Network
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# If resources exist, install them
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/
            DESTINATION share/${PROJECT_NAME})
endif()