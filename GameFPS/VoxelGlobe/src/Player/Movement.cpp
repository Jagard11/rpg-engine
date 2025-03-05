// ./src/Player/Movement.cpp
#include "Player/Movement.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "Utils/SphereUtils.hpp"

// Constants for collision and ground detection
const float COLLISION_OFFSET = 0.25f;  // Increased from 0.15f to prevent sinking
const float GROUND_OFFSET = 0.3f;      // Increased from 0.2f to keep player higher
const float STEP_HEIGHT = 0.55f;       // Maximum height player can automatically step up
const float PLAYER_RADIUS = 0.4f;      // Player collision radius (slightly smaller than a block)

Movement::Movement(const World& w, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& u)
    : world(w), position(pos), cameraDirection(camDir), movementDirection(moveDir), up(u),
      frameCounter(0) {}

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
    
    // Calculate intended position
    glm::vec3 newPos = position + forwardDir * effectiveSpeed * deltaTime;
    
    // Try offset positions if there's a collision (step up)
    if (checkCollision(newPos)) {
        // Try stepping up at increasing heights
        for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
            glm::vec3 steppedPos = position + forwardDir * effectiveSpeed * deltaTime + up * yOffset;
            if (!checkCollision(steppedPos)) {
                // Found a valid position - use it
                position = steppedPos;
                return;
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
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 newPos = position - forwardDir * effectiveSpeed * deltaTime;
    
    // Try stepping up if there's a collision
    if (checkCollision(newPos)) {
        for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
            glm::vec3 steppedPos = position - forwardDir * effectiveSpeed * deltaTime + up * yOffset;
            if (!checkCollision(steppedPos)) {
                position = steppedPos;
                return;
            }
        }
        
        // Try sliding
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
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
    glm::vec3 newPos = position - rightDir * effectiveSpeed * deltaTime;
    
    // Try stepping up if there's a collision
    if (checkCollision(newPos)) {
        for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
            glm::vec3 steppedPos = position - rightDir * effectiveSpeed * deltaTime + up * yOffset;
            if (!checkCollision(steppedPos)) {
                position = steppedPos;
                return;
            }
        }
        
        // Try sliding
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
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
    glm::vec3 newPos = position + rightDir * effectiveSpeed * deltaTime;
    
    // Try stepping up if there's a collision
    if (checkCollision(newPos)) {
        for (float yOffset = 0.1f; yOffset <= STEP_HEIGHT; yOffset += 0.1f) {
            glm::vec3 steppedPos = position + rightDir * effectiveSpeed * deltaTime + up * yOffset;
            if (!checkCollision(steppedPos)) {
                position = steppedPos;
                return;
            }
        }
        
        // Try sliding
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
    
    if (!isGrounded) {
        // Apply gravity if not on ground, with reduced strength to minimize bouncing
        verticalVelocity += 5.0f * deltaTime; // Reduced from 9.81 for smoother movement
        
        // Calculate fall position
        glm::vec3 newPos = position + gravityDir * verticalVelocity * deltaTime;
        
        // Check for collisions with ground
        if (checkCollision(newPos)) {
            // We hit the ground - find a safe position
            isGrounded = true;
            verticalVelocity = 0.0f;
            
            // Calculate exact surface position - move to further above the surface
            // This prevents sinking and ensures consistent player height
            float targetDistance = surfaceR + GROUND_OFFSET;
            
            // Use exact calculation to position player at precise surface height
            glm::vec3 exactSurfacePos = glm::normalize(position) * targetDistance;
            
            // Check if this position is free from block collisions
            if (!checkCollision(exactSurfacePos)) {
                position = exactSurfacePos;
            } else {
                // If there's a block at exact surface, try to find closest safe position
                for (float offset = 0.1f; offset <= 1.0f; offset += 0.1f) {
                    glm::vec3 testPos = glm::normalize(position) * (targetDistance + offset);
                    if (!checkCollision(testPos)) {
                        position = testPos;
                        break;
                    }
                }
            }
            
            if (DebugManager::getInstance().logCollision()) {
                std::cout << "Landed on ground. New position: " << position.x << ", "
                          << position.y << ", " << position.z 
                          << " (dist from center: " << glm::length(position) << ")" << std::endl;
            }
        } else {
            position = newPos;
        }
    } else {
        // When on ground, check if still grounded
        // Calculate a point below current position to check for ground
        float checkDistance = 0.3f; // Increased check distance
        glm::vec3 testPos = position + gravityDir * checkDistance;
        
        // We check two things: 
        // 1. If we're well above the surface radius
        // 2. If there's no block directly beneath us
        
        bool blockBeneath = checkCollision(testPos);
        bool aboveSurface = (distFromCenter > surfaceR + GROUND_OFFSET * 1.5f);
        
        if (aboveSurface && !blockBeneath) {
            // We're well above the surface and no block is supporting us - not grounded
            isGrounded = false;
            verticalVelocity = 0.1f; // Small initial velocity for smooth start of fall
            
            if (DebugManager::getInstance().logCollision()) {
                std::cout << "No longer grounded. Height above surface: " 
                          << (distFromCenter - surfaceR) << std::endl;
            }
        } else {
            // Keep player at consistent height above surface
            float targetDistance = surfaceR + GROUND_OFFSET;
            
            // Only apply if we're sinking too much
            if (distFromCenter < targetDistance && blockBeneath) {
                // Push player up to target height to prevent sinking into terrain
                glm::vec3 exactSurfacePos = glm::normalize(position) * targetDistance;
                
                // Only use this position if it doesn't cause a block collision
                if (!checkCollision(exactSurfacePos)) {
                    position = exactSurfacePos;
                }
                
                if (DebugManager::getInstance().logCollision() && frameCounter % 120 == 0) {
                    std::cout << "Maintaining ground position. Height above surface: "
                              << (targetDistance - surfaceR) << std::endl;
                }
            }
        }
    }
    
    frameCounter++;
}

void Movement::jump() {
    if (isGrounded) {
        // In sphere world, jumping means moving away from center
        // So negative velocity means moving outward
        verticalVelocity = -5.25f;  // Stronger jump
        isGrounded = false;
        if (DebugManager::getInstance().logPlayerInfo()) {
            std::cout << "Jump initiated, verticalVelocity = " << verticalVelocity << std::endl;
        }
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