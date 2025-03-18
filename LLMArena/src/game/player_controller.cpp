// src/game/player_controller.cpp
#include "../include/game/player_controller.h"
#include "../include/game/game_scene.h"
#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <cmath>

// Static mutex for thread safety during player movement
static QMutex playerMovementMutex;

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

float PlayerController::getEyeHeight() const {
    switch (stance) {
        case PlayerStance::Standing:
            return 1.6f; // Default eye level
        case PlayerStance::Crouching:
            return 0.8f; // Half height
        case PlayerStance::Prone:
            return 0.2f; // Ground level
        case PlayerStance::Jumping:
            return 1.6f + jumpVelocity; // Add jump height
        default:
            return 1.6f;
    }
}

float PlayerController::getSpeedMultiplier() const {
    // Base multiplier from stance
    float multiplier = 1.0f;
    
    switch (stance) {
        case PlayerStance::Standing:
            multiplier = 1.0f;
            break;
        case PlayerStance::Crouching:
            multiplier = 0.5f; // Half speed
            break;
        case PlayerStance::Prone:
            multiplier = 0.25f; // Quarter speed
            break;
        case PlayerStance::Jumping:
            multiplier = 1.0f;
            break;
    }
    
    // Apply sprint multiplier if sprinting and allowed to sprint
    if (sprinting && stance == PlayerStance::Standing) {
        multiplier *= 2.0f; // Double speed when sprinting
    }
    
    return multiplier;
}

void PlayerController::beginStanceTransition(PlayerStance newStance) {
    if (stance == newStance || inStanceTransition)
        return;
    
    // Already set target stance
    targetStance = newStance;
    inStanceTransition = true;
    
    // Set transition time based on current stance and target
    int transitionTime = 0;
    
    if (stance == PlayerStance::Prone) {
        if (targetStance == PlayerStance::Standing) {
            transitionTime = 1000; // 1 second from prone to standing
        } else if (targetStance == PlayerStance::Crouching) {
            transitionTime = 500; // 0.5 seconds from prone to crouch
        }
    } else if (stance == PlayerStance::Crouching) {
        if (targetStance == PlayerStance::Standing) {
            transitionTime = 300; // 0.3 seconds from crouch to standing
        } else if (targetStance == PlayerStance::Prone) {
            transitionTime = 500; // 0.5 seconds from crouch to prone
        }
    } else if (stance == PlayerStance::Standing) {
        if (targetStance == PlayerStance::Crouching) {
            transitionTime = 200; // 0.2 seconds from standing to crouch
        } else if (targetStance == PlayerStance::Prone) {
            transitionTime = 800; // 0.8 seconds from standing to prone
        }
    }
    
    // Start the timer if there's a transition time
    if (transitionTime > 0) {
        qDebug() << "Starting stance transition from" << (int)stance << "to" << (int)targetStance 
                 << "with duration" << transitionTime << "ms";
        stanceTransitionTimer.start(transitionTime);
    } else {
        // Complete immediately if no transition time
        completeStanceTransition();
    }
}

void PlayerController::completeStanceTransition() {
    if (!inStanceTransition)
        return;
    
    qDebug() << "Completed stance transition to" << (int)targetStance;
    stance = targetStance;
    inStanceTransition = false;
    
    emit stanceChanged(stance);
}

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

