// src/arena/ui/gl_widgets/gl_arena_widget_debug.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QVariant>
#include <QDebug>

// Initialize the debug system
void GLArenaWidget::initializeDebugSystem() {
    // Check that necessary components exist before creating debug system
    if (!m_gameScene || !m_playerController) {
        qWarning() << "Cannot initialize debug system: missing required components";
        return;
    }
    
    try {
        // Log information
        qDebug() << "Creating debug system...";
        
        // Create debug system with careful null checks
        m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
        qDebug() << "Debug system created successfully";
        
        // Set the render widget immediately
        if (m_debugSystem) {
            // Initialize the debug system
            m_debugSystem->initialize();
            
            // Set the render widget for the debug system (for console overlay)
            quintptr thisPtr = reinterpret_cast<quintptr>(this);
            QVariant widgetProp = QVariant::fromValue(thisPtr);
            
            // Verify the widget pointer is valid
            if (thisPtr != 0 && widgetProp.isValid()) {
                qDebug() << "Setting console widget pointer to:" << thisPtr;
                m_debugSystem->setConsoleWidget(widgetProp);
            } else {
                qWarning() << "Failed to create valid widget pointer for debug console";
            }
            
            qDebug() << "Debug system initialized successfully";
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Failed to create debug system:" << e.what();
        m_debugSystem.reset(); // Ensure the unique_ptr is null
    }
    catch (...) {
        qWarning() << "Unknown exception creating debug system";
        m_debugSystem.reset(); // Ensure the unique_ptr is null
    }
}

// Render debug overlays (console, visualizers, etc.)
void GLArenaWidget::renderDebugSystem() {
    // Skip if debug system is not available or widget has no valid context
    if (!m_debugSystem || !isValid() || !context()) {
        return;
    }
    
    // Skip if not initialized yet
    if (!m_initialized) {
        return;
    }
    
    try {
        // Check if OpenGL is ready for rendering
        if (!QOpenGLContext::currentContext()) {
            return;
        }
        
        // Always set the console widget before rendering
        // This ensures the widget pointer is up-to-date
        quintptr thisPtr = reinterpret_cast<quintptr>(this);
        QVariant widgetProp = QVariant::fromValue(thisPtr);
        
        if (thisPtr != 0 && widgetProp.isValid()) {
            m_debugSystem->setConsoleWidget(widgetProp);
        }
        
        // Render debug system components with valid matrices
        if (m_viewMatrix.isIdentity() && m_projectionMatrix.isIdentity()) {
            // Don't render with identity matrices - likely not properly set up yet
            return;
        }
        
        // Save OpenGL state
        glFinish();  // Make sure all GL commands are executed
        
        // Render the debug system
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
        
        // Force update to ensure the debug console is drawn
        if (m_debugSystem->isConsoleVisible()) {
            update();
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception rendering debug system:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception rendering debug system";
    }
}

// Handle debug key presses
bool GLArenaWidget::processDebugKeyEvent(QKeyEvent* event) {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        // Check specifically for backtick/tilde key for toggling console
        if (event->key() == Qt::Key_QuoteLeft || event->key() == Qt::Key_AsciiTilde) {
            qDebug() << "Backtick/tilde key detected - toggling debug console";
            toggleDebugConsole();
            return true;
        }
        
        // Pass key event to debug system
        return m_debugSystem->handleKeyPress(event->key(), event->text());
    }
    catch (const std::exception& e) {
        qWarning() << "Exception processing debug key event:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception processing debug key event";
    }
    
    return false;
}

// Toggle debug console visibility
void GLArenaWidget::toggleDebugConsole() {
    if (!m_debugSystem) {
        qWarning() << "Cannot toggle debug console: debug system not available";
        return;
    }
    
    try {
        // Always update the widget pointer before toggling
        quintptr thisPtr = reinterpret_cast<quintptr>(this);
        QVariant widgetProp = QVariant::fromValue(thisPtr);
        
        if (thisPtr != 0 && widgetProp.isValid()) {
            m_debugSystem->setConsoleWidget(widgetProp);
        }
        
        m_debugSystem->toggleConsoleVisibility();
        qDebug() << "Debug console toggled, new visible state:" << m_debugSystem->isConsoleVisible();
        update(); // Trigger redraw
    }
    catch (const std::exception& e) {
        qWarning() << "Exception toggling debug console:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception toggling debug console";
    }
}

// Check if debug console is visible
bool GLArenaWidget::isDebugConsoleVisible() const {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        return m_debugSystem->isConsoleVisible();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception checking debug console visibility:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception checking debug console visibility";
    }
    
    return false;
}

// Toggle frustum visualization
void GLArenaWidget::toggleFrustumVisualization() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        m_debugSystem->toggleFrustumVisualization();
        update(); // Trigger redraw
    }
    catch (const std::exception& e) {
        qWarning() << "Exception toggling frustum visualization:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception toggling frustum visualization";
    }
}