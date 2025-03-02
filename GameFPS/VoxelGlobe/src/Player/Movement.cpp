// ./GameFPS/VoxelGlobe/src/Player/Movement.cpp
#include "Player/Movement.hpp"
#include <iostream>
#include "Core/Debug.hpp"
#include <glm/gtc/matrix_transform.hpp>

Movement::Movement(const World& w, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& u)
    : world(w), position(pos), cameraDirection(camDir), movementDirection(moveDir), up(u) {}

void Movement::moveForward(float deltaTime) { position += movementDirection * speed * deltaTime; }
void Movement::moveBackward(float deltaTime) { position -= movementDirection * speed * deltaTime; }
void Movement::moveLeft(float deltaTime) {
    glm::vec3 right = glm::normalize(glm::cross(movementDirection, up));
    position -= right * speed * deltaTime;
}
void Movement::moveRight(float deltaTime) {
    glm::vec3 right = glm::normalize(glm::cross(movementDirection, up));
    position += right * speed * deltaTime;
}

void Movement::applyGravity(float deltaTime) {
    glm::vec3 toCenter = (glm::length(position) > 0.001f) ? -glm::normalize(position) : -up;
    float gravity = 9.81f;
    glm::vec3 newPosition = position + toCenter * gravity * deltaTime;

    int chunkX = static_cast<int>(position.x / Chunk::SIZE);
    int chunkZ = static_cast<int>(position.z / Chunk::SIZE);
    chunkX = glm::clamp(chunkX, -1000, 1000);
    chunkZ = glm::clamp(chunkZ, -1000, 1000);
    float terrainHeight = world.findSurfaceHeight(chunkX, chunkZ) + 1.0f + 1.0f;

    position.x = newPosition.x;
    position.z = newPosition.z;
    if (newPosition.y <= terrainHeight) {
        position.y = terrainHeight;
    } else {
        position.y = newPosition.y;
    }
    up = (glm::length(position) > 0.001f) ? glm::normalize(position) : glm::vec3(0.0f, 1.0f, 0.0f);

    if (g_showDebug) {
        std::cout << "terrainHeight: " << terrainHeight << ", newPosition.y: " << newPosition.y << std::endl;
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