// src/rendering/gl_arena/gl_arena_widget_input.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QKeyEvent>
#include <QDebug>

void GLArenaWidget::keyPressEvent(QKeyEvent* event)
{
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

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event)
{
    // Safety check before handling mouse events
    if (!m_initialized || !m_playerController || !event) {
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }

    // Enable mouse tracking for continuous movement
    setMouseTracking(true);
    
    try {
        // Forward event to player controller
        m_playerController->handleMouseMove(event);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in mouseMoveEvent:" << e.what();
    }
    
    // Let parent widget handle the event too
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event)
{
    // Grab focus
    setFocus();
    
    // Capture mouse cursor for mouse look
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::BlankCursor);
    }
    
    QOpenGLWidget::mousePressEvent(event);
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Release mouse cursor
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ArrowCursor);
    }
    
    QOpenGLWidget::mouseReleaseEvent(event);
}