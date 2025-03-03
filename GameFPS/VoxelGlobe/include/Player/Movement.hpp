// ./include/Player/Movement.hpp
#ifndef MOVEMENT_HPP
#define MOVEMENT_HPP

#include <glm/glm.hpp>
#include "World/World.hpp"

class Movement {
public:
    Movement(const World& world, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& up);
    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);
    void applyGravity(float deltaTime);
    void jump(); // New method
    void updateOrientation(float deltaX, float deltaY);

private:
    const World& world;
    glm::vec3& position;
    glm::vec3& cameraDirection;
    glm::vec3& movementDirection;
    glm::vec3& up;
    float speed = 5.0f;
    float height = 1.75f;
    float verticalVelocity = 0.0f; // New variable for vertical movement
    bool isGrounded = false; // Track if player is on ground
    bool checkCollision(const glm::vec3& newPos) const;
};

#endif