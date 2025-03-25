// src/arena/game/player_controller_movement.cpp
#include "../include/arena/game/player_controller.h"
#include "include/arena/core/arena_core.h"
#include "include/arena/voxels/voxel_system_integration.h"
#include <QDebug>
#include <QMutex>
#include <cmath>

// External reference to movement mutex
extern QMutex playerMovementMutex;

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
        
        // Track if position has changed
        bool positionHasChanged = false;
        
        // Apply gravity regardless of jumping state
        // This makes the player fall when in the air
        bool isOnGround = false;
        float groundHeight = 1.0f; // Default ground height
        
        // Check if we're on a voxel surface
        if (gameScene && gameScene->getVoxelSystem()) {
            // Get surface height at current position
            try {
                float surfaceHeight = gameScene->getVoxelSystem()->getSurfaceHeightAt(position.x(), position.z());
                if (surfaceHeight > 0) {
                    groundHeight = surfaceHeight + 0.1f; // Slightly above surface
                    // Check if we're on or very close to the ground
                    isOnGround = (position.y() <= groundHeight + 0.1f);
                }
            } catch (...) {
                // Use default ground height if surface check fails
            }
        }
            
        // Handle jumping physics
        if (jumping) {
            // Apply gravity to jump velocity
            jumpVelocity -= gravity;
            
            // Update vertical position
            newPosition.setY(newPosition.y() + jumpVelocity);
            
            // Check if we've landed
            if (newPosition.y() <= groundHeight) {
                newPosition.setY(groundHeight); 
                jumping = false;
                jumpVelocity = 0.0f;
                isOnGround = true;
            }
        }
        // Apply gravity when not on ground
        else if (!isOnGround) {
            // Apply gravity
            velocity.setY(velocity.y() - gravity);
            
            // Update vertical position
            newPosition.setY(newPosition.y() + velocity.y());
            
            // Check if we've landed
            if (newPosition.y() <= groundHeight) {
                newPosition.setY(groundHeight);
                velocity.setY(0);
                isOnGround = true;
            }
            
            positionHasChanged = true;
        }
        
        // Apply movement changes at speed based on stance
        // bool positionHasChanged = false; // Already declared above
        
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
            
            // Check for collisions with all entities
            bool hasCollision = false;
            if (gameScene) {
                hasCollision = gameScene->checkCollision("player", newPositionBeforeConstraints);
            }
            
            if (!hasCollision) {
                // No collision - free movement
                newPosition = newPositionBeforeConstraints;
            } else {
                // Try X and Z movements separately
                QVector3D xOnlyPosition = QVector3D(newPositionBeforeConstraints.x(), newPosition.y(), newPosition.z());
                QVector3D zOnlyPosition = QVector3D(newPosition.x(), newPosition.y(), newPositionBeforeConstraints.z());
                
                bool xCollision = false, zCollision = false;
                
                if (gameScene) {
                    xCollision = gameScene->checkCollision("player", xOnlyPosition);
                    zCollision = gameScene->checkCollision("player", zOnlyPosition);
                }
                
                // Check which direction we can move in
                if (!xCollision) {
                    newPosition = xOnlyPosition;
                    velocity.setZ(0); // Stop Z movement to prevent sliding
                } else if (!zCollision) {
                    newPosition = zOnlyPosition;
                    velocity.setX(0); // Stop X movement to prevent sliding
                } else {
                    // Can't move along either axis, slow down
                    velocity *= 0.5f;
                }
            }
            
            positionHasChanged = true;
        }
        
        // Apply vertical boundary limit only - prevent falling through floor
        if (newPosition.y() < groundHeight && !jumping) {
            newPosition.setY(groundHeight); // Use our calculated ground height
        }
        
        // Update position if changed
        if (positionHasChanged || jumping) {
            // Final collision check to ensure we don't end up inside a wall
            if (!gameScene || !gameScene->checkCollision("player", newPosition)) {
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