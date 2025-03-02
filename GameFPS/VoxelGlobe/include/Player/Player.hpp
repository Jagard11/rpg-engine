// ./include/Player/Player.hpp
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Player/Movement.hpp"
#include "UI/Inventory/Inventory.hpp" // Updated include path
#include "World/World.hpp"
#include <GLFW/glfw3.h>

class Player {
public:
    Player(const World& world);
    void update(GLFWwindow* window, float deltaTime);
    float getHeight() const { return 1.75f; }

    glm::vec3 position;
    glm::vec3 cameraDirection;
    glm::vec3 movementDirection;
    glm::vec3 up;

    Inventory inventory;

private:
    const World& world;
    Movement movement;
};

#endif