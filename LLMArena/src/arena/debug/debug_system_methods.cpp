// src/arena/debug/debug_system_methods.cpp
#include "../../../include/arena/debug/debug_system.h"
#include "../../../include/arena/debug/console/debug_console.h"
#include "../../../include/arena/debug/visualizers/frustum_visualizer.h"
#include <QDebug>

// Implementation of the additional methods needed by the GLArenaWidget

bool DebugSystem::isConsoleVisible() const {
    if (!m_console) {
        return false;
    }
    
    return m_console->isVisible();
}

void DebugSystem::toggleConsoleVisibility() {
    if (!m_console) {
        return;
    }
    
    m_console->setVisible(!m_console->isVisible());
}

void DebugSystem::toggleFrustumVisualization() {
    if (!m_frustumVisualizer) {
        return;
    }
    
    m_frustumVisualizer->setEnabled(!m_frustumVisualizer->isEnabled());
}

void DebugSystem::setConsoleWidget(const QVariant& widget) {
    if (!m_console) {
        return;
    }
    
    try {
        // Set the render widget property for the console
        m_console->setProperty("render_widget", widget);
    } catch (const std::exception& e) {
        qWarning() << "Exception setting console widget:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception setting console widget";
    }
}