void PlayerController::createPlayerEntity() {
    if (!gameScene) {
        qWarning() << "Cannot create player entity: no game scene";
        return;
    }
    
    // Acquire mutex for thread safety
    QMutexLocker locker(&playerMovementMutex);
    
    try {
        // Create player entity in the game scene
        GameEntity playerEntity;
        playerEntity.id = "player";
        playerEntity.type = "player";
        playerEntity.position = QVector3D(5, 0.9, 5); // Start position
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
        
        qDebug() << "Player entity created at position:" << position.x() << position.y() << position.z()
                << "with rotation:" << rotation;
        
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
    qDebug() << "Starting position updates at" << updateTimer.interval() << "ms interval";
    updateTimer.start();
}

void PlayerController::stopUpdates() {
    qDebug() << "Stopping position updates";
    updateTimer.stop();
}

void PlayerController::updatePosition() {
    // Skip update if mutex can't be acquired immediately
    if (!playerMovementMutex.tryLock()) {
        return;
    }
    
    // Ensure mutex is released when function exits
    struct MutexReleaser {
        QMutex& mutex;
        MutexReleaser(QMutex& m) : mutex(m) {}
        ~MutexReleaser() { mutex.unlock(); }
    } releaser(playerMovementMutex);
    
    // Ensure game scene exists
    if (!gameScene) {
        return;
    }
    
    try {
        // Verify the player entity exists
        bool playerExists = false;
        try {
            GameEntity playerEntity = gameScene->getEntity("player");
            playerExists = !playerEntity.id.isEmpty();
        } catch (...) {
            // Ignore exceptions
        }
        
        if (!playerExists) {
            qWarning() << "Player entity not found in scene, recreating";
            createPlayerEntity();
            return;
        }
        
        // Calculate new position and rotation
        QVector3D newPosition = position;
        float newRotation = rotation;
        
        // Apply rotation changes
        bool rotationHasChanged = false;
        if (rotatingLeft) {
            newRotation -= rotationSpeed;
            rotationHasChanged = true;
        }
        if (rotatingRight) {
            newRotation += rotationSpeed;
            rotationHasChanged = true;
        }
        
        // Normalize rotation to 0-2π
        while (newRotation < 0) newRotation += 2 * M_PI;
        while (newRotation >= 2 * M_PI) newRotation -= 2 * M_PI;
        
        // Handle jumping physics
        if (jumping) {
            // Apply gravity to jump velocity
            jumpVelocity -= gravity;
            
            // Update vertical position
            newPosition.setY(newPosition.y() + jumpVelocity);
            
            // Check if we've landed
            if (newPosition.y() <= 0.9f) {
                newPosition.setY(0.9f);
                jumping = false;
                jumpVelocity = 0.0f;
            }
        }
        
        // Apply movement changes at speed based on stance
        bool positionHasChanged = false;
        
        // Reset target velocity to zero
        targetVelocity = QVector3D(0, 0, 0);
        
        // Calculate movement direction
        if (movingForward || movingBackward || movingLeft || movingRight) {
            QVector3D movementVector(0, 0, 0);
            
            // Calculate raw direction based on inputs
            if (movingForward) {
                movementVector += QVector3D(cos(rotation), 0, sin(rotation));
            }
            if (movingBackward) {
                movementVector -= QVector3D(cos(rotation), 0, sin(rotation));
            }
            if (movingLeft) {
                movementVector -= QVector3D(cos(rotation + M_PI/2), 0, sin(rotation + M_PI/2));
            }
            if (movingRight) {
                movementVector += QVector3D(cos(rotation + M_PI/2), 0, sin(rotation + M_PI/2));
            }
            
            // Apply movement with appropriate speed multiplier
            if (movementVector.length() > 0.01) {
                movementVector.normalize();
                float speedMult = getSpeedMultiplier();
                
                // Set target velocity based on input
                targetVelocity = movementVector * (movementSpeed * speedMult);
            }
        }
        
        // Apply physics: gradually change current velocity toward target velocity
        velocity.setX(velocity.x() + (targetVelocity.x() - velocity.x()) * acceleration);
        velocity.setZ(velocity.z() + (targetVelocity.z() - velocity.z()) * acceleration);
        
        // Apply friction when no input in that direction
        if (qAbs(targetVelocity.x()) < 0.001f) {
            velocity.setX(velocity.x() * (1.0f - friction));
        }
        if (qAbs(targetVelocity.z()) < 0.001f) {
            velocity.setZ(velocity.z() * (1.0f - friction));
        }
        
        // Update position with current velocity
        if (velocity.length() > 0.001f) {
            QVector3D newPositionBeforeConstraints = newPosition + velocity;
            
            // Wall sliding - try each axis independently if both fail
            bool canMove = true;
            double distanceFromCenter = sqrt(newPositionBeforeConstraints.x() * newPositionBeforeConstraints.x() + 
                                     newPositionBeforeConstraints.z() * newPositionBeforeConstraints.z());
            
            if (distanceFromCenter >= 9.0) {
                // Try X movement only
                QVector3D xOnlyPosition = QVector3D(newPositionBeforeConstraints.x(), newPosition.y(), newPosition.z());
                double xDistanceFromCenter = sqrt(xOnlyPosition.x() * xOnlyPosition.x() + xOnlyPosition.z() * xOnlyPosition.z());
                
                // Try Z movement only
                QVector3D zOnlyPosition = QVector3D(newPosition.x(), newPosition.y(), newPositionBeforeConstraints.z());
                double zDistanceFromCenter = sqrt(zOnlyPosition.x() * zOnlyPosition.x() + zOnlyPosition.z() * zOnlyPosition.z());
                
                // Choose the valid axis to slide along, or none if both are invalid
                if (xDistanceFromCenter < 9.0) {
                    newPosition = xOnlyPosition;
                } else if (zDistanceFromCenter < 9.0) {
                    newPosition = zOnlyPosition;
                } else {
                    canMove = false;
                }
                
                // Reflect velocity to prevent sticking to walls
                if (!canMove) {
                    // Normalize direction from center to player
                    QVector3D dirFromCenter = QVector3D(position.x(), 0, position.z()).normalized();
                    
                    // Project velocity onto tangent to create sliding effect
                    float dotProduct = QVector3D::dotProduct(velocity, dirFromCenter);
                    velocity -= dirFromCenter * dotProduct * 1.2f; // Slightly over-correct
                }
            } else {
                // No wall collision, move normally
                newPosition = newPositionBeforeConstraints;
            }
            
            positionHasChanged = true;
        }
        
        // Apply hard boundary limits
        if (newPosition.x() < -9.0f) newPosition.setX(-9.0f);
        if (newPosition.x() > 9.0f) newPosition.setX(9.0f);
        if (newPosition.z() < -9.0f) newPosition.setZ(-9.0f);
        if (newPosition.z() > 9.0f) newPosition.setZ(9.0f);
        if (newPosition.y() < 0.9f && !jumping) newPosition.setY(0.9f);
        
        // Update position if changed
        if (positionHasChanged || jumping) {
            // Skip collision check - just use simplified boundary check
            double distanceFromCenter = sqrt(newPosition.x() * newPosition.x() + 
                                           newPosition.z() * newPosition.z());
            
            // Only move if we're within arena bounds
            if (distanceFromCenter < 9.0) {
                position = newPosition;
                
                // Update entity position
                try {
                    gameScene->updateEntityPosition("player", position);
                    emit positionChanged(position);
                } catch (...) {
                    // Ignore exceptions
                }
            }
        }
        
        // Update rotation if changed
        if (rotationHasChanged) {
            rotation = newRotation;
            emit rotationChanged(rotation);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in PlayerController::updatePosition:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in PlayerController::updatePosition";
    }
}

QVector3D PlayerController::applyConstraints(const QVector3D &newPosition) {
    // Method no longer used
    return newPosition;
}