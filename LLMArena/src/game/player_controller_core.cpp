// src/game/player_controller_core.cpp
#include "../include/game/player_controller.h"
#include "../include/game/game_scene.h"
#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <cmath>

// Define the mutex here so it can be used in all the player controller modules
QMutex playerMovementMutex;

PlayerController::PlayerController(GameScene *scene, QObject *parent)
    : QObject(parent), gameScene(scene), 
    position(0, 0, 0), 
    velocity(0, 0, 0),
    targetVelocity(0, 0, 0),
    rotation(0),
    movementSpeed(0.1), rotationSpeed(0.05),
    acceleration(0.01), friction(0.05),
    stance(PlayerStance::Standing), targetStance(PlayerStance::Standing),
    inStanceTransition(false),
    movingForward(false), movingBackward(false), 
    movingLeft(false), movingRight(false),
    rotatingLeft(false), rotatingRight(false),
    jumping(false), sprinting(false),
    jumpVelocity(0.0f), gravity(0.01f) {
    
    // Set up update timer with higher framerate for smoother movement
    updateTimer.setInterval(16); // ~60 FPS (16.67ms)
    connect(&updateTimer, &QTimer::timeout, this, &PlayerController::updatePosition);
    
    // Set up stance transition timer
    stanceTransitionTimer.setSingleShot(true);
    connect(&stanceTransitionTimer, &QTimer::timeout, this, &PlayerController::completeStanceTransition);
}

void PlayerController::createPlayerEntity() {
    if (!gameScene) {
        return;
    }
    
    // Acquire mutex for thread safety
    QMutexLocker locker(&playerMovementMutex);
    
    try {
        // Create player entity in the game scene
        GameEntity playerEntity;
        playerEntity.id = "player";
        playerEntity.type = "player";
        playerEntity.position = QVector3D(5, 1.0, 5); // Start position - 1 unit above floor (was 0.9)
        playerEntity.dimensions = QVector3D(0.6, 1.8, 0.6); // Human dimensions
        playerEntity.isStatic = false;
        
        // Check if player entity already exists and remove it if necessary
        try {
            GameEntity existingPlayer = gameScene->getEntity("player");
            if (!existingPlayer.id.isEmpty()) {
                gameScene->removeEntity("player");
            }
        } catch (...) {
            // Ignore errors here
        }
        
        // Reset stance
        stance = PlayerStance::Standing;
        targetStance = PlayerStance::Standing;
        inStanceTransition = false;
        jumping = false;
        sprinting = false;
        
        // Reset velocity
        velocity = QVector3D(0, 0, 0);
        targetVelocity = QVector3D(0, 0, 0);
        
        // Add new player entity
        try {
            gameScene->addEntity(playerEntity);
        } catch (const std::exception& e) {
            qWarning() << "Exception adding player entity:" << e.what();
        }
        
        // Set initial position
        position = playerEntity.position;
        
        // Initial rotation (facing toward center of room)
        rotation = atan2(-position.z(), -position.x());
        
        // Emit initial position and stance
        emit positionChanged(position);
        emit rotationChanged(rotation);
        emit stanceChanged(stance);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception creating player entity:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception creating player entity";
    }
}

void PlayerController::startUpdates() {
    updateTimer.start();
}

void PlayerController::stopUpdates() {
    updateTimer.stop();
}