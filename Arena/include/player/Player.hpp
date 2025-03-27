#pragma once

#include <glm/glm.hpp>
#include <string>
#include "../world/World.hpp"
#include "../physics/CollisionSystem.hpp"

class Player {
public:
    Player();
    ~Player();

    void update(float deltaTime, World* world);
    void handleInput(float deltaTime, World* world);
    
    // Save and load
    bool saveToFile(const std::string& filepath);
    bool loadFromFile(const std::string& filepath);

    // Movement
    void moveWithCollision(const glm::vec3& movement, World* world);
    void jump();
    void toggleFlying() { m_isFlying = !m_isFlying; }

    // Getters/Setters
    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }
    
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    
    const glm::vec3& getForward() const { return m_forward; }
    const glm::vec3& getRight() const { return m_right; }
    const glm::vec3& getUp() const { return m_up; }
    
    bool isFlying() const { return m_isFlying; }
    bool isJetpackEnabled() const { return m_jetpackEnabled; }
    float getJetpackFuel() const { return m_jetpackFuel; }
    
    // Collision helpers
    glm::vec3 getMinBounds() const;
    glm::vec3 getMaxBounds() const;
    
    // Enable/disable debug mode
    void setDebugMode(bool enabled) { m_collisionSystem.setDebugMode(enabled); }

    // Add a method to ignore the next mouse movement
    void ignoreNextMouseMovement() { m_ignoreNextMouseMovement = true; }

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::vec3 m_up;
    
    float m_yaw;
    float m_pitch;
    float m_moveSpeed;
    float m_jumpForce;
    
    bool m_isOnGround;
    bool m_isFlying;
    bool m_jetpackEnabled;
    float m_jetpackFuel;
    bool m_canDoubleJump;
    
    // Collision box dimensions
    float m_width;
    float m_height;
    
    // Collision system
    CollisionSystem m_collisionSystem;

    // Mouse movement tracking
    bool m_ignoreNextMouseMovement;
    bool m_firstMouse;
    double m_lastX, m_lastY;
}; 