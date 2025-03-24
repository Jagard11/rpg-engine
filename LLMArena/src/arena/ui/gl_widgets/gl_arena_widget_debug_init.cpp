// src/arena/ui/gl_widgets/gl_arena_widget_debug_init.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../include/arena/debug/debug_system.h"
#include "../../../include/arena/debug/console/debug_console.h"

void GLArenaWidget::initializeDebugSystem() {
    // Create the debug system
    m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
    
    // Initialize debug system
    m_debugSystem->initialize();
    
    // Set this widget as the rendering target for the debug console
    // Store a pointer to this widget as a quintptr to avoid QPaintDevice* metatype issues
    DebugConsole* console = m_debugSystem->getConsole();
    if (console) {
        quintptr widgetPtr = reinterpret_cast<quintptr>(static_cast<void*>(this));
        console->setProperty("render_widget", QVariant::fromValue<quintptr>(widgetPtr));
    }
}

void GLArenaWidget::renderDebugSystem() {
    if (m_debugSystem) {
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
    }
}

bool GLArenaWidget::processDebugKeyEvent(QKeyEvent* event) {
    if (m_debugSystem) {
        return m_debugSystem->handleKeyPress(event->key(), event->text());
    }
    return false;
}

void GLArenaWidget::toggleDebugConsole() {
    if (m_debugSystem && m_debugSystem->getConsole()) {
        m_debugSystem->getConsole()->setVisible(!m_debugSystem->getConsole()->isVisible());
    }
}

bool GLArenaWidget::isDebugConsoleVisible() const {
    if (m_debugSystem && m_debugSystem->getConsole()) {
        return m_debugSystem->getConsole()->isVisible();
    }
    return false;
}

void GLArenaWidget::toggleFrustumVisualization() {
    if (m_debugSystem && m_debugSystem->getFrustumVisualizer()) {
        m_debugSystem->getFrustumVisualizer()->setEnabled(
            !m_debugSystem->getFrustumVisualizer()->isEnabled());
    }
}