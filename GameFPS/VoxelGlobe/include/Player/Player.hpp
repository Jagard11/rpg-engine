// ./GameFPS/VoxelGlobe/include/Player/Player.hpp
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Player/Movement.hpp"
#include "Player/Inventory.hpp"
#include "World/World.hpp"
#include <GLFW/glfw3.h>

class Player {
public:
    Player(const World& world);
    void update(GLFWwindow* window, float deltaTime);
    float getHeight() const { return 1.75f; } // Added for Renderer; could delegate to Movement if exposed

    glm::vec3 position;
    glm::vec3 cameraDirection;
    glm::vec3 movementDirection;
    glm::vec3 up;

    Inventory inventory; // Public for now, could encapsulate later

private:
    const World& world;
    Movement movement;
};

#endif