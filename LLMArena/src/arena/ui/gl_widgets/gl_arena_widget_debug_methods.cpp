// src/arena/ui/gl_widgets/gl_arena_widget_debug.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include "../../../../include/arena/debug/console/debug_console.h"
#include "../../../../include/arena/debug/visualizers/frustum_visualizer.h"
#include <QDebug>

// Initialize the debug system
void GLArenaWidget::initializeDebugSystem() {
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

// Render debug overlays
void GLArenaWidget::renderDebugSystem() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Get current matrices for debug rendering
        QMatrix4x4 viewMatrix = m_viewMatrix;
        QMatrix4x4 projectionMatrix = m_projectionMatrix;
        
        // Render debug system with current viewport dimensions
        m_debugSystem->render(viewMatrix, projectionMatrix, width(), height());
    }
    catch (const std::exception& e) {
        qWarning() << "Error in debug overlay rendering:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown error in debug overlay rendering";
    }
}

// Handle debug key presses
bool GLArenaWidget::processDebugKeyEvent(QKeyEvent* event) {
    if (!m_debugSystem || !event) {
        return false;
    }
    
    try {
        // Handle key press with debug system
        return m_debugSystem->handleKeyPress(event->key(), event->text());
    }
    catch (const std::exception& e) {
        qWarning() << "Error handling debug key press:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "Unknown error handling debug key press";
        return false;
    }
}

// Toggle debug console
void GLArenaWidget::toggleDebugConsole() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        DebugConsole* console = m_debugSystem->getConsole();
        if (console) {
            console->setVisible(!console->isVisible());
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Error toggling debug console:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown error toggling debug console";
    }
}

// Check if debug console is visible
bool GLArenaWidget::isDebugConsoleVisible() const {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        DebugConsole* console = m_debugSystem->getConsole();
        return console ? console->isVisible() : false;
    }
    catch (const std::exception& e) {
        qWarning() << "Error checking debug console visibility:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "Unknown error checking debug console visibility";
        return false;
    }
}

// Toggle frustum visualization
void GLArenaWidget::toggleFrustumVisualization() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        FrustumVisualizer* visualizer = m_debugSystem->getFrustumVisualizer();
        if (visualizer) {
            visualizer->setEnabled(!visualizer->isEnabled());
            qDebug() << "Frustum visualizer" << (visualizer->isEnabled() ? "enabled" : "disabled");
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Error toggling frustum visualization:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown error toggling frustum visualization";
    }
}