// src/arena/ui/gl_widgets/gl_arena_widget_hooks.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDebug>
#include <QCursor>

// This file contains input event hooks for GLArenaWidget

void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    // First, check if debug system wants to handle this
    if (m_debugSystem && processDebugKeyEvent(event)) {
        return;
    }
    
    // Handle inventory UI
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        return;
    }
    
    // Handle voxel placement
    if (event->key() == Qt::Key_E && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        placeVoxel();
        return;
    }
    
    // Handle voxel removal
    if (event->key() == Qt::Key_Q && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        removeVoxel();
        return;
    }
    
    // Handle inventory toggle
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        updateMouseTrackingState();
        return;
    }
    
    // Handle debug console toggle
    if (event->key() == Qt::Key_Escape) {
        bool consoleVisible = false;
        
        // Check if debug console is visible using isConsoleVisible method
        if (m_debugSystem) {
            QMetaObject::invokeMethod(m_debugSystem.get(), "isConsoleVisible", 
                                     Qt::DirectConnection,
                                     Q_RETURN_ARG(bool, consoleVisible));
            
            if (consoleVisible) {
                toggleDebugConsole();
                return;
            }
        }
        
        // If inventory is open, close it
        if (m_inventoryUI && m_inventoryUI->isVisible()) {
            m_inventoryUI->setVisible(false);
            updateMouseTrackingState();
            return;
        }
    }
    
    // Pass to player controller
    if (m_playerController) {
        m_playerController->handleKeyPress(event);
    }
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) {
    // Pass to player controller
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
}

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) {
    // Skip if not initialized
    if (!m_initialized) {
        return;
    }
    
    // Handle inventory first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        return;
    }
    
    // Handle debug console
    bool consoleVisible = false;
    if (m_debugSystem) {
        // Check console visibility using isConsoleVisible method
        QMetaObject::invokeMethod(m_debugSystem.get(), "isConsoleVisible", 
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(bool, consoleVisible));
        
        if (consoleVisible) {
            // Debug console doesn't handle mouse moves, but we should skip player handling
            return;
        }
    }
    
    // Pass to player controller for look control
    if (m_playerController) {
        m_playerController->handleMouseMove(event);
    }
    
    // Reset cursor to center after handling movement
    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event) {
    // Handle inventory first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        return;
    }
    
    // Skip if debug console is visible
    bool consoleVisible = false;
    if (m_debugSystem) {
        // Check console visibility using isConsoleVisible method
        QMetaObject::invokeMethod(m_debugSystem.get(), "isConsoleVisible", 
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(bool, consoleVisible));
        
        if (consoleVisible) {
            // Debug console should handle this
            return;
        }
    }
    
    // Voxel interaction check
    if (event->button() == Qt::LeftButton && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        placeVoxel();
        return;
    }
    
    if (event->button() == Qt::RightButton && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        removeVoxel();
        return;
    }
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) {
    // Handle inventory first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        return;
    }
}

void GLArenaWidget::updateMouseTrackingState() {
    bool consoleVisible = false;
    
    // Check if debug console is visible using isConsoleVisible method
    if (m_debugSystem) {
        QMetaObject::invokeMethod(m_debugSystem.get(), "isConsoleVisible", 
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(bool, consoleVisible));
    }
    
    // Determine whether to hide cursor based on UI state
    bool hideCursor = m_initialized && 
                     (!m_inventoryUI || !m_inventoryUI->isVisible()) &&
                     (!m_debugSystem || !consoleVisible);
    
    if (hideCursor) {
        setCursor(Qt::BlankCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    
    // Update focus state
    if (hideCursor) {
        setFocus();
    }
}