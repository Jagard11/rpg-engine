// src/player_controller.cpp - Complete file with fixes for player movement
#include "../include/player_controller.h"
#include "../include/game_scene.h"
#include <QDebug>
#include <cmath>

PlayerController::PlayerController(GameScene *scene, QObject *parent)
    : QObject(parent), gameScene(scene), position(0, 0, 0), rotation(0),
    movementSpeed(0.15), rotationSpeed(0.075),  // Increased speeds for better responsiveness
    movingForward(false), movingBackward(false), 
    movingLeft(false), movingRight(false),
    rotatingLeft(false), rotatingRight(false) {
    
    // Set up update timer with faster framerate for smoother movement
    updateTimer.setInterval(16); // ~60 FPS
    connect(&updateTimer, &QTimer::timeout, this, &PlayerController::updatePosition);
}

void PlayerController::handleKeyPress(QKeyEvent *event) {
    // Handle key press events for movement
    switch (event->key()) {
        case Qt::Key_W:
            movingForward = true;
            qDebug() << "Key W pressed - Moving forward";
            break;
        case Qt::Key_S:
            movingBackward = true;
            qDebug() << "Key S pressed - Moving backward";
            break;
        case Qt::Key_A:
            rotatingLeft = true;
            qDebug() << "Key A pressed - Rotating left";
            break;
        case Qt::Key_D:
            rotatingRight = true;
            qDebug() << "Key D pressed - Rotating right";
            break;
        case Qt::Key_Q:
            movingLeft = true;
            qDebug() << "Key Q pressed - Strafing left";
            break;
        case Qt::Key_E:
            movingRight = true;
            qDebug() << "Key E pressed - Strafing right";
            break;
    }
}

void PlayerController::handleKeyRelease(QKeyEvent *event) {
    // Handle key release events for movement
    switch (event->key()) {
        case Qt::Key_W:
            movingForward = false;
            qDebug() << "Key W released - Stopped moving forward";
            break;
        case Qt::Key_S:
            movingBackward = false;
            qDebug() << "Key S released - Stopped moving backward";
            break;
        case Qt::Key_A:
            rotatingLeft = false;
            qDebug() << "Key A released - Stopped rotating left";
            break;
        case Qt::Key_D:
            rotatingRight = false;
            qDebug() << "Key D released - Stopped rotating right";
            break;
        case Qt::Key_Q:
            movingLeft = false;
            qDebug() << "Key Q released - Stopped strafing left";
            break;
        case Qt::Key_E:
            movingRight = false;
            qDebug() << "Key E released - Stopped strafing right";
            break;
    }
}

void PlayerController::createPlayerEntity() {
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

void PlayerController::startUpdates() {
    qDebug() << "Starting position updates";
    updateTimer.start();
}

void PlayerController::stopUpdates() {
    qDebug() << "Stopping position updates";
    updateTimer.stop();
}

void PlayerController::updatePosition() {
    // Calculate new position based on input
    QVector3D newPosition = position;
    float newRotation = rotation;
    
    // Rotation - increased speed for better responsiveness
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
    
    // Debug movement if position or rotation changed
    bool posChanged = newPosition != position;
    bool rotChanged = newRotation != rotation;
    
    if (posChanged || rotChanged) {
        qDebug() << "Player movement update - Position:" << newPosition.x() << newPosition.y() << newPosition.z()
                 << "Rotation:" << newRotation;
    }
    
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

QVector3D PlayerController::applyConstraints(const QVector3D &newPosition) {
    // Check if new position would cause a collision
    if (gameScene->checkCollision("player", newPosition)) {
        qDebug() << "Collision detected, preventing movement to" << newPosition.x() << newPosition.y() << newPosition.z();
        return position; // If collision, keep old position
    }
    
    return newPosition;
}