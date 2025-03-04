// ./src/VoxelManipulator.cpp
#include "VoxelManipulator.hpp"
#include "Debug/DebugManager.hpp"
#include <iostream>
#include <glm/geometric.hpp>

VoxelManipulator::VoxelManipulator(World& world) : worldRef(world) {}

bool VoxelManipulator::placeBlock(const Player& player, BlockType type) {
    if (type == BlockType::AIR) return false;

    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;
    
    if (raycast(eyePos, player.cameraDirection, MAX_REACH, hitPos, hitNormal)) {
        // Placement position is adjacent to the hit face
        glm::ivec3 placePos = hitPos + glm::ivec3(round(hitNormal.x), round(hitNormal.y), round(hitNormal.z));
        
        // Place the block
        worldRef.setBlock(placePos.x, placePos.y, placePos.z, type);
        
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Placed block at (" << placePos.x << ", " << placePos.y << ", " << placePos.z 
                      << ") Type: " << static_cast<int>(type) << std::endl;
        }
        return true;
    }
    
    return false;
}

bool VoxelManipulator::removeBlock(const Player& player) {
    // Get eye position (camera position)
    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;

    // Use raycast to find where the player is looking
    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Attempting to remove block, eye position: " << eyePos.x << ", " << eyePos.y << ", " << eyePos.z << std::endl;
    }
    
    if (raycast(eyePos, player.cameraDirection, MAX_REACH, hitPos, hitNormal)) {
        // Get the block at the hit position
        Block block = worldRef.getBlock(hitPos.x, hitPos.y, hitPos.z);
        
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Raycast hit at " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << std::endl;
            std::cout << "Block type: " << static_cast<int>(block.type) << std::endl;
        }
        
        if (block.type == BlockType::AIR) {
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Cannot remove air at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
            }
            return false;
        }

        // Remove the block (set to air)
        worldRef.setBlock(hitPos.x, hitPos.y, hitPos.z, BlockType::AIR);
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Removed block at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
        }
        return true;
    } else if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Raycast didn't hit anything" << std::endl;
    }
    return false;
}

bool VoxelManipulator::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                              glm::ivec3& hitPos, glm::vec3& hitNormal, ToolType tool) const {
    // Normalize the direction vector
    glm::vec3 dir = glm::normalize(direction);
    
    // Using DDA algorithm for more precise raycasting
    glm::ivec3 mapPos = glm::ivec3(floor(origin.x), floor(origin.y), floor(origin.z));
    
    // Direction to step in (+1 or -1 for each axis)
    glm::ivec3 stepDir = glm::ivec3(
        dir.x > 0 ? 1 : (dir.x < 0 ? -1 : 0),
        dir.y > 0 ? 1 : (dir.y < 0 ? -1 : 0),
        dir.z > 0 ? 1 : (dir.z < 0 ? -1 : 0)
    );
    
    // Distance from current position to next voxel boundary
    glm::vec3 sideDist = glm::vec3(
        stepDir.x > 0 ? (floor(origin.x) + 1.0f - origin.x) : (origin.x - floor(origin.x)),
        stepDir.y > 0 ? (floor(origin.y) + 1.0f - origin.y) : (origin.y - floor(origin.y)),
        stepDir.z > 0 ? (floor(origin.z) + 1.0f - origin.z) : (origin.z - floor(origin.z))
    );
    
    // Avoid division by zero
    sideDist.x = (dir.x == 0) ? INFINITY : sideDist.x / std::abs(dir.x);
    sideDist.y = (dir.y == 0) ? INFINITY : sideDist.y / std::abs(dir.y);
    sideDist.z = (dir.z == 0) ? INFINITY : sideDist.z / std::abs(dir.z);
    
    // Delta distances (distance between voxel boundaries)
    glm::vec3 deltaDist = glm::vec3(
        (dir.x == 0) ? INFINITY : std::abs(1.0f / dir.x),
        (dir.y == 0) ? INFINITY : std::abs(1.0f / dir.y),
        (dir.z == 0) ? INFINITY : std::abs(1.0f / dir.z)
    );
    
    float distance = 0.0f;
    
    // Main raycast loop
    while (distance < maxDistance) {
        // Find axis with closest boundary
        int axis = 0;
        if (sideDist.y < sideDist.x && sideDist.y < sideDist.z) {
            axis = 1;
        } else if (sideDist.z < sideDist.x && sideDist.z < sideDist.y) {
            axis = 2;
        }
        
        // Step to next voxel
        if (axis == 0) {
            mapPos.x += stepDir.x;
            distance = sideDist.x;
            sideDist.x += deltaDist.x;
        } else if (axis == 1) {
            mapPos.y += stepDir.y;
            distance = sideDist.y;
            sideDist.y += deltaDist.y;
        } else {
            mapPos.z += stepDir.z;
            distance = sideDist.z;
            sideDist.z += deltaDist.z;
        }
        
        // Check if hit a block
        Block block = worldRef.getBlock(mapPos.x, mapPos.y, mapPos.z);
        if (block.type != BlockType::AIR) {
            hitPos = mapPos;
            
            // Set normal based on which face we hit
            hitNormal = glm::vec3(0.0f);
            if (axis == 0) hitNormal.x = -stepDir.x;
            else if (axis == 1) hitNormal.y = -stepDir.y;
            else hitNormal.z = -stepDir.z;
            
            if (DebugManager::getInstance().logRaycast()) {
                std::cout << "Raycast hit block at " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z 
                          << " with normal " << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z << std::endl;
            }
            
            return true;
        }
    }
    
    return false;
}

bool VoxelManipulator::isAdjacentToSolid(const glm::ivec3& pos) const {
    // Check all 6 adjacent positions
    const glm::ivec3 directions[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    for (const auto& dir : directions) {
        glm::ivec3 neighbor = pos + dir;
        Block block = worldRef.getBlock(neighbor.x, neighbor.y, neighbor.z);
        if (block.type == BlockType::GRASS || block.type == BlockType::DIRT) {
            return true;
        }
    }
    return false;
}