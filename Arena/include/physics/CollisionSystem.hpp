#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include "../world/World.hpp"
#include "../world/Chunk.hpp"

// CollisionBox struct to make collision more modular
struct CollisionBox {
    float width;        // Width of the collision box (X-axis)
    float height;       // Height of the collision box (Y-axis)
    float depth;        // Depth of the collision box (Z-axis)
    glm::vec3 offset;   // Offset from entity position
    
    CollisionBox(float w = 0.25f, float h = 1.8f, float d = 0.25f, glm::vec3 off = glm::vec3(0.0f))
        : width(w), height(h), depth(d), offset(off) {}
    
    // Calculate min/max bounds based on a position
    glm::vec3 getMin(const glm::vec3& position) const {
        return position - glm::vec3(width * 0.5f, 0.0f, depth * 0.5f) + offset;
    }
    
    glm::vec3 getMax(const glm::vec3& position) const {
        return position + glm::vec3(width * 0.5f, height, depth * 0.5f) + offset;
    }
    
    // Adjust the collision box for falling
    CollisionBox getSmallerBox(float insetFactor = 0.15f) const {
        CollisionBox smaller = *this;
        smaller.width *= (1.0f - insetFactor);
        smaller.depth *= (1.0f - insetFactor);
        return smaller;
    }
};

class CollisionSystem {
public:
    CollisionSystem();
    
    // Initialize with dimensions of the entity
    void init(float width, float height);
    
    // Set the collision box directly
    void setCollisionBox(const CollisionBox& box) { m_collisionBox = box; }
    const CollisionBox& getCollisionBox() const { return m_collisionBox; }
    
    // Primary collision detection methods
    bool collidesWithBlocks(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision = false);
    bool collidesWithBlocksGreedy(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision = false);
    bool checkGroundCollision(const glm::vec3& pos, const glm::vec3& velocity, World* world);
    bool isPositionSafe(const glm::vec3& pos, World* world);
    
    // Perform movement with collision detection
    glm::vec3 moveWithCollision(const glm::vec3& currentPos, const glm::vec3& movement, const glm::vec3& velocity, 
                              World* world, bool isFlying, bool& isOnGround);
    
    // Helper functions for collision box calculations
    glm::vec3 getMinBounds(const glm::vec3& pos, const glm::vec3& forward) const;
    glm::vec3 getMaxBounds(const glm::vec3& pos, const glm::vec3& forward) const;
    
    // Special collision cases
    glm::vec3 findSafePosition(const glm::vec3& pos, World* world, float startHeight = 100.0f);
    
    // Block and position checking utilities
    bool isInsideBlock(const glm::vec3& pos, World* world);
    void adjustPositionAtBlockBoundaries(glm::vec3& pos, bool verbose = false);
    glm::vec3 adjustPositionOutOfBlock(const glm::vec3& pos, World* world);
    void correctVerticalWallCollision(glm::vec3& position, const glm::vec3& velocity, World* world);
    
    // Camera position adjustment to prevent clipping
    glm::vec3 getCameraPosition(const glm::vec3& playerPos, float cameraHeight, const glm::vec3& forward, World* world);
    
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
    
    // New collision box
    CollisionBox m_collisionBox;
    
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
                           
    // Helper for point vs block collision with margin
    bool pointInsideBlockWithMargin(const glm::vec3& point, const glm::vec3& blockMin, 
                                 const glm::vec3& blockMax, float margin) const;

    // Helper for oriented collision box calculations
    glm::vec3 rotatePoint(const glm::vec3& point, const glm::vec3& forward) const;
}; 