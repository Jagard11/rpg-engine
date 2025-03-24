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
        
        // Delay further initialization until after construction is complete
        QTimer::singleShot(100, this, [this]() {
            try {
                if (m_debugSystem) {
                    // Initialize the debug system
                    m_debugSystem->initialize();
                    
                    // Set the render widget for the debug system (for console overlay)
                    // Use a QVariant to store the widget pointer
                    quintptr thisPtr = reinterpret_cast<quintptr>(this);
                    m_debugSystem->setConsoleWidget(QVariant::fromValue(thisPtr));
                    
                    qDebug() << "Debug system initialized successfully";
                }
            }
            catch (const std::exception& e) {
                qWarning() << "Failed to initialize debug system components:" << e.what();
                m_debugSystem.reset(); // Clean up on failure
            }
            catch (...) {
                qWarning() << "Unknown exception initializing debug system components";
                m_debugSystem.reset(); // Clean up on failure
            }
        });
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
        
        // Render debug system components with valid matrices
        if (m_viewMatrix.isIdentity() && m_projectionMatrix.isIdentity()) {
            // Don't render with identity matrices - likely not properly set up yet
            return;
        }
        
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
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
        return;
    }
    
    try {
        m_debugSystem->toggleConsoleVisibility();
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