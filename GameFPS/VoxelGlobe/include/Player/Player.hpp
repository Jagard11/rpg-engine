// ./include/Player/Player.hpp
#ifndef PLAYER_HPP
#define PLAYER_HPP

// Include GLEW first
#include <GL/glew.h>
// Then include GLFW
#include <GLFW/glfw3.h>

#include "Player/Movement.hpp"
#include "UI/Inventory/Inventory.hpp"
#include "World/World.hpp"

class Player {
public:
    Player(const World& world);
    void update(GLFWwindow* window, float deltaTime);
    float getHeight() const { return 1.75f; }
    const World& getWorld() const { return world; }
    void finishLoading(); // New method to signal loading complete

    // Movement delegation methods
    void moveForward(float deltaTime) { movement.moveForward(deltaTime); }
    void moveBackward(float deltaTime) { movement.moveBackward(deltaTime); }
    void moveLeft(float deltaTime) { movement.moveLeft(deltaTime); }
    void moveRight(float deltaTime) { movement.moveRight(deltaTime); }
    void jump() { movement.jump(); }
    void setSprinting(bool sprinting) { movement.setSprinting(sprinting); }
    void updateOrientation(float deltaX, float deltaY) { movement.updateOrientation(deltaX, deltaY); }

    glm::vec3 position;
    glm::vec3 cameraDirection;
    glm::vec3 movementDirection;
    glm::vec3 up;

    Inventory inventory;

private:
    const World& world;
    Movement movement;
    bool isLoading; // New flag
};

#endif