#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include "../world/World.hpp"
#include "../world/Chunk.hpp"

class CollisionSystem {
public:
    CollisionSystem();
    
    // Initialize with dimensions of the entity
    void init(float width, float height);
    
    // Primary collision detection methods
    bool collidesWithBlocks(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision = false);
    bool collidesWithBlocksGreedy(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision = false);
    bool checkGroundCollision(const glm::vec3& pos, const glm::vec3& velocity, World* world);
    bool isPositionSafe(const glm::vec3& pos, World* world);
    
    // Perform movement with collision detection
    glm::vec3 moveWithCollision(const glm::vec3& currentPos, const glm::vec3& movement, const glm::vec3& velocity, 
                              World* world, bool isFlying, bool& isOnGround);
    
    // Helper functions for collision box calculations
    glm::vec3 getMinBounds(const glm::vec3& pos) const;
    glm::vec3 getMaxBounds(const glm::vec3& pos) const;
    
    // Special collision cases
    glm::vec3 findSafePosition(const glm::vec3& pos, World* world, float startHeight = 100.0f);
    
    // Block and position checking utilities
    bool isInsideBlock(const glm::vec3& pos, World* world);
    void adjustPositionAtBlockBoundaries(glm::vec3& pos, bool verbose = false);
    glm::vec3 adjustPositionOutOfBlock(const glm::vec3& pos, World* world);
    
    // Debug visualization
    void setDebugMode(bool enabled) { m_debugMode = enabled; }
    bool isDebugMode() const { return m_debugMode; }
    
    // Enable or disable greedy meshing for collision detection
    void setUseGreedyMeshing(bool enabled) { m_useGreedyMeshing = enabled; }
    bool isUsingGreedyMeshing() const { return m_useGreedyMeshing; }
    
    // Collision enabling/disabling
    bool hasCollision() const { return m_collisionEnabled; }
    void setCollision(bool enabled) { m_collisionEnabled = enabled; }
    
private:
    // Entity dimensions for collision
    float m_width;
    float m_height;
    
    // Collision detection parameters
    float m_collisionInset;    // How much to inset the collision box for stability
    float m_widerCollisionExpansion;
    float m_edgeThreshold;     // Threshold for detecting edges
    float m_verticalStepSize;  // Step size for vertical movement
    
    // Debug flag
    bool m_debugMode;
    
    // Use greedy meshing for collision detection
    bool m_useGreedyMeshing;
    
    // Is collision enabled
    bool m_collisionEnabled = true;
    
    // Internal helper methods
    bool isBlockSolid(int blockType) const;
    bool intersectsWithBlock(const glm::vec3& min, const glm::vec3& max, 
                           const glm::vec3& blockMin, const glm::vec3& blockMax) const;
    
    // Helper for AABB vs AABB collision detection 
    bool intersectsWithAABB(const glm::vec3& min1, const glm::vec3& max1,
                           const glm::vec3& min2, const glm::vec3& max2) const;
}; 