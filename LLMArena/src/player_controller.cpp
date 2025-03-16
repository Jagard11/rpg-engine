// src/player_controller.cpp - Fixed version
#include "../include/player_controller.h"
#include "../include/game_scene.h"
#include <QDebug>
#include <cmath>

PlayerController::PlayerController(GameScene *scene, QObject *parent)
    : QObject(parent), gameScene(scene), position(0, 0, 0), rotation(0),
    movementSpeed(0.1), rotationSpeed(0.05),  // Reduced speeds for better stability
    movingForward(false), movingBackward(false), 
    movingLeft(false), movingRight(false),
    rotatingLeft(false), rotatingRight(false) {
    
    // Set up update timer with appropriate framerate
    updateTimer.setInterval(33); // ~30 FPS - more stable update rate
    connect(&updateTimer, &QTimer::timeout, this, &PlayerController::updatePosition);
}

void PlayerController::handleKeyPress(QKeyEvent *event) {
    if (!event) return;
    
    // Handle key press events for movement
    switch (event->key()) {
        case Qt::Key_W:
            movingForward = true;
            break;
        case Qt::Key_S:
            movingBackward = true;
            break;
        case Qt::Key_A:
            rotatingLeft = true;
            break;
        case Qt::Key_D:
            rotatingRight = true;
            break;
        case Qt::Key_Q:
            movingLeft = true;
            break;
        case Qt::Key_E:
            movingRight = true;
            break;
    }
    
    // Don't call update directly - let the timer handle updates
}

void PlayerController::handleKeyRelease(QKeyEvent *event) {
    if (!event) return;
    
    // Handle key release events for movement
    switch (event->key()) {
        case Qt::Key_W:
            movingForward = false;
            break;
        case Qt::Key_S:
            movingBackward = false;
            break;
        case Qt::Key_A:
            rotatingLeft = false;
            break;
        case Qt::Key_D:
            rotatingRight = false;
            break;
        case Qt::Key_Q:
            movingLeft = false;
            break;
        case Qt::Key_E:
            movingRight = false;
            break;
    }
    
    // Don't call update directly - let the timer handle updates
}

void PlayerController::createPlayerEntity() {
    if (!gameScene) {
        qWarning() << "Cannot create player entity: no game scene";
        return;
    }
    
    try {
        // First check if player entity already exists and remove it
        gameScene->removeEntity("player");
        
        // Create player entity in the game scene
        GameEntity playerEntity;
        playerEntity.id = "player";
        playerEntity.type = "player";
        playerEntity.position = QVector3D(5, 0.9, 5); // Start in the arena, offset from center, and slightly above the floor
        playerEntity.dimensions = QVector3D(0.6, 1.8, 0.6); // Typical human dimensions
        playerEntity.isStatic = false;
        
        gameScene->addEntity(playerEntity);
        
        // Set initial position
        position = playerEntity.position;
        
        // Set initial rotation (facing center of arena)
        rotation = atan2(-position.z(), -position.x());
        
        qDebug() << "Player entity created at position:" << position.x() << position.y() << position.z()
                << "with rotation:" << rotation;
        
        // Emit initial position
        emit positionChanged(position);
        emit rotationChanged(rotation);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception creating player entity:" << e.what();
    }
}

void PlayerController::startUpdates() {
    qDebug() << "Starting position updates";
    updateTimer.start();
}

void PlayerController::stopUpdates() {
    qDebug() << "Stopping position updates";
    updateTimer.stop();
}

void PlayerController::updatePosition() {
    try {
        // Check if game scene exists
        if (!gameScene) {
            qWarning() << "No game scene in player controller";
            return;
        }
        
        // Calculate new position based on input
        QVector3D newPosition = position;
        float newRotation = rotation;
        
        // Rotation
        if (rotatingLeft) {
            newRotation += rotationSpeed;
        }
        if (rotatingRight) {
            newRotation -= rotationSpeed;
        }
        
        // Normalize rotation to 0-2π
        while (newRotation < 0) newRotation += 2 * M_PI;
        while (newRotation >= 2 * M_PI) newRotation -= 2 * M_PI;
        
        // Forward/backward movement in the direction of rotation
        QVector3D movementVector(0, 0, 0);
        
        if (movingForward) {
            movementVector += QVector3D(cos(rotation), 0, sin(rotation));
        }
        if (movingBackward) {
            movementVector -= QVector3D(cos(rotation), 0, sin(rotation));
        }
        
        // Strafe left/right (perpendicular to rotation)
        if (movingLeft) {
            movementVector += QVector3D(cos(rotation + M_PI/2), 0, sin(rotation + M_PI/2));
        }
        if (movingRight) {
            movementVector -= QVector3D(cos(rotation + M_PI/2), 0, sin(rotation + M_PI/2));
        }
        
        // Normalize movement vector if moving in multiple directions
        if (movementVector.length() > 0.01) {
            movementVector.normalize();
            // Apply movement speed
            newPosition += movementVector * movementSpeed;
        }
        
        // Apply constraints to keep player in arena
        newPosition = applyConstraints(newPosition);
        
        // Only update if position or rotation actually changed
        bool posChanged = (newPosition - position).length() > 0.001;
        bool rotChanged = fabs(newRotation - rotation) > 0.001;
        
        // Update position if it has changed
        if (posChanged) {
            position = newPosition;
            gameScene->updateEntityPosition("player", position);
            emit positionChanged(position);
        }
        
        // Update rotation if it has changed
        if (rotChanged) {
            rotation = newRotation;
            emit rotationChanged(rotation);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in PlayerController::updatePosition:" << e.what();
    }
}

QVector3D PlayerController::applyConstraints(const QVector3D &newPosition) {
    if (!gameScene) {
        return position; // If no game scene, keep current position
    }
    
    try {
        // Check if new position would cause a collision
        if (gameScene->checkCollision("player", newPosition)) {
            return position; // If collision, keep old position
        }
        
        // Check if new position is within arena bounds
        QVector3D centerPos(0, 0, 0);
        float distanceFromCenter = QVector2D(newPosition.x(), newPosition.z()).length();
        
        // Keep player within a safe distance from arena radius
        float arenaRadius = gameScene->getArenaRadius();
        if (arenaRadius > 0 && distanceFromCenter > arenaRadius - 1.0) {
            // Calculate direction vector from center to position
            QVector3D direction = newPosition - centerPos;
            direction.setY(0); // Keep y-coordinate unchanged
            
            // Normalize and scale to safe radius
            if (direction.length() > 0.001) {
                direction.normalize();
                direction *= (arenaRadius - 1.0);
                
                // Create new position using direction from center
                return QVector3D(direction.x(), newPosition.y(), direction.z());
            }
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in collision check:" << e.what();
        return position; // On error, keep current position
    }
    
    return newPosition;
}