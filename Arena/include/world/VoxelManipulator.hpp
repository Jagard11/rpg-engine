#pragma once

#include <glm/glm.hpp>
#include "World.hpp"
#include "../player/Player.hpp"
#include "../renderer/Renderer.hpp"

class Renderer; // Forward declaration to avoid circular dependencies

class VoxelManipulator {
public:
    // Default constructor
    VoxelManipulator();
    
    // Initialize with world reference
    void initialize(World* world);
    
    // Process mouse inputs for voxel manipulation
    void processInput(World* world, Player* player, int mouseButton, bool isPressed, Renderer* renderer);
    
    // Add a voxel at the highlighted face position
    bool addVoxel(World* world, Player* player, const World::RaycastResult& raycastResult, int blockType = 1); // 1 = dirt
    
    // Remove the highlighted voxel
    bool removeVoxel(World* world, const World::RaycastResult& raycastResult);
    
private:
    // Check if a position is valid for placing a block
    bool isValidPlacement(World* world, const glm::ivec3& pos) const;
    
    // Check if a block position overlaps with player bounds
    bool wouldCollideWithPlayer(const glm::ivec3& blockPos, Player* player) const;
    
    // Helper to update chunks after manipulation
    void updateSurroundingChunks(World* world, const glm::ivec3& blockPos);
    
    // Current state
    bool m_leftMousePressed;
    bool m_rightMousePressed;
}; 