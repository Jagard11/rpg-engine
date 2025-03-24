// src/arena/ui/gl_widgets/gl_arena_widget_debug.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include "../../../../include/arena/debug/console/debug_console.h"
#include "../../../../include/arena/debug/visualizers/frustum_visualizer.h"
#include <QDebug>

// This file contains the debug-related functionality for the GL Arena Widget
// Keeping debug code separate helps maintain modularity and keeps files under 300 lines

namespace {
    // Helper function to set QPainter target for debug console text rendering
    void setQPainterTarget(QObject* obj, QPaintDevice* device) {
        if (obj) {
            obj->setProperty("qpainter_target", QVariant::fromValue<QPaintDevice*>(device));
        }
    }
}

void GLArenaWidget::initializeDebugSystem() {
    try {
        qDebug() << "Initializing debug system...";
        
        // Create debug system if it doesn't exist
        if (!m_debugSystem) {
            m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
        }
        
        // Initialize the debug system
        m_debugSystem->initialize();
        
        // Set this widget as the render target for debug console
        setDebugRenderTarget(m_debugSystem->getConsole(), this);
        
        qDebug() << "Debug system initialized successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize debug system:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in debug system initialization";
    }
}

void GLArenaWidget::renderDebugOverlays() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Render debug visualizations and console
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
    } catch (const std::exception& e) {
        qWarning() << "Error rendering debug overlays:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error rendering debug overlays";
    }
}

bool GLArenaWidget::handleDebugKeyPress(QKeyEvent* event) {
    if (!m_debugSystem || !event) {
        return false;
    }
    
    try {
        // Pass key press to debug system
        return m_debugSystem->handleKeyPress(event->key(), event->text());
    } catch (const std::exception& e) {
        qWarning() << "Error handling debug key press:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "Unknown error handling debug key press";
        return false;
    }
}

void GLArenaWidget::toggleDebugVisualizer(int visualizerType) {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Currently we only have one visualizer (frustum)
        if (visualizerType == 0) { // FrustumVisualizer
            FrustumVisualizer* visualizer = m_debugSystem->getFrustumVisualizer();
            if (visualizer) {
                visualizer->setEnabled(!visualizer->isEnabled());
                qDebug() << "Frustum visualization" << (visualizer->isEnabled() ? "enabled" : "disabled");
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Error toggling debug visualizer:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error toggling debug visualizer";
    }
}

bool GLArenaWidget::isDebugConsoleVisible() const {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        return m_debugSystem->getConsole()->isVisible();
    } catch (...) {
        return false;
    }
}

bool GLArenaWidget::isDebugVisualizerEnabled(int visualizerType) const {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        // Currently we only have one visualizer (frustum)
        if (visualizerType == 0) { // FrustumVisualizer
            FrustumVisualizer* visualizer = m_debugSystem->getFrustumVisualizer();
            return visualizer ? visualizer->isEnabled() : false;
        }
        return false;
    } catch (...) {
        return false;
    }
}