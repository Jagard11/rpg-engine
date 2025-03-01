#include "Player.hpp"
#include <iostream>
#include <ios> // For streamsize
#include "Debug.hpp"
#include <glm/gtc/type_ptr.hpp>

Player::Player(const World& world) : speed(5.0f) {
    float surfaceHeight = world.findSurfaceHeight(0, 0); // 1599.55
    position = glm::vec3(0.0f, surfaceHeight, 0.0f);
    direction = glm::vec3(0.0f, -1.0f, 0.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    updateOrientation(0, 0);
    position.x = 0.0f;
    position.z = 0.0f;

    if (g_showDebug) {
        std::cout << "Initial Player Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
    }
}

void Player::applyGravity(float deltaTime, const World& world) {
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

void Player::moveForward(float deltaTime) { 
    position += direction * speed * deltaTime; 
}

void Player::moveBackward(float deltaTime) { 
    position -= direction * speed * deltaTime; 
}

void Player::moveLeft(float deltaTime) { 
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    position -= right * speed * deltaTime; 
}

void Player::moveRight(float deltaTime) { 
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    position += right * speed * deltaTime; 
}

void Player::updateOrientation(float deltaX, float deltaY) {
    yaw += deltaX * 0.1f;
    pitch += deltaY * 0.1f;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    direction = glm::normalize(glm::vec3(
        cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
        sin(glm::radians(pitch)),
        cos(glm::radians(pitch)) * sin(glm::radians(yaw))
    ));
    direction = glm::normalize(direction - glm::dot(direction, up) * up);

    if (g_showDebug) {
        std::cout << "Up: " << up.x << ", " << up.y << ", " << up.z << std::endl;
        std::cout << "Dir: " << direction.x << ", " << direction.y << ", " << direction.z << std::endl;
    }
}