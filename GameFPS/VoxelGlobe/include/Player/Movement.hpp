// ./include/Player/Movement.hpp
#ifndef MOVEMENT_HPP
#define MOVEMENT_HPP

#include <glm/glm.hpp>
#include "World/World.hpp"

/**
 * Handles player movement and physics on a spherical planet
 * Manages movement direction, gravity, and collision detection
 */
class Movement {
public:
    /**
     * Create a new movement controller
     * @param world Reference to the voxel world for collision detection
     * @param pos Reference to player position (will be updated by movement)
     * @param camDir Reference to camera direction
     * @param moveDir Reference to movement direction (on tangent plane)
     * @param up Reference to up vector
     */
    Movement(const World& world, glm::vec3& pos, glm::vec3& camDir, glm::vec3& moveDir, glm::vec3& up);
    
    /**
     * Move player forward along the tangent plane
     * @param deltaTime Time since last frame in seconds
     */
    void moveForward(float deltaTime);
    
    /**
     * Move player backward along the tangent plane
     * @param deltaTime Time since last frame in seconds
     */
    void moveBackward(float deltaTime);
    
    /**
     * Move player left (perpendicular to view direction on tangent plane)
     * @param deltaTime Time since last frame in seconds
     */
    void moveLeft(float deltaTime);
    
    /**
     * Move player right (perpendicular to view direction on tangent plane)
     * @param deltaTime Time since last frame in seconds
     */
    void moveRight(float deltaTime);
    
    /**
     * Apply gravity toward planet center
     * @param deltaTime Time since last frame in seconds
     */
    void applyGravity(float deltaTime);
    
    /**
     * Jump (move away from planet center)
     */
    void jump();
    
    /**
     * Update player orientation based on mouse movement
     * @param deltaX Mouse X movement (negative = left, positive = right)
     * @param deltaY Mouse Y movement (negative = down, positive = up)
     */
    void updateOrientation(float deltaX, float deltaY);
    
    /**
     * Set sprinting state
     * @param sprinting True if player is sprinting
     */
    void setSprinting(bool sprinting);

private:
    const World& world;             // Reference to voxel world
    glm::vec3& position;            // Reference to player position
    glm::vec3& cameraDirection;     // Reference to camera direction
    glm::vec3& movementDirection;   // Reference to movement direction
    glm::vec3& up;                  // Reference to up vector
    float speed = 5.0f;             // Base movement speed in meters/second
    float height = 1.75f;           // Player height in meters
    float verticalVelocity = 0.0f;  // Current vertical velocity
    bool isGrounded = false;        // Is player on the ground
    bool isSprinting = false;       // Is player sprinting
    static constexpr float sprintMultiplier = 2.0f; // Speed multiplier when sprinting
    
    /**
     * Check for collision at the given position
     * @param newPosition Position to check
     * @return True if collision detected
     */
    bool checkCollision(const glm::vec3& newPosition) const;
    
    // Frame counter for debug logging
    int frameCounter = 0;
};

#endif // MOVEMENT_HPP