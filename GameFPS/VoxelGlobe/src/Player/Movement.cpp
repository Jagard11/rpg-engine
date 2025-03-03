// ./src/Player/Movement.cpp
#include "Player/Movement.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/gtc/matrix_transform.hpp>

Movement::Movement(const World& w, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& u)
    : world(w), position(pos), cameraDirection(camDir), movementDirection(moveDir), up(u) {}

bool Movement::checkCollision(const glm::vec3& newPos) const {
    int minX = static_cast<int>(floor(newPos.x - 0.25f));
    int maxX = static_cast<int>(floor(newPos.x + 0.25f));
    int minY = static_cast<int>(floor(newPos.y));
    int maxY = static_cast<int>(floor(newPos.y + height));
    int minZ = static_cast<int>(floor(newPos.z - 0.25f));
    int maxZ = static_cast<int>(floor(newPos.z + 0.25f));

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (world.getBlock(x, y, z).type != BlockType::AIR) {
                    if (DebugManager::getInstance().logCollision()) {
                        std::cout << "Collision detected at (" << x << ", " << y << ", " << z << ")" << std::endl;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

void Movement::moveForward(float deltaTime) {
    float effectiveSpeed = speed * (isSprinting ? sprintMultiplier : 1.0f); // Apply sprint multiplier
    glm::vec3 newPos = position + movementDirection * effectiveSpeed * deltaTime;
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}

void Movement::moveBackward(float deltaTime) {
    glm::vec3 newPos = position - movementDirection * speed * deltaTime;
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}

void Movement::moveLeft(float deltaTime) {
    glm::vec3 right = glm::normalize(glm::cross(movementDirection, up));
    glm::vec3 newPos = position - right * speed * deltaTime;
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}

void Movement::moveRight(float deltaTime) {
    glm::vec3 right = glm::normalize(glm::cross(movementDirection, up));
    glm::vec3 newPos = position + right * speed * deltaTime;
    if (!checkCollision(newPos)) {
        position = newPos;
    }
}

void Movement::applyGravity(float deltaTime) {
    float gravity = 9.81f;
    verticalVelocity -= gravity * deltaTime;
    glm::vec3 newPos = position + glm::vec3(0.0f, verticalVelocity * deltaTime, 0.0f);

    int worldX = static_cast<int>(floor(newPos.x));
    int worldZ = static_cast<int>(floor(newPos.z));
    int floorY = static_cast<int>(floor(newPos.y - 0.01f));
    Block blockBelow = world.getBlock(worldX, floorY, worldZ);

    isGrounded = false;
    if (blockBelow.type != BlockType::AIR && verticalVelocity <= 0) {
        newPos.y = static_cast<float>(floorY + 1);
        verticalVelocity = 0.0f;
        isGrounded = true;
        if (DebugManager::getInstance().logPlayerInfo()) {
            std::cout << "Landed on block at y = " << floorY << std::endl;
        }
    } else if (checkCollision(newPos)) {
        verticalVelocity = 0.0f;
        newPos.y = position.y;
        if (DebugManager::getInstance().logCollision()) {
            std::cout << "Hit ceiling or obstacle" << std::endl;
        }
    }

    position = newPos;

    if (DebugManager::getInstance().logPlayerInfo()) {
        std::cout << "Gravity applied, Pos: " << position.x << ", " << position.y << ", " << position.z 
                  << ", Vertical Velocity: " << verticalVelocity << std::endl;
    }
}

void Movement::jump() {
    if (isGrounded) {
        verticalVelocity = 4.85f;
        isGrounded = false;
        if (DebugManager::getInstance().logPlayerInfo()) {
            std::cout << "Jump initiated, verticalVelocity = " << verticalVelocity << std::endl;
        }
    }
}

void Movement::updateOrientation(float deltaX, float deltaY) {
    float deltaYaw = -deltaX * 0.1f;
    float deltaPitch = -deltaY * 0.1f;

    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaYaw), up);
    cameraDirection = glm::vec3(yawRotation * glm::vec4(cameraDirection, 0.0f));
    glm::vec3 right = glm::normalize(glm::cross(cameraDirection, up));
    glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaPitch), right);
    cameraDirection = glm::vec3(pitchRotation * glm::vec4(cameraDirection, 0.0f));
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
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (DebugManager::getInstance().logPlayerInfo()) {
        std::cout << "Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
    }
}

void Movement::setSprinting(bool sprinting) {
    isSprinting = sprinting;
    if (DebugManager::getInstance().logPlayerInfo()) {
        std::cout << "Sprinting: " << (sprinting ? "ON" : "OFF") << std::endl;
    }
}