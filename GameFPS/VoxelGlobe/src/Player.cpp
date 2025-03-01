// ./GameFPS/VoxelGlobe/src/Player.cpp
#include "Player.hpp"
#include <iostream>
#include "Debug.hpp"
#include <glm/gtc/matrix_transform.hpp>

Player::Player(const World& world) : speed(5.0f), height(1.75f) {
    position = glm::vec3(0.0f, 1640.0f, 0.0f); // Adjusted spawn height
    up = glm::normalize(position);
    yaw = 0.0f;
    pitch = 45.0f; // Look downward at 45 degrees
    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);
    cameraDirection = glm::vec3(cos(radPitch) * cos(radYaw), sin(radPitch), cos(radPitch) * sin(radYaw));
    cameraDirection = glm::normalize(cameraDirection);
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (g_showDebug) {
        std::cout << "Initial Player Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Initial Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
    }
}

void Player::moveForward(float deltaTime) { position += movementDirection * speed * deltaTime; }
void Player::moveBackward(float deltaTime) { position -= movementDirection * speed * deltaTime; }
void Player::moveLeft(float deltaTime) {
    glm::vec3 right = glm::normalize(glm::cross(movementDirection, up));
    position -= right * speed * deltaTime;
}
void Player::moveRight(float deltaTime) {
    glm::vec3 right = glm::normalize(glm::cross(movementDirection, up));
    position += right * speed * deltaTime;
}

void Player::applyGravity(const World& world, float deltaTime) {
    glm::vec3 toCenter = (glm::length(position) > 0.001f) ? -glm::normalize(position) : -up;
    float gravity = 9.81f;
    glm::vec3 newPosition = position + toCenter * gravity * deltaTime;

    int chunkX = static_cast<int>(position.x / Chunk::SIZE); // Use current position
    int chunkZ = static_cast<int>(position.z / Chunk::SIZE); // Use current position
    chunkX = glm::clamp(chunkX, -1000, 1000);
    chunkZ = glm::clamp(chunkZ, -1000, 1000);
    float terrainHeight = world.findSurfaceHeight(chunkX, chunkZ) + 9.0f; // Adjusted to match top face at radius + 17

    position.x = newPosition.x;
    position.z = newPosition.z;
    if (newPosition.y <= terrainHeight) {
        position.y = terrainHeight; // Snap feet to the top of the terrain
    } else {
        position.y = newPosition.y; // Continue falling
    }
    up = (glm::length(position) > 0.001f) ? glm::normalize(position) : glm::vec3(0.0f, 1.0f, 0.0f);

    if (g_showDebug) {
        std::cout << "terrainHeight: " << terrainHeight << ", newPosition.y: " << newPosition.y << std::endl;
        std::cout << "Gravity applied, Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Eye Pos (pos.y + height): " << position.x << ", " << (position.y + height) << ", " << position.z << std::endl;
    }
}

void Player::updateOrientation(float deltaX, float deltaY) {
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

void Player::scrollInventory(float delta) {
    if (delta > 0) {
        selectedSlot = (selectedSlot + 1) % 10;
    } else if (delta < 0) {
        selectedSlot = (selectedSlot - 1 + 10) % 10;
    }
    if (g_showDebug) {
        std::cout << "Selected Slot: " << selectedSlot << " (" << static_cast<int>(inventory[selectedSlot]) << ")" << std::endl;
    }
}