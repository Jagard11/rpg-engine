#include "Player.hpp"
#include <iostream>
#include "Debug.hpp"

Player::Player(const World& world) : speed(5.0f) {
    float surfaceY = world.findSurfaceHeight(0, 0); // 1599.55
    position = glm::vec3(0, surfaceY, 0); // Feet at terrain top
    direction = glm::vec3(0, -1, 0); // Look down initially
    updateOrientation(0, 0);
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

void Player::applyGravity(float deltaTime, const World& world) {
    glm::vec3 toCenter = -glm::normalize(position);
    float gravity = 9.81f;
    position += toCenter * gravity * deltaTime;

    float surfaceY = world.findSurfaceHeight(
        static_cast<int>(position.x / Chunk::SIZE),
        static_cast<int>(position.z / Chunk::SIZE)
    );
    if (position.y < surfaceY) {
        position.y = surfaceY; // Feet snap to surface
    }
    if (g_showDebug) std::cout << "Gravity applied, Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
}

void Player::updateOrientation(float deltaX, float deltaY) {
    up = glm::normalize(position);
    
    yaw += deltaX * 0.1f;
    pitch += deltaY * 0.1f; // Fixed pitch direction
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