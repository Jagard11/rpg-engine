// src/arena/ui/gl_widgets/gl_arena_widget_key_events.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../include/arena/debug/debug_system.h"
#include <QDebug>

void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    if (!event) {
        return;
    }
    
    // Special handler for backtick/tilde key to toggle debug console
    if (event->key() == Qt::Key_QuoteLeft || event->key() == Qt::Key_AsciiTilde) {
        qDebug() << "Backtick/tilde key detected in GLArenaWidget::keyPressEvent";
        if (m_debugSystem) {
            toggleDebugConsole();
            event->accept();
            return;
        }
    }
    
    // Then check other debug key event handlers
    if (m_debugSystem && processDebugKeyEvent(event)) {
        event->accept();
        return; // Debug system handled the event
    }
    
    // Handle inventory toggle key (I)
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        
        // Update mouse tracking state based on inventory visibility
        updateMouseTrackingState();
        event->accept();
        return;
    }
    
    // Handle voxel placement/removal
    if (m_voxelSystem && m_highlightedVoxelFace >= 0) {
        if (event->key() == Qt::Key_E) {
            placeVoxel();
            event->accept();
            return;
        } else if (event->key() == Qt::Key_Q) {
            removeVoxel();
            event->accept();
            return;
        }
    }
    
    // Handle inventory UI
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        event->accept();
        return;
    }
    
    // Handle debug console
    bool consoleVisible = false;
    if (m_debugSystem) {
        // Check console visibility
        consoleVisible = isDebugConsoleVisible();
        
        if (consoleVisible) {
            // Debug console handles the event through processDebugKeyEvent above
            event->accept();
            return;
        }
    }
    
    // Handle escape key
    if (event->key() == Qt::Key_Escape) {
        // If escape menu exists, toggle it
        if (m_escapeMenu) {
            toggleEscapeMenu();
            event->accept();
            return;
        }
        
        // Close inventory if it's open (fallback if escape menu doesn't exist)
        if (m_inventoryUI && m_inventoryUI->isVisible()) {
            m_inventoryUI->setVisible(false);
            updateMouseTrackingState();
            event->accept();
            return;
        }
    }
    
    // Pass to player controller
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
    
    // Skip key release handling if debug console is open
    if (m_debugSystem && isDebugConsoleVisible()) {
        event->accept();
        return;
    }
    
    // Skip if inventory UI has focus
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        event->accept();
        return;
    }
    
    // Skip if escape menu is visible
    if (m_escapeMenu && m_escapeMenu->isVisible()) {
        event->accept();
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
    
    // Check if debug console is visible
    bool consoleVisible = false;
    if (m_debugSystem) {
        consoleVisible = isDebugConsoleVisible();
    }
    
    // Handle inventory first (highest priority)
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        event->accept();
        return;
    }
    
    // Skip camera movement if escape menu is visible
    if (m_escapeMenu && m_escapeMenu->isVisible()) {
        event->accept();
        return;
    }
    
    // Skip camera movement if debug console is visible
    if (consoleVisible) {
        event->accept();
        return;
    }
    
    // Pass to player controller for camera movement
    if (m_playerController && hasFocus() && !consoleVisible) {
        m_playerController->handleMouseMove(event);
        
        // Recenter cursor if not in console mode
        if (cursor().shape() == Qt::BlankCursor) {
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
    
    // Check if debug console is visible
    bool consoleVisible = false;
    if (m_debugSystem) {
        consoleVisible = isDebugConsoleVisible();
    }
    
    // Handle inventory first (highest priority)
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        event->accept();
        return;
    }
    
    // Skip mouse handling if escape menu is visible
    if (m_escapeMenu && m_escapeMenu->isVisible()) {
        event->accept();
        return;
    }
    
    // Skip mouse handling if debug console is visible
    if (consoleVisible) {
        event->accept();
        return;
    }
    
    // Handle voxel placement/removal
    if (m_voxelSystem && m_highlightedVoxelFace >= 0) {
        if (event->button() == Qt::LeftButton) {
            placeVoxel();
            event->accept();
            return;
        } else if (event->button() == Qt::RightButton) {
            removeVoxel();
            event->accept();
            return;
        }
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
    
    // Check if debug console is visible
    bool consoleVisible = false;
    if (m_debugSystem) {
        consoleVisible = isDebugConsoleVisible();
    }
    
    // Handle inventory first (highest priority)
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        event->accept();
        return;
    }
    
    // Skip mouse handling if escape menu is visible
    if (m_escapeMenu && m_escapeMenu->isVisible()) {
        event->accept();
        return;
    }
    
    // Skip mouse handling if debug console is visible
    if (consoleVisible) {
        event->accept();
        return;
    }
    
    // Default implementation
    QOpenGLWidget::mouseReleaseEvent(event);
}

// Toggle the escape menu visibility
void GLArenaWidget::toggleEscapeMenu() {
    if (!m_escapeMenu) {
        return;
    }
    
    m_escapeMenu->toggleVisibility();
    updateMouseTrackingState();
}

// Handle the return to main menu signal from escape menu
void GLArenaWidget::onReturnToMainMenu() {
    // Forward the signal
    emit returnToMainMenu();
}