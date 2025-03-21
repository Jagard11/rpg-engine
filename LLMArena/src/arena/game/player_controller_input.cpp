// src/arena/game/player_controller_input.cpp
#include "../include/arena/game/player_controller.h"
#include <QDebug>
#include <QMutex>
#include <QWidget>
#include <cmath>

// External reference to movement mutex
extern QMutex playerMovementMutex;

void PlayerController::handleKeyPress(QKeyEvent *event) {
    if (!event) return;
    
    // Acquire mutex for thread safety
    QMutexLocker locker(&playerMovementMutex);
    
    try {
        // Handle key press events for movement with correct WASD mappings
        switch (event->key()) {
            case Qt::Key_W:  // Forward
                movingForward = true;
                break;
            case Qt::Key_S:  // Backward
                movingBackward = true;
                break;
            case Qt::Key_A:  // Strafe left
                movingLeft = true;
                break;
            case Qt::Key_D:  // Strafe right
                movingRight = true;
                break;
            case Qt::Key_Q:  // Rotate left
                rotatingLeft = true;
                break;
            case Qt::Key_E:  // Rotate right
                rotatingRight = true;
                break;
            case Qt::Key_Space:  // Jump
                if (stance == PlayerStance::Standing && !jumping) {
                    jumping = true;
                    jumpVelocity = 0.2f; // Initial upward velocity
                }
                break;
            case Qt::Key_Shift:  // Sprint
                sprinting = true;
                // If crouching or prone, begin transition to standing
                if (stance == PlayerStance::Crouching || stance == PlayerStance::Prone) {
                    beginStanceTransition(PlayerStance::Standing);
                }
                break;
            case Qt::Key_C:  // Crouch toggle
                if (stance == PlayerStance::Crouching) {
                    beginStanceTransition(PlayerStance::Standing);
                } else {
                    beginStanceTransition(PlayerStance::Crouching);
                }
                break;
            case Qt::Key_Z:  // Prone toggle
                if (stance == PlayerStance::Prone) {
                    beginStanceTransition(PlayerStance::Standing);
                } else {
                    beginStanceTransition(PlayerStance::Prone);
                }
                break;
        }
    } catch (...) {
        qWarning() << "Exception in handleKeyPress";
    }
}

void PlayerController::handleKeyRelease(QKeyEvent *event) {
    if (!event) return;
    
    // Acquire mutex for thread safety
    QMutexLocker locker(&playerMovementMutex);
    
    try {
        // Handle key release events for movement
        switch (event->key()) {
            case Qt::Key_W:  // Forward
                movingForward = false;
                break;
            case Qt::Key_S:  // Backward
                movingBackward = false;
                break;
            case Qt::Key_A:  // Strafe left
                movingLeft = false;
                break;
            case Qt::Key_D:  // Strafe right
                movingRight = false;
                break;
            case Qt::Key_Q:  // Rotate left
                rotatingLeft = false;
                break;
            case Qt::Key_E:  // Rotate right
                rotatingRight = false;
                break;
            case Qt::Key_Shift:  // Sprint
                sprinting = false;
                break;
        }
    } catch (...) {
        qWarning() << "Exception in handleKeyRelease";
    }
}

void PlayerController::handleMouseMove(QMouseEvent *event) {
    if (!event) return;
    
    // Acquire mutex for thread safety
    QMutexLocker locker(&playerMovementMutex);
    
    try {
        // Calculate mouse movement delta directly from parameters
        int dx = event->x() - width()/2;
        int dy = event->y() - height()/2;
        
        // Only process if there's actual movement
        if (dx != 0) {
            // Apply mouse sensitivity - reduced for smoother control
            float sensitivity = 0.003f;
            rotation += dx * sensitivity;
            
            // Normalize rotation to 0-2π
            while (rotation < 0) rotation += 2 * M_PI;
            while (rotation >= 2 * M_PI) rotation -= 2 * M_PI;
            
            // Emit rotation changed signal
            emit rotationChanged(rotation);
        }
        
        if (dy != 0) {
            // Apply mouse sensitivity (negative to make up = look up)
            float sensitivity = 0.003f;
            pitch -= dy * sensitivity;
            
            // Clamp pitch to prevent looking too far up or down
            const float maxPitch = 1.48f; // ~85 degrees
            if (pitch > maxPitch) pitch = maxPitch;
            if (pitch < -maxPitch) pitch = -maxPitch;
            
            // Emit pitch changed signal
            emit pitchChanged(pitch);
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception in handleMouseMove:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in handleMouseMove";
    }
}

// Get widget width for mouse centering
int PlayerController::width() const {
    if (parent() && parent()->isWidgetType()) {
        return qobject_cast<QWidget*>(parent())->width();
    }
    return 800; // Default width if parent widget not available
}

// Get widget height for mouse centering
int PlayerController::height() const {
    if (parent() && parent()->isWidgetType()) {
        return qobject_cast<QWidget*>(parent())->height();
    }
    return 600; // Default height if parent widget not available
}