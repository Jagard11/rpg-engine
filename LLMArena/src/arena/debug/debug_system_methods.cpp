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
    
    try {
        return m_console->isVisible();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception checking console visibility:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception checking console visibility";
    }
    
    return false;
}

void DebugSystem::toggleConsoleVisibility() {
    if (!m_console) {
        qWarning() << "Cannot toggle console visibility: console not available";
        return;
    }
    
    try {
        bool newState = !m_console->isVisible();
        m_console->setVisible(newState);
        qDebug() << "Console visibility toggled to:" << newState;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception toggling console visibility:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception toggling console visibility";
    }
}

void DebugSystem::toggleFrustumVisualization() {
    if (!m_frustumVisualizer) {
        qWarning() << "Cannot toggle frustum visualization: visualizer not available";
        return;
    }
    
    try {
        bool newState = !m_frustumVisualizer->isEnabled();
        m_frustumVisualizer->setEnabled(newState);
        qDebug() << "Frustum visualization toggled to:" << newState;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception toggling frustum visualization:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception toggling frustum visualization";
    }
}

void DebugSystem::setConsoleWidget(const QVariant& widget) {
    if (!m_console) {
        qWarning() << "Cannot set console widget: console not available";
        return;
    }
    
    try {
        // Validate widget pointer
        if (!widget.isValid()) {
            qWarning() << "Invalid widget pointer provided to setConsoleWidget";
            return;
        }
        
        quintptr ptr = widget.value<quintptr>();
        if (ptr == 0) {
            qWarning() << "Invalid null widget pointer in setConsoleWidget";
            return;
        }
        
        // Log successful widget pointer setting
        qDebug() << "Setting console widget pointer:" << ptr;
        
        // Set the render widget property for the console
        m_console->setProperty("render_widget", widget);
        // No need to call update() since DebugConsole is not a QWidget
        
        qDebug() << "Console widget set successfully";
    }
    catch (const std::exception& e) {
        qWarning() << "Exception setting console widget:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception setting console widget";
    }
}