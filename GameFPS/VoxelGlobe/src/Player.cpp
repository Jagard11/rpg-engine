// ./GameFPS/VoxelGlobe/src/Player.cpp
#include "Player.hpp"
#include <iostream>
#include <ios>
#include "Debug.hpp"
#include <glm/gtc/matrix_transform.hpp>

Player::Player(const World& world) : speed(5.0f), height(1.75f) {
    float surfaceHeight = world.findSurfaceHeight(0, 0); // e.g., 1599.55
    position = glm::vec3(0.0f, surfaceHeight, 0.0f);
    up = glm::normalize(position); // Local up for spherical world

    // Set initial yaw (0°) and pitch (-40° downward)
    yaw = 0.0f;
    pitch = -40.0f; // Negative pitch for downward angle
    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);
    cameraDirection = glm::vec3(
        cos(radPitch) * cos(radYaw), // x ≈ 0.766
        sin(radPitch),               // y ≈ -0.643 (downward)
        cos(radPitch) * sin(radYaw)  // z ≈ 0
    );
    cameraDirection = glm::normalize(cameraDirection);

    // Compute horizontal movementDirection
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (g_showDebug) {
        std::cout << "Initial Player Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Initial Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
        std::cout << "Initial Pitch (deg): " << glm::degrees(asin(cameraDirection.y)) << std::endl;
    }
}

void Player::moveForward(float deltaTime) { 
    position += movementDirection * speed * deltaTime; 
}

void Player::moveBackward(float deltaTime) { 
    position -= movementDirection * speed * deltaTime; 
}

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

    int nextChunkX = static_cast<int>(newPosition.x / Chunk::SIZE);
    int nextChunkZ = static_cast<int>(newPosition.z / Chunk::SIZE);
    nextChunkX = glm::clamp(nextChunkX, -1000, 1000);
    nextChunkZ = glm::clamp(nextChunkZ, -1000, 1000);
    float nextSurfaceY = world.findSurfaceHeight(nextChunkX, nextChunkZ);

    position.x = newPosition.x;
    position.z = newPosition.z;
    if (newPosition.y < nextSurfaceY) {
        position.y = nextSurfaceY;
        up = (glm::length(position) > 0.001f) ? glm::normalize(position) : glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        position.y = newPosition.y;
    }

    if (g_showDebug) {
        std::cout << "Gravity applied, Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Next Surface Y: " << nextSurfaceY << std::endl;
        std::cout << "New Pos Y: " << newPosition.y << std::endl;
        std::cout << "To Center: " << toCenter.x << ", " << toCenter.y << ", " << toCenter.z << std::endl;
    }
}

void Player::updateOrientation(float deltaX, float deltaY) {
    float deltaYaw = -deltaX * 0.1f;  // Mouse left increases yaw (counterclockwise)
    float deltaPitch = -deltaY * 0.1f; // Mouse up looks up

    // Apply yaw: rotate around the local up vector
    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaYaw), up);
    cameraDirection = glm::vec3(yawRotation * glm::vec4(cameraDirection, 0.0f));

    // Compute local right vector
    glm::vec3 right = glm::normalize(glm::cross(cameraDirection, up));

    // Apply pitch: rotate around the local right vector
    glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), glm::radians(deltaPitch), right);
    cameraDirection = glm::vec3(pitchRotation * glm::vec4(cameraDirection, 0.0f));

    // Clamp pitch to avoid flipping (between -89 and 89 degrees)
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

    // Compute horizontal movementDirection
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (g_showDebug) {
        std::cout << "Up: " << up.x << ", " << up.y << ", " << up.z << std::endl;
        std::cout << "Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
        std::cout << "Movement Dir: " << movementDirection.x << ", " << movementDirection.y << ", " << movementDirection.z << std::endl;
    }
}