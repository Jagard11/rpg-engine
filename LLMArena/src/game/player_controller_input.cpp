// src/game/player_controller_input.cpp
#include "../include/game/player_controller.h"
#include <QDebug>
#include <QMutex>
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
        // Calculate mouse movement delta
        static QPoint lastPos;
        QPoint currentPos = event->pos();
        
        // Skip first event to initialize lastPos
        if (lastPos.isNull()) {
            lastPos = currentPos;
            return;
        }
        
        // Calculate delta and update rotation
        int dx = currentPos.x() - lastPos.x();
        if (dx != 0) {
            // Apply mouse sensitivity with inverted controls
            rotation += dx * 0.01f;
            
            // Normalize rotation to 0-2π
            while (rotation < 0) rotation += 2 * M_PI;
            while (rotation >= 2 * M_PI) rotation -= 2 * M_PI;
            
            // Emit rotation changed signal
            emit rotationChanged(rotation);
        }
        
        // Store current position for next time
        lastPos = currentPos;
    } catch (...) {
        qWarning() << "Exception in handleMouseMove";
    }
}