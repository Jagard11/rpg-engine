// src/arena/ui/gl_widgets/gl_arena_widget_initialize.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>

// Initialize the arena with given parameters
void GLArenaWidget::initializeArena(double radius, double height) 
{
    try {
        // Store parameters
        m_arenaRadius = radius;
        m_wallHeight = height;
        
        // Create the arena geometry
        createArena(radius, height);
        
        // Create floor
        createFloor(radius);
        
        // Create grid
        createGrid(radius * 2, 20);
        
        // Initialize the player controller
        if (m_playerController) {
            m_playerController->createPlayerEntity();
            m_playerController->startUpdates();
        }
        
        // Initialize voxel system if available
        if (m_voxelSystem) {
            m_voxelSystem->createDefaultWorld();
        }
        
        // Mark as initialized
        m_initialized = true;
        
        qDebug() << "Arena initialized with radius" << radius << "and height" << height;
    }
    catch (const std::exception& e) {
        qCritical() << "Exception initializing arena:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception initializing arena";
    }
}