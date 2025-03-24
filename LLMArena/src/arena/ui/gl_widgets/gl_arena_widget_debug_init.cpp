// src/arena/ui/gl_widgets/gl_arena_widget_debug_init.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include "../../../../include/arena/debug/console/debug_console.h"
#include <QDebug>

// Register the quintptr type for QVariant
Q_DECLARE_METATYPE(quintptr)

// This file handles initialization of debug-related components for the GLArenaWidget
void GLArenaWidget::initializeDebugSystem()
{
    try {
        // Create debug system with game scene and player controller
        m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
        
        // Get debug console and set render widget property
        DebugConsole* console = m_debugSystem->getConsole();
        if (console) {
            // Store widget pointer as quintptr to avoid direct pointer storage
            console->setProperty("render_widget", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(this)));
        }
        
        // Initialize the debug system
        m_debugSystem->initialize();
        
        qDebug() << "Debug system initialized successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to initialize debug system:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception initializing debug system";
    }
}