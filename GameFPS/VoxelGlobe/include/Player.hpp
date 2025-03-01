#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <glm/glm.hpp>
#include "World.hpp"

class Player {
public:
    Player(const World& world);
    glm::vec3 position;
    glm::vec3 cameraDirection;  // Direction the camera faces (allows vertical rotation)
    glm::vec3 movementDirection; // Direction for horizontal movement
    glm::vec3 up;
    float height = 1.75f;
    float speed = 5.0f;
    float yaw = 0.0f;   // Horizontal rotation (degrees)
    float pitch = 0.0f; // Vertical rotation (degrees)

    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);
    void applyGravity(const World& world, float deltaTime);
    void updateOrientation(float deltaX, float deltaY); // Mouse input
};

#endif