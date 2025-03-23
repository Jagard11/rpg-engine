// src/arena/ui/gl_widgets/gl_arena_widget_input.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QKeyEvent>
#include <QDebug>
#include <QApplication>

void GLArenaWidget::keyPressEvent(QKeyEvent* event)
{
    // Handle inventory toggle with 'I' key
    if (event->key() == Qt::Key_I && !event->isAutoRepeat()) {
        if (m_inventoryUI) {
            m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
            event->accept();
            return;
        }
    }
    
    // Pass to inventory UI for handling action bar keys (1-=)
    if (m_inventoryUI) {
        m_inventoryUI->handleKeyPress(event->key());
    }
    
    // Safety check before handling key events
    if (!m_initialized || !m_playerController || !event) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    try {
        m_playerController->handleKeyPress(event);
    } 
    catch (const std::exception& e) {
        qWarning() << "Exception in keyPressEvent:" << e.what();
    }
    
    // Let parent widget handle the event too
    QOpenGLWidget::keyPressEvent(event);
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event)
{
    // Safety check before handling key events
    if (!m_initialized || !m_playerController || !event) {
        QOpenGLWidget::keyReleaseEvent(event);
        return;
    }

    try {
        m_playerController->handleKeyRelease(event);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in keyReleaseEvent:" << e.what();
    }
    
    // Let parent widget handle the event too
    QOpenGLWidget::keyReleaseEvent(event);
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position)
{
    emit playerPositionUpdated(position.x(), position.y(), position.z());
    update(); // Request a redraw
}

void GLArenaWidget::onPlayerRotationChanged(float rotation)
{
    update(); // Request a redraw
}

void GLArenaWidget::onPlayerPitchChanged(float pitch)
{
    update(); // Request a redraw
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event) {
    // Ensure we have focus
    setFocus();
    
    // First check if inventory is visible and handle inventory interactions
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        event->accept();
        return;
    }
    
    // If inventory is not visible, handle voxel placement/removal
    if (event->button() == Qt::LeftButton) {
        // Place voxel
        if (m_inventoryUI && m_inventoryUI->hasVoxelTypeSelected()) {
            placeVoxel();
        }
    } else if (event->button() == Qt::RightButton) {
        // Remove voxel
        if (m_inventoryUI && m_inventoryUI->hasVoxelTypeSelected()) {
            removeVoxel();
        }
    }
    
    // Update cursor visibility and mouse tracking
    updateMouseTrackingState();
    
    // Move cursor to center when clicked if in game mode
    if (!m_inventoryUI || !m_inventoryUI->isVisible()) {
        QPoint center(width()/2, height()/2);
        QCursor::setPos(mapToGlobal(center));
    }
    
    QOpenGLWidget::mousePressEvent(event);
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) {
    // Handle inventory interactions if visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        event->accept();
        return;
    }
    
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) {
    // Handle inventory mode - process normally
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }
    
    // For game mode (inventory closed) - capture mouse for camera control
    if (m_playerController) {
        // Only process if mouse movement is significant
        static QPoint lastPos = QPoint(width()/2, height()/2);
        QPoint currentPos = event->pos();
        QPoint delta = currentPos - lastPos;
        
        if (!delta.isNull()) {
            // Create a new event with correct position data
            QMouseEvent newEvent(
                event->type(),
                currentPos,
                event->button(),
                event->buttons(),
                event->modifiers()
            );
            
            // Pass to player controller
            m_playerController->handleMouseMove(&newEvent);
            
            // Reset cursor position to center
            QPoint center(width()/2, height()/2);
            QCursor::setPos(mapToGlobal(center));
            
            // Update last position to center
            lastPos = center;
        }
    }
    
    // Don't pass to parent for camera control mode
    event->accept();
}

// Helper to manage cursor and mouse tracking state
void GLArenaWidget::updateMouseTrackingState() {
    // Always enable tracking
    QOpenGLWidget::setMouseTracking(true);
    
    // Set cursor visibility based on inventory
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::BlankCursor);
    }
}