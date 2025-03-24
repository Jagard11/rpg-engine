// src/arena/ui/gl_widgets/gl_arena_widget_hooks.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include "../../../../include/arena/debug/console/debug_console.h"
#include <QDebug>

// OpenGL paint event - overrides QOpenGLWidget::paintGL()
void GLArenaWidget::paintGL() 
{
    if (!m_initialized) {
        // Skip rendering if not initialized
        return;
    }
    
    try {
        // Clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render floor, walls, and grid
        renderFloor();
        renderWalls();
        renderGrid();
        
        // Render voxels
        if (m_voxelSystem) {
            m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
        }
        
        // Render characters
        renderCharacters();
        
        // Render voxel highlight if needed
        renderVoxelHighlight();
        
        // Render inventory UI if needed
        renderInventory();
        
        // Render debug system if available
        if (m_debugSystem) {
            renderDebugSystem();
        }
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in paintGL:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in paintGL";
    }
}

// Handle key press events - overrides QWidget::keyPressEvent()
void GLArenaWidget::keyPressEvent(QKeyEvent* event) 
{
    // Check if debug system wants to handle this event
    if (processDebugKeyEvent(event)) {
        return;
    }
    
    // Handle inventory UI key events
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        update(); // Trigger repaint
        return;
    }
    
    // Pass event to player controller
    if (m_playerController) {
        // Handle player movement keys
        m_playerController->handleKeyPress(event);
    }
    
    // Handle other keys
    switch (event->key()) {
        case Qt::Key_Escape:
            // Close inventory if open, or show exit dialog
            if (m_inventoryUI && m_inventoryUI->isVisible()) {
                m_inventoryUI->setVisible(false);
            } else {
                QWidget::keyPressEvent(event);
            }
            break;
            
        case Qt::Key_I:
            // Toggle inventory
            if (m_inventoryUI) {
                m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
            }
            break;
            
        case Qt::Key_F:
            // Toggle frustum visualization
            toggleFrustumVisualization();
            break;
            
        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

// Handle key release events - overrides QWidget::keyReleaseEvent()
void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) 
{
    // Pass event to player controller
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
    
    QWidget::keyReleaseEvent(event);
}

// Handle mouse move events - overrides QWidget::mouseMoveEvent()
void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) 
{
    // Check if inventory UI is handling the mouse
    if (m_inventoryUI && 
        m_inventoryUI->isVisible() && 
        m_inventoryUI->isMouseOverUI(event->x(), event->y())) {
        
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        update(); // Trigger repaint
        return;
    }
    
    // Check if debug console is active - don't handle camera movement if so
    if (m_debugSystem && m_debugSystem->getConsole() && m_debugSystem->getConsole()->isVisible()) {
        return;
    }
    
    // Pass mouse movement to player controller if not in inventory and not in debug console
    if (m_playerController && 
        (!m_inventoryUI || !m_inventoryUI->isVisible()) && 
        (!m_debugSystem || !m_debugSystem->getConsole() || !m_debugSystem->getConsole()->isVisible())) {
        
        // Create a new mouse event with the event data
        QMouseEvent mouseEvent(QEvent::MouseMove, 
                              event->pos(), 
                              event->button(), 
                              event->buttons(), 
                              event->modifiers());
        
        m_playerController->handleMouseMove(&mouseEvent);
    }
}

// Handle mouse press events - overrides QWidget::mousePressEvent()
void GLArenaWidget::mousePressEvent(QMouseEvent* event) 
{
    // Check if inventory UI is handling the mouse
    if (m_inventoryUI && 
        m_inventoryUI->isVisible() && 
        m_inventoryUI->isMouseOverUI(event->x(), event->y())) {
        
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        update(); // Trigger repaint
        return;
    }
    
    // Check if debug console is active
    if (m_debugSystem && m_debugSystem->getConsole() && m_debugSystem->getConsole()->isVisible()) {
        return;
    }
    
    // Handle voxel placement/removal
    if (event->button() == Qt::LeftButton) {
        placeVoxel();
    } else if (event->button() == Qt::RightButton) {
        removeVoxel();
    }
    
    // Call parent implementation
    QOpenGLWidget::mousePressEvent(event);
}

// Handle mouse release events - overrides QWidget::mouseReleaseEvent()
void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) 
{
    // Check if inventory UI is handling the mouse
    if (m_inventoryUI && 
        m_inventoryUI->isVisible()) {
        
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        update(); // Trigger repaint
        return;
    }
    
    // Call parent implementation
    QOpenGLWidget::mouseReleaseEvent(event);
}

// Update mouse tracking state - called when focus or window state changes
void GLArenaWidget::updateMouseTrackingState() 
{
    // Update cursor depending on whether inventory or debug console is open
    if ((!m_inventoryUI || !m_inventoryUI->isVisible()) &&
        (!m_debugSystem || !m_debugSystem->getConsole() || !m_debugSystem->getConsole()->isVisible())) {
        
        // Hide cursor for gameplay
        if (isActiveWindow()) {
            setCursor(Qt::BlankCursor);
        }
    } else {
        // Show cursor for UI
        setCursor(Qt::ArrowCursor);
    }
}