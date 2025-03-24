// src/arena/ui/gl_widgets/gl_arena_widget_debug_methods.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDebug>
#include <QMetaObject>

// This file contains additional debug-related methods for GLArenaWidget

void GLArenaWidget::renderDebugSystem() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Create widget property for console rendering
        QVariant widgetProperty = QVariant::fromValue(quintptr(this));
        
        // Set the console widget through the debug system
        QMetaObject::invokeMethod(m_debugSystem.get(), "setConsoleWidget", 
                                 Qt::DirectConnection,
                                 Q_ARG(QVariant, widgetProperty));
        
        // Render debug system
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
    } catch (const std::exception& e) {
        qWarning() << "Exception in debug system rendering:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in debug system rendering";
    }
}

bool GLArenaWidget::processDebugKeyEvent(QKeyEvent* event) {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        // Pass key event to debug system
        return m_debugSystem->handleKeyPress(event->key(), event->text());
    } catch (const std::exception& e) {
        qWarning() << "Exception in debug key handling:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in debug key handling";
    }
    
    return false;
}

void GLArenaWidget::toggleDebugConsole() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Toggle console visibility through the debug system
        QMetaObject::invokeMethod(m_debugSystem.get(), "toggleConsoleVisibility", 
                                 Qt::DirectConnection);
        
        // Update mouse tracking state
        updateMouseTrackingState();
    } catch (const std::exception& e) {
        qWarning() << "Exception toggling debug console:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception toggling debug console";
    }
}

bool GLArenaWidget::isDebugConsoleVisible() const {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        // Get console visibility through the debug system
        bool visible = false;
        QMetaObject::invokeMethod(m_debugSystem.get(), "isConsoleVisible", 
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(bool, visible));
        return visible;
    } catch (const std::exception& e) {
        qWarning() << "Exception checking debug console visibility:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception checking debug console visibility";
    }
    
    return false;
}

void GLArenaWidget::toggleFrustumVisualization() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Toggle frustum visualization through the debug system
        QMetaObject::invokeMethod(m_debugSystem.get(), "toggleFrustumVisualization", 
                                 Qt::DirectConnection);
    } catch (const std::exception& e) {
        qWarning() << "Exception toggling frustum visualization:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception toggling frustum visualization";
    }
}