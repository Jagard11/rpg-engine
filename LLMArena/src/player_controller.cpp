// src/player_controller.cpp
#include "../include/player_controller.h"
#include "../include/game_scene.h"
#include <QDebug>
#include <cmath>

PlayerController::PlayerController(GameScene *scene, QObject *parent)
    : QObject(parent), gameScene(scene), position(0, 0, 0), rotation(0),
    movementSpeed(0.1), rotationSpeed(0.05),
    movingForward(false), movingBackward(false), 
    movingLeft(false), movingRight(false),
    rotatingLeft(false), rotatingRight(false) {
    
    // Set up update timer
    updateTimer.setInterval(16); // ~60 FPS
    connect(&updateTimer, &QTimer::timeout, this, &PlayerController::updatePosition);
}

void PlayerController::handleKeyPress(QKeyEvent *event) {
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
}

void PlayerController::handleKeyRelease(QKeyEvent *event) {
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
}

void PlayerController::createPlayerEntity() {
    // Create player entity in the game scene
    GameEntity playerEntity;
    playerEntity.id = "player";
    playerEntity.type = "player";
    playerEntity.position = QVector3D(5, 0, 5); // Start in the arena, offset from center
    playerEntity.dimensions = QVector3D(0.6, 1.8, 0.6); // Typical human dimensions
    playerEntity.isStatic = false;
    
    gameScene->addEntity(playerEntity);
    
    // Set initial position
    position = playerEntity.position;
    
    // Set initial rotation (facing center of arena)
    rotation = atan2(-position.z(), -position.x());
    
    // Emit initial position
    emit positionChanged(position);
    emit rotationChanged(rotation);
}

void PlayerController::startUpdates() {
    updateTimer.start();
}

void PlayerController::stopUpdates() {
    updateTimer.stop();
}

void PlayerController::updatePosition() {
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
    if (movingForward) {
        newPosition.setX(position.x() + cos(rotation) * movementSpeed);
        newPosition.setZ(position.z() + sin(rotation) * movementSpeed);
    }
    if (movingBackward) {
        newPosition.setX(position.x() - cos(rotation) * movementSpeed);
        newPosition.setZ(position.z() - sin(rotation) * movementSpeed);
    }
    
    // Strafe left/right (perpendicular to rotation)
    if (movingLeft) {
        newPosition.setX(position.x() + cos(rotation + M_PI/2) * movementSpeed);
        newPosition.setZ(position.z() + sin(rotation + M_PI/2) * movementSpeed);
    }
    if (movingRight) {
        newPosition.setX(position.x() - cos(rotation + M_PI/2) * movementSpeed);
        newPosition.setZ(position.z() - sin(rotation + M_PI/2) * movementSpeed);
    }
    
    // Apply constraints to keep player in arena
    newPosition = applyConstraints(newPosition);
    
    // Update position if it has changed
    if (newPosition != position) {
        position = newPosition;
        gameScene->updateEntityPosition("player", position);
        emit positionChanged(position);
    }
    
    // Update rotation if it has changed
    if (newRotation != rotation) {
        rotation = newRotation;
        emit rotationChanged(rotation);
    }
}

QVector3D PlayerController::applyConstraints(const QVector3D &newPosition) {
    // Check if new position would cause a collision
    if (gameScene->checkCollision("player", newPosition)) {
        return position; // If collision, keep old position
    }
    
    return newPosition;
}