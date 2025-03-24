// src/arena/ui/gl_widgets/gl_arena_widget_key_events.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QKeyEvent>

void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    // First check if debug system should handle the key
    if (processDebugKeyEvent(event)) {
        event->accept();
        return;
    }

    // If debug console is active, don't pass keys to player controller
    if (isDebugConsoleVisible()) {
        event->accept();
        return;
    }

    // If inventory is visible, handle inventory keys first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        event->accept();
        return;
    }

    // Handle voxel placement keys
    if (event->key() == Qt::Key_E) {
        placeVoxel();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Q) {
        removeVoxel();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Tab) {
        if (m_inventoryUI) {
            m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        }
        event->accept();
        return;
    }

    // Pass other key events to player controller
    if (m_playerController) {
        m_playerController->handleKeyPress(event);
    }
    
    QOpenGLWidget::keyPressEvent(event);
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) {
    // If debug console is active, don't pass keys to player controller
    if (isDebugConsoleVisible()) {
        event->accept();
        return;
    }
    
    // Pass key release events to player controller
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
    
    QOpenGLWidget::keyReleaseEvent(event);
}