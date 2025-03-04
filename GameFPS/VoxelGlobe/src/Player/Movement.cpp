// ./src/Player/Movement.cpp
#include "Player/Movement.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/gtc/matrix_transform.hpp>

Movement::Movement(const World& w, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& u)
    : world(w), position(pos), cameraDirection(camDir), movementDirection(moveDir), up(u) {}

bool Movement::checkCollision(const glm::vec3& newPosition) const {
    // Add a slightly larger buffer below for ground detection
    float heightOffset = 0.05f; // Small offset to prevent sinking
    
    // Calculate a bounding box around the player's position
    int minX = static_cast<int>(floor(newPosition.x - 0.25f));
    int maxX = static_cast<int>(floor(newPosition.x + 0.25f));
    int minY = static_cast<int>(floor(newPosition.y - heightOffset)); // Check slightly below
    int maxY = static_cast<int>(floor(newPosition.y + height));
    int minZ = static_cast<int>(floor(newPosition.z - 0.25f));
    int maxZ = static_cast<int>(floor(newPosition.z + 0.25f));

    // Check all voxels in the bounding box for collision
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                Block block = world.getBlock(x, y, z);
                if (block.type != BlockType::AIR) {
                    return true;
                }
            }
        }
    }
    return false;
}


void Movement::moveForward(float deltaTime) {
    // Get effective speed based on sprint state
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    
    // Get forward direction in the tangent plane of the sphere
    // This is crucial for spherical world movement
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    
    // Calculate intended position
    glm::vec3 newPos = position + forwardDir * effectiveSpeed * deltaTime;
    
    // Try offset position slightly higher if there's a collision
    if (checkCollision(newPos)) {
        // Try moving slightly upward to step over small obstacles
        for (float yOffset = 0.1f; yOffset <= 0.4f; yOffset += 0.1f) {
            glm::vec3 steppedPos = position + forwardDir * effectiveSpeed * deltaTime + glm::vec3(0, yOffset, 0);
            if (!checkCollision(steppedPos)) {
                position = steppedPos;
                return;
            }
        }
    } else {
        // No collision detected, safe to move
        position = newPos;
    }
}

// Similar improvements for other movement methods
void Movement::moveBackward(float deltaTime) {
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 newPos = position - forwardDir * effectiveSpeed * deltaTime;
    
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}

void Movement::moveLeft(float deltaTime) {
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
    glm::vec3 newPos = position - rightDir * effectiveSpeed * deltaTime;
    
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}

void Movement::moveRight(float deltaTime) {
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f);
    glm::vec3 forwardDir = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
    glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));
    glm::vec3 newPos = position + rightDir * effectiveSpeed * deltaTime;
    
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}



void Movement::applyGravity(float deltaTime) {
    // Calculate gravity direction (toward planet center)
    glm::vec3 gravityDir = -glm::normalize(position);
    
    if (!isGrounded) {
        // Apply gravity if not on ground
        verticalVelocity += 9.81f * deltaTime;
        
        // Calculate fall position
        glm::vec3 newPos = position + gravityDir * verticalVelocity * deltaTime;
        
        // Check for collisions with ground
        if (checkCollision(newPos)) {
            // We hit the ground - find a safe position
            isGrounded = true;
            verticalVelocity = 0.0f;
            
            // Push player up slightly to prevent sinking
            position = position - gravityDir * 0.05f;
        } else {
            position = newPos;
        }
    } else {
        // When on ground, check if still grounded
        glm::vec3 testPos = position + gravityDir * 0.1f;
        if (!checkCollision(testPos)) {
            isGrounded = false;
        }
    }
}


void Movement::jump() {
    if (isGrounded) {
        // In sphere world, jumping means moving away from center
        // So negative velocity means moving outward
        verticalVelocity = -4.85f;
        isGrounded = false;
        if (DebugManager::getInstance().logPlayerInfo()) {
            std::cout << "Jump initiated, verticalVelocity = " << verticalVelocity << std::endl;
        }
    }
}

void Movement::updateOrientation(float deltaX, float deltaY) {
    // Convert mouse movement to rotation angles
    float deltaYaw = -deltaX * 0.1f;
    float deltaPitch = -deltaY * 0.1f;  // deltaY is now flipped in Player.cpp
    
    // Create rotation matrices
    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaYaw), up);
    cameraDirection = glm::vec3(yawRotation * glm::vec4(cameraDirection, 0.0f));
    
    // Calculate right vector
    glm::vec3 right = glm::normalize(glm::cross(cameraDirection, up));
    
    // Apply pitch rotation
    glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaPitch), right);
    cameraDirection = glm::normalize(glm::vec3(pitchRotation * glm::vec4(cameraDirection, 0.0f)));
    
    // Limit pitch to avoid gimbal lock
    float currentPitch = glm::degrees(asin(glm::dot(cameraDirection, up)));
    if (currentPitch > 89.0f) {
        float adjustment = 89.0f - currentPitch;
        glm::mat4 adjustRotation = glm::rotate(glm::mat4(1.0f), glm::radians(adjustment), right);
        cameraDirection = glm::vec3(adjustRotation * glm::vec4(cameraDirection, 0.0f));
    } else if (currentPitch < -89.0f) {
        float adjustment = -89.0f - currentPitch;
        glm::mat4 adjustRotation = glm::rotate(glm::mat4(1.0f), glm::radians(adjustment), right);
        cameraDirection = glm::vec3(adjustRotation * glm::vec4(cameraDirection, 0.0f));
    }
    
    // Update movement direction (projected onto the tangent plane of the sphere)
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (DebugManager::getInstance().logPlayerInfo()) {
        static int logCounter = 0;
        if (++logCounter % 120 == 0) { // Log less frequently to avoid spam
            std::cout << "Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
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