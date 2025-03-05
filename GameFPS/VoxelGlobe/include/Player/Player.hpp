// ./include/Player/Player.hpp
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Player/Movement.hpp"
#include "UI/Inventory/Inventory.hpp"
#include "World/World.hpp"

/**
 * Represents the player in the voxel world
 * Handles player state, movement, and interaction with the world
 */
class Player {
public:
    /**
     * Create a new player in the given world
     * @param world Reference to the voxel world
     */
    Player(const World& world);
    
    /**
     * Update player state for this frame
     * @param window GLFW window
     * @param deltaTime Time since last frame in seconds
     */
    void update(GLFWwindow* window, float deltaTime);
    
    /**
     * Get the player's height (for eye position calculations)
     * @return Height in meters
     */
    float getHeight() const { return 1.75f; }
    
    /**
     * Get a reference to the world
     * @return Reference to the voxel world
     */
    const World& getWorld() const { return world; }
    
    /**
     * Signal that initial loading is complete
     * Enables physics and other systems
     */
    void finishLoading();

    // Movement delegation methods
    void moveForward(float deltaTime) { movement.moveForward(deltaTime); }
    void moveBackward(float deltaTime) { movement.moveBackward(deltaTime); }
    void moveLeft(float deltaTime) { movement.moveLeft(deltaTime); }
    void moveRight(float deltaTime) { movement.moveRight(deltaTime); }
    void jump() { movement.jump(); }
    void setSprinting(bool sprinting) { movement.setSprinting(sprinting); }
    void updateOrientation(float deltaX, float deltaY) { movement.updateOrientation(deltaX, deltaY); }

    // Player state
    glm::vec3 position;           // Current position in world space
    glm::vec3 cameraDirection;    // Direction player is looking
    glm::vec3 movementDirection;  // Direction player is moving (on tangent plane)
    glm::vec3 up;                 // Up vector (points away from planet center)

    // Player inventory
    Inventory inventory;

private:
    const World& world;           // Reference to the voxel world
    Movement movement;            // Handles player movement and physics
    bool isLoading;               // Flag for initial loading state
};

#endif // PLAYER_HPP