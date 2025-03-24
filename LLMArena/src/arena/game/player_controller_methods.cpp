// src/arena/game/player_controller_methods.cpp
#include "../../../include/arena/game/player_controller.h"
#include "../../../include/arena/core/arena_core.h"
#include <QDebug>
#include <QMutex>
#include <QWidget>

// External reference to movement mutex
extern QMutex playerMovementMutex;

// Helper methods for mouse handling and screen dimensions
int PlayerController::width() const {
    if (parent() && parent()->isWidgetType()) {
        return qobject_cast<QWidget*>(parent())->width();
    }
    return 800; // Default width if parent widget not available
}

int PlayerController::height() const {
    if (parent() && parent()->isWidgetType()) {
        return qobject_cast<QWidget*>(parent())->height();
    }
    return 600; // Default height if parent widget not available
}

// Add this method to handle screen dimension changes
void PlayerController::setScreenDimensions(int width, int height) {
    // We don't need to store these dimensions explicitly, as we're getting them from the parent widget
    // But the method is needed for API compatibility with GLArenaWidget
    
    // We could potentially use this method to update mouse sensitivity based on screen size
    Q_UNUSED(width);
    Q_UNUSED(height);
    
    // Log for debugging
    qDebug() << "Player controller screen dimensions set to:" << width << "x" << height;
    
    // Since we depend on the parent widget for dimensions, we don't need to store these values
    // They will be obtained through the width() and height() methods when needed
}