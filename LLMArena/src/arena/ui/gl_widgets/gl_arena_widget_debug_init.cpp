// src/arena/ui/gl_widgets/gl_arena_widget_debug_init.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDebug>

// This file contains initialization methods for the debug system in GLArenaWidget

void GLArenaWidget::initializeDebugSystem() {
    // Create and initialize debug system
    try {
        // Create new debug system instance
        m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
        
        // Initialize the system
        if (m_debugSystem) {
            m_debugSystem->initialize();
            qDebug() << "Debug system initialized successfully";
        } else {
            qWarning() << "Failed to create debug system instance";
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception initializing debug system:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception initializing debug system";
    }
}

