// src/arena/ui/gl_widgets/gl_arena_widget_debug_init.cpp
#include <QDebug>
// Include wrapper header that ensures all necessary types are fully defined
#include "gl_arena_widget_debug_wrapper.h"

void GLArenaWidget::initializeDebugSystem() {
    try {
        // Create debug system with game scene and player controller
        m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
        
        // Initialize debug system
        m_debugSystem->initialize();
        
        // Set this widget as the render target for the debug console
        // This allows console to draw text on top of the OpenGL scene
        if (m_debugSystem->getConsole()) {
            // Store the widget pointer as a quintptr in a QVariant property
            quintptr widgetPtr = reinterpret_cast<quintptr>(this);
            m_debugSystem->getConsole()->setProperty("render_widget", QVariant::fromValue(widgetPtr));
        }
        
        qDebug() << "Debug system initialized successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to initialize debug system:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown error initializing debug system";
    }
}

void GLArenaWidget::renderDebugSystem() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Render debug visuals with current matrices and screen dimensions
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
    }
    catch (const std::exception& e) {
        qWarning() << "Error rendering debug system:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown error rendering debug system";
    }
}

bool GLArenaWidget::processDebugKeyEvent(QKeyEvent* event) {
    if (!m_debugSystem || !event) {
        return false;
    }
    
    try {
        // Pass key events to debug system
        QString text = event->text();
        return m_debugSystem->handleKeyPress(event->key(), text);
    }
    catch (const std::exception& e) {
        qWarning() << "Error processing debug key event:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown error processing debug key event";
    }
    
    return false;
}

void GLArenaWidget::toggleDebugConsole() {
    if (!m_debugSystem || !m_debugSystem->getConsole()) {
        return;
    }
    
    try {
        // Toggle console visibility
        bool currentState = m_debugSystem->getConsole()->isVisible();
        m_debugSystem->getConsole()->setVisible(!currentState);
        
        // Update mouse tracking state based on console visibility
        updateMouseTrackingState();
    }
    catch (const std::exception& e) {
        qWarning() << "Error toggling debug console:" << e.what();
    }
}

bool GLArenaWidget::isDebugConsoleVisible() const {
    if (!m_debugSystem || !m_debugSystem->getConsole()) {
        return false;
    }
    
    return m_debugSystem->getConsole()->isVisible();
}

void GLArenaWidget::toggleFrustumVisualization() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Get the visualizer pointer - check it here to avoid calling methods on nullptr
        FrustumVisualizer* visualizer = m_debugSystem->getFrustumVisualizer();
        if (!visualizer) {
            qWarning() << "Frustum visualizer is not available";
            return;
        }
        
        // Toggle frustum visualizer
        bool currentState = visualizer->isEnabled();
        visualizer->setEnabled(!currentState);
        
        qDebug() << "Frustum visualization" << (!currentState ? "enabled" : "disabled");
    }
    catch (const std::exception& e) {
        qWarning() << "Error toggling frustum visualization:" << e.what();
    }
}