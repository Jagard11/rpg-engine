// ./src/Player/Movement.cpp
#include "Player/Movement.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "Utils/SphereUtils.hpp"

// Constants for collision and ground detection
const float COLLISION_OFFSET = 0.25f;      // Distance offset for collision detection
const float GROUND_OFFSET = 0.3f;          // Height above surface for grounded state
const float STEP_HEIGHT = 0.55f;           // Maximum height player can automatically step up
const float PLAYER_RADIUS = 0.4f;          // Player collision radius

// Physics constants
const float GRAVITY_ACCELERATION = 9.81f;  // Standard gravity acceleration (m/s²)
const float JUMP_IMPULSE = 10.0f;         // Direct upward impulse force for jumps
const float TERMINAL_VELOCITY = 53.0f;     // Terminal velocity (m/s)
const float AIR_CONTROL = 0.25f;           // Factor for air control (0-1)
const float DRAG_FACTOR = 0.02f;           // Air resistance factor

Movement::Movement(const World& w, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& u)
    : world(w), position(pos), cameraDirection(camDir), movementDirection(moveDir), up(u),
      frameCounter(0), verticalVelocity(0.0f), isGrounded(true), lateralVelocity(0.0f, 0.0f, 0.0f) {}

bool Movement::checkCollision(const glm::vec3& newPosition) const {
    // Get surface radius using standardized method
    float surfaceR = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
    
    // Calculate distance from center with double precision for accuracy
    double px = static_cast<double>(newPosition.x);
    double py = static_cast<double>(newPosition.y);
    double pz = static_cast<double>(newPosition.z);
    double distFromCenter = sqrt(px*px + py*py + pz*pz);
    
    // If we're below the surface collision radius, we've collided
    if (distFromCenter < SphereUtils::getCollisionRadiusMeters()) {
        if (DebugManager::getInstance().logCollision()) {
            std::cout << "Surface collision detected - dist: " << distFromCenter 
                      << ", surface at: " << surfaceR 
                      << ", collision radius: " << SphereUtils::getCollisionRadiusMeters() << std::endl;
        }
        return true;
    }
    
    // Check for collisions with specific blocks within bounding box
    // We need to check multiple points to ensure no slipping through cracks
    
    // Generate test points at player's body corners and edges
    const float VERTICAL_OFFSET = 0.1f; // Check slightly below and above to catch all collisions
    std::vector<glm::vec3> testPoints;
    
    // Calculate up vector at player position (pointing away from planet center)
    glm::vec3 upDir = glm::normalize(newPosition);
    
    // Calculate perpendicular axes for the bounding box
    glm::vec3 rightDir = glm::normalize(glm::cross(cameraDirection, upDir));
    glm::vec3 forwardDir = glm::normalize(glm::cross(upDir, rightDir));
    
    // Add test points at feet level
    testPoints.push_back(newPosition + upDir * VERTICAL_OFFSET); // Center
    testPoints.push_back(newPosition + rightDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET); // Right
    testPoints.push_back(newPosition - rightDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET); // Left
    testPoints.push_back(newPosition + forwardDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET); // Forward
    testPoints.push_back(newPosition - forwardDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET); // Back
    
    // Add diagonal points
    testPoints.push_back(newPosition + rightDir * PLAYER_RADIUS + forwardDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET);
    testPoints.push_back(newPosition + rightDir * PLAYER_RADIUS - forwardDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET);
    testPoints.push_back(newPosition - rightDir * PLAYER_RADIUS + forwardDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET);
    testPoints.push_back(newPosition - rightDir * PLAYER_RADIUS - forwardDir * PLAYER_RADIUS + upDir * VERTICAL_OFFSET);
    
    // Check if any test point is inside a solid block
    for (const auto& testPoint : testPoints) {
        int blockX = static_cast<int>(floor(testPoint.x));
        int blockY = static_cast<int>(floor(testPoint.y));
        int blockZ = static_cast<int>(floor(testPoint.z));
        
        // Check if this position contains a non-air block
        Block block = world.getBlock(blockX, blockY, blockZ);
        if (block.type != BlockType::AIR) {
            if (DebugManager::getInstance().logCollision()) {
                std::cout << "Block collision detected at (" << blockX << ", " << blockY << ", " << blockZ
                          << ") - block type: " << static_cast<int>(block.type) << std::endl;
            }
            return true;
        }
    }
    
    // No collision detected
    return false;
}

