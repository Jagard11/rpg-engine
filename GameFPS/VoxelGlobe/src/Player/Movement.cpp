// ./src/Player/Movement.cpp
#include "Player/Movement.hpp"
#include <iostream>
#include "Core/Debug.hpp"
#include <glm/gtc/matrix_transform.hpp>

Movement::Movement(const World& w, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& u)
    : world(w), position(pos), cameraDirection(camDir), movementDirection(moveDir), up(u) {}

bool Movement::checkCollision(const glm::vec3& newPos) const {
    // Check player's bounding box (simplified: 0.5 width, 1.75 height)
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
                    if (g_showDebug) {
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
    glm::vec3 newPos = position + movementDirection * speed * deltaTime;
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
    glm::vec3 newPos = position - glm::vec3(0.0f, gravity * deltaTime, 0.0f);

    // Check block directly beneath player's feet
    int worldX = static_cast<int>(floor(newPos.x));
    int worldZ = static_cast<int>(floor(newPos.z));
    int floorY = static_cast<int>(floor(newPos.y - 0.01f)); // Slightly below feet
    Block blockBelow = world.getBlock(worldX, floorY, worldZ);

    if (blockBelow.type != BlockType::AIR) {
        newPos.y = static_cast<float>(floorY + 1); // Stand on block
        if (g_showDebug) std::cout << "Landed on block at y = " << floorY << std::endl;
    }

    position = newPos;

    if (g_showDebug) {
        std::cout << "Gravity applied, Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Eye Pos (pos.y + height): " << position.x << ", " << (position.y + height) << ", " << position.z << std::endl;
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

    if (g_showDebug) {
        std::cout << "Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
    }
}