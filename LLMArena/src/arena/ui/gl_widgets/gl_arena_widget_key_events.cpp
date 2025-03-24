// src/arena/ui/gl_widgets/gl_arena_widget_key_events.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../include/arena/debug/debug_system.h"
#include <QDebug>

void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    if (!event) {
        return;
    }
    
    // First, check if debug system wants to handle this event
    if (m_debugSystem && processDebugKeyEvent(event)) {
        return; // Debug system handled the event
    }
    
    // Check if inventory UI has priority (when visible)
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        return;
    }
    
    // Handle inventory toggle key (I)
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        
        // Update mouse tracking state based on inventory visibility
        updateMouseTrackingState();
        return;
    }
    
    // Handle voxel placement/removal
    if (m_voxelSystem && m_highlightedVoxelFace >= 0) {
        if (event->key() == Qt::Key_E) {
            placeVoxel();
            return;
        } else if (event->key() == Qt::Key_Q) {
            removeVoxel();
            return;
        }
    }
    
    // Handle player controller key events
    if (m_playerController) {
        m_playerController->handleKeyPress(event);
    }
    
    // Default base class implementation
    QOpenGLWidget::keyPressEvent(event);
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) {
    if (!event) {
        return;
    }
    
    // Pass to player controller
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
    
    // Default implementation
    QOpenGLWidget::keyReleaseEvent(event);
}

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!event) {
        return;
    }
    
    // Check if inventory UI has priority
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        return;
    }
    
    // Check if debug console is visible - don't process mouse movement for camera
    if (m_debugSystem && isDebugConsoleVisible()) {
        return;
    }
    
    // Pass to player controller for camera movement
    if (m_playerController && hasFocus()) {
        m_playerController->handleMouseMove(event);
        
        // Recenter cursor
        if (hasFocus()) {
            QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
        }
    }
    
    // Default implementation
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event) {
    if (!event) {
        return;
    }
    
    // Check if inventory UI has priority
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        return;
    }
    
    // Make sure focus is set
    setFocus();
    
    // Default implementation
    QOpenGLWidget::mousePressEvent(event);
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!event) {
        return;
    }
    
    // Check if inventory UI has priority
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        return;
    }
    
    // Default implementation
    QOpenGLWidget::mouseReleaseEvent(event);
}