void Movement::moveForward(float deltaTime) {
    // Get effective speed based on sprint state
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    
    // Get forward direction in the tangent plane of the sphere
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    
    // Reduced control in the air - only a quarter of normal influence
    if (!isGrounded) {
        effectiveSpeed *= 0.25f; // 25% of normal speed while airborne
    }
    
    // Calculate intended position
    glm::vec3 newPos = position + forwardDir * effectiveSpeed * deltaTime;
    
    // Try offset positions if there's a collision (step up)
    if (checkCollision(newPos)) {
        // Only allow stepping up if grounded
        if (isGrounded) {
            // Try stepping up at increasing heights
            for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
                glm::vec3 steppedPos = position + forwardDir * effectiveSpeed * deltaTime + up * yOffset;
                if (!checkCollision(steppedPos)) {
                    // Found a valid position - use it
                    position = steppedPos;
                    return;
                }
            }
        }
        
        // If we can't step up, try sliding along the collision surface
        // Project movement onto the collision surface
        glm::vec3 slideDir = glm::normalize(forwardDir - glm::dot(forwardDir, up) * up);
        glm::vec3 slidePos = position + slideDir * effectiveSpeed * deltaTime;
        
        if (!checkCollision(slidePos)) {
            position = slidePos;
            return;
        }
    } else {
        // No collision detected, safe to move
        position = newPos;
    }
}

void Movement::moveBackward(float deltaTime) {
    // Get effective speed based on sprint state
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    
    // Reduced control in the air - only a quarter of normal influence
    if (!isGrounded) {
        effectiveSpeed *= 0.25f; // 25% of normal speed while airborne
    }
    
    // Get forward direction in the tangent plane of the sphere
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    
    // Calculate intended position
    glm::vec3 newPos = position - forwardDir * effectiveSpeed * deltaTime;
    
    // Try stepping up if there's a collision (only when grounded)
    if (checkCollision(newPos)) {
        if (isGrounded) {
            // Try stepping up (only if grounded)
            for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
                glm::vec3 steppedPos = position - forwardDir * effectiveSpeed * deltaTime + up * yOffset;
                if (!checkCollision(steppedPos)) {
                    position = steppedPos;
                    return;
                }
            }
        }
        
        // Try sliding along the surface
        glm::vec3 slideDir = glm::normalize(forwardDir - glm::dot(forwardDir, up) * up);
        glm::vec3 slidePos = position - slideDir * effectiveSpeed * deltaTime;
        
        if (!checkCollision(slidePos)) {
            position = slidePos;
        }
    } else {
        position = newPos;
    }
}

void Movement::moveLeft(float deltaTime) {
    // Get effective speed based on sprint state
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    
    // Reduced control in the air - only a quarter of normal influence
    if (!isGrounded) {
        effectiveSpeed *= 0.25f; // 25% of normal speed while airborne
    }
    
    // Calculate direction vectors
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
    
    // Calculate intended position
    glm::vec3 newPos = position - rightDir * effectiveSpeed * deltaTime;
    
    // Try stepping up if there's a collision (only when grounded)
    if (checkCollision(newPos)) {
        if (isGrounded) {
            // Try stepping up (only if grounded)
            for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
                glm::vec3 steppedPos = position - rightDir * effectiveSpeed * deltaTime + up * yOffset;
                if (!checkCollision(steppedPos)) {
                    position = steppedPos;
                    return;
                }
            }
        }
        
        // Try sliding along the surface
        glm::vec3 slideDir = glm::normalize(rightDir - glm::dot(rightDir, up) * up);
        glm::vec3 slidePos = position - slideDir * effectiveSpeed * deltaTime;
        
        if (!checkCollision(slidePos)) {
            position = slidePos;
        }
    } else {
        position = newPos;
    }
}

void Movement::moveRight(float deltaTime) {
    // Get effective speed based on sprint state
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    
    // Reduced control in the air - only a quarter of normal influence
    if (!isGrounded) {
        effectiveSpeed *= 0.25f; // 25% of normal speed while airborne
    }
    
    // Calculate direction vectors
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
    
    // Calculate intended position
    glm::vec3 newPos = position + rightDir * effectiveSpeed * deltaTime;
    
    // Try stepping up if there's a collision (only when grounded)
    if (checkCollision(newPos)) {
        if (isGrounded) {
            // Try stepping up (only if grounded)
            for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
                glm::vec3 steppedPos = position + rightDir * effectiveSpeed * deltaTime + up * yOffset;
                if (!checkCollision(steppedPos)) {
                    position = steppedPos;
                    return;
                }
            }
        }
        
        // Try sliding along the surface
        glm::vec3 slideDir = glm::normalize(rightDir - glm::dot(rightDir, up) * up);
        glm::vec3 slidePos = position + slideDir * effectiveSpeed * deltaTime;
        
        if (!checkCollision(slidePos)) {
            position = slidePos;
        }
    } else {
        position = newPos;
    }
}

void Movement::applyGravity(float deltaTime) {
    // Calculate gravity direction (toward planet center)
    glm::vec3 gravityDir = -glm::normalize(position);
    
    // Get surface radius using standardized method
    float surfaceR = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
    
    // Calculate distance from center with double precision for accuracy
    double px = static_cast<double>(position.x);
    double py = static_cast<double>(position.y);
    double pz = static_cast<double>(position.z);
    double distFromCenter = sqrt(px*px + py*py + pz*pz);
    
    // Print debug info every 60 frames
    bool debugFrame = (frameCounter % 60 == 0);
    
    if (debugFrame) {
        std::cout << "GRAVITY: isGrounded=" << isGrounded 
                  << ", verticalVelocity=" << verticalVelocity
                  << ", height=" << (distFromCenter - surfaceR) << "m" << std::endl;
    }
    
    // If we're airborne (in a jump or falling)
    if (!isGrounded) {
        // Apply gravity acceleration
        verticalVelocity += GRAVITY_ACCELERATION * deltaTime;
        
        // Apply air drag (gradually reduces lateral velocity)
        if (glm::length(lateralVelocity) > 0.01f) {
            lateralVelocity -= lateralVelocity * DRAG_FACTOR * deltaTime;
        }
        
        // Clamp to terminal velocity
        if (verticalVelocity > TERMINAL_VELOCITY) {
            verticalVelocity = TERMINAL_VELOCITY;
        }
        
        // Calculate lateral direction in tangent plane
        glm::vec3 lateralDir = glm::vec3(0.0f);
        float lateralSpeed = glm::length(lateralVelocity);
        
        if (lateralSpeed > 0.01f) {
            // Make sure lateral movement is perpendicular to up vector
            lateralDir = glm::normalize(lateralVelocity - glm::dot(lateralVelocity, up) * up);
        }
        
        // Calculate new position with both vertical and lateral movement
        glm::vec3 newPos = position;
        
        // Apply lateral movement first
        if (lateralSpeed > 0.01f) {
            newPos += lateralDir * lateralSpeed * deltaTime;
        }
        
        // Apply vertical movement separately
        glm::vec3 verticalMovement = gravityDir * verticalVelocity * deltaTime;
        glm::vec3 verticalOnlyPos = position + verticalMovement;
        
        // Test if vertical movement alone would cause collision
        bool verticalCollision = checkCollision(verticalOnlyPos);
        
        // Apply full movement if no collision, otherwise handle separately
        if (!verticalCollision) {
            newPos += verticalMovement;
        }
        
        // Check for collisions with the combined movement
        bool finalCollision = checkCollision(newPos);
        
        if (finalCollision || verticalCollision) {
            // Debug output
            if (verticalCollision) {
                std::cout << "VERTICAL COLLISION DETECTED during jump/fall" << std::endl;
            }
            
            if (finalCollision) {
                std::cout << "FINAL COLLISION DETECTED during jump/fall" << std::endl;
            }
            
            // We hit the ground - transition to grounded state
            isGrounded = true;
            verticalVelocity = 0.0f;
            lateralVelocity = glm::vec3(0.0f);
            
            // Calculate exact surface position
            float targetDistance = surfaceR + GROUND_OFFSET;
            
            // Position player at precise surface height
            glm::vec3 exactSurfacePos = glm::normalize(position) * targetDistance;
            
            // Check if this position is free from block collisions
            if (!checkCollision(exactSurfacePos)) {
                position = exactSurfacePos;
                if (debugFrame) {
                    std::cout << "LANDING: Placed at exact surface height" << std::endl;
                }
            } else {
                // If there's a block at exact surface, try to find closest safe position
                bool foundSafePos = false;
                for (float offset = 0.1f; offset <= 1.0f; offset += 0.1f) {
                    glm::vec3 testPos = glm::normalize(position) * (targetDistance + offset);
                    if (!checkCollision(testPos)) {
                        position = testPos;
                        foundSafePos = true;
                        if (debugFrame) {
                            std::cout << "LANDING: Placed at height +" << offset << "m above surface" << std::endl;
                        }
                        break;
                    }
                }
                
                if (!foundSafePos) {
                    // If we couldn't find a safe position, just stay where we are
                    std::cout << "WARNING: Couldn't find safe landing position" << std::endl;
                }
            }
            
            std::cout << "Landed on ground. New position: " << position.x << ", "
                      << position.y << ", " << position.z 
                      << " (dist from center: " << glm::length(position) << ")" << std::endl;
        } else {
            // No collision, can move to the new position
            position = newPos;
            
            // Log details during air movement
            if (debugFrame) {
                float height = glm::length(position) - surfaceR;
                std::cout << "AIRBORNE: height=" << height 
                         << "m, verticalVel=" << verticalVelocity 
                         << ", lateralVel=" << lateralSpeed << std::endl;
            }
        }
    } else {
        // Currently on ground - check if we should transition to airborne
        // Check if there's ground beneath us
        float checkDistance = 0.3f;
        glm::vec3 testPos = position + gravityDir * checkDistance;
        
        bool blockBeneath = checkCollision(testPos);
        bool wellAboveSurface = (distFromCenter > surfaceR + GROUND_OFFSET * 1.5f);
        
        // Debug output
        if (debugFrame) {
            std::cout << "GROUND CHECK: blockBeneath=" << blockBeneath 
                     << ", wellAboveSurface=" << wellAboveSurface 
                     << ", height=" << (distFromCenter - surfaceR) << "m" << std::endl;
        }
        
        // If we're above the surface and there's no block beneath, we should fall
        if (wellAboveSurface && !blockBeneath) {
            // Transition to falling state
            isGrounded = false;
            verticalVelocity = 0.1f; // Small initial velocity for fall
            std::cout << "FALLING: No longer grounded. Height: " 
                     << (distFromCenter - surfaceR) << "m" << std::endl;
        } else {
            // Keep player at consistent height above surface
            float targetDistance = surfaceR + GROUND_OFFSET;
            
            // Only push up if we're too low
            if (distFromCenter < targetDistance && blockBeneath) {
                // Push player up to target height
                glm::vec3 exactSurfacePos = glm::normalize(position) * targetDistance;
                
                // Only use this position if it doesn't cause a block collision
                if (!checkCollision(exactSurfacePos)) {
                    position = exactSurfacePos;
                    if (debugFrame) {
                        std::cout << "ADJUSTED: Maintaining ground height" << std::endl;
                    }
                }
            }
        }
    }
    
    frameCounter++;
}

void Movement::jump() {
    if (isGrounded) {
        // Debug output to confirm jump is triggered
        std::cout << "Jump triggered! isGrounded = " << isGrounded << std::endl;
        
        // Set player to airborne state immediately
        isGrounded = false;
        
        // Calculate up direction (away from planet center)
        glm::vec3 upDir = glm::normalize(position);
        
        // Apply direct upward impulse - immediately move the player upward
        position += upDir * JUMP_IMPULSE;
        
        // Set initial upward velocity to zero - we'll let gravity handle the physics
        verticalVelocity = 0.0f;
        
        // Log details
        std::cout << "JUMP: Applied direct upward impulse of " << JUMP_IMPULSE << " meters" << std::endl;
        std::cout << "JUMP: New position is " << position.x << ", " << position.y << ", " << position.z << std::endl;
        
        // Calculate height above surface
        float surfaceR = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
        double newDistFromCenter = glm::length(position);
        double newHeight = newDistFromCenter - surfaceR;
        
        std::cout << "JUMP: New height above surface: " << newHeight << " meters" << std::endl;
    } else {
        // Debug output if jump is attempted while in the air
        std::cout << "Jump attempted but player is not grounded" << std::endl;
    }
}

void Movement::updateOrientation(float deltaX, float deltaY) {
    // Ensure up vector is normalized for stable rotations
    up = glm::normalize(up);
    
    // Set yaw and pitch changes
    // For yaw (horizontal), negative deltaX means look right
    float deltaYaw = -deltaX * 0.1f;
    
    // For pitch (vertical), POSITIVE deltaY means look UP
    // This is the correct orientation - moving mouse up should look up
    float deltaPitch = deltaY * 0.1f;
    
    // Calculate right vector and current pitch
    glm::vec3 right = glm::normalize(glm::cross(cameraDirection, up));
    float currentPitch = glm::degrees(asin(glm::dot(cameraDirection, up)));
    
    // Apply yaw rotation around up vector
    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaYaw), up);
    cameraDirection = glm::normalize(glm::vec3(yawRotation * glm::vec4(cameraDirection, 0.0f)));
    
    // Update right vector after yaw rotation
    right = glm::normalize(glm::cross(cameraDirection, up));
    
    // Only apply pitch if within limits
    float newPitch = currentPitch + deltaPitch;
    if (newPitch <= 85.0f && newPitch >= -85.0f) {
        glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaPitch), right);
        cameraDirection = glm::normalize(glm::vec3(pitchRotation * glm::vec4(cameraDirection, 0.0f)));
    }
    
    // Store the full camera direction for looking
    // But ensure movement direction stays on the tangent plane
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (DebugManager::getInstance().logPlayerInfo()) {
        static int logCounter = 0;
        if (++logCounter % 120 == 0) { // Log less frequently to avoid spam
            std::cout << "Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
            std::cout << "Up Vector: " << up.x << ", " << up.y << ", " << up.z << std::endl;
        }
    }
}

void Movement::setSprinting(bool sprinting) {
    isSprinting = sprinting;
    if (DebugManager::getInstance().logPlayerInfo()) {
        static bool lastSprintState = false;
        if (isSprinting != lastSprintState) {
            std::cout << "Sprinting: " << (sprinting ? "ON" : "OFF") << std::endl;
            lastSprintState = isSprinting;
        }
    }
}

// Get ground state for UI/animations
bool Movement::isPlayerGrounded() const {
    return isGrounded;
}

// Get fall velocity for UI/animations
float Movement::getVerticalVelocity() const {
    return verticalVelocity;
}