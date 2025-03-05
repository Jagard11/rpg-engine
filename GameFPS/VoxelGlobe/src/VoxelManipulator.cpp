// ./src/VoxelManipulator.cpp
#include "VoxelManipulator.hpp"
#include "Debug/DebugManager.hpp"
#include <iostream>
#include <glm/geometric.hpp>
#include "World/Chunk.hpp"

VoxelManipulator::VoxelManipulator(World& world) : worldRef(world) {}

bool VoxelManipulator::placeBlock(const Player& player, BlockType type) {
    if (type == BlockType::AIR) {
        std::cout << "Cannot place AIR blocks" << std::endl;
        return false;
    }

    // Print player position and camera direction for debugging
    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;
    
    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Attempting to place block, type: " << static_cast<int>(type) << std::endl;
        std::cout << "Player position: " << player.position.x << ", " << player.position.y << ", " << player.position.z << std::endl;
        std::cout << "Camera direction: " << player.cameraDirection.x << ", " << player.cameraDirection.y << ", " << player.cameraDirection.z << std::endl;
    }
    
    // Use increased reach distance suitable for the planet scale
    if (raycast(eyePos, player.cameraDirection, MAX_REACH, hitPos, hitNormal)) {
        // Placement position is adjacent to the hit face
        glm::ivec3 placePos = hitPos + glm::ivec3(round(hitNormal.x), round(hitNormal.y), round(hitNormal.z));
        
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Raycast hit at: " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << std::endl;
            std::cout << "Normal: " << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z << std::endl;
            std::cout << "Placing at: " << placePos.x << ", " << placePos.y << ", " << placePos.z << std::endl;
            
            // Debug distance from center - for range check validation
            double px = static_cast<double>(placePos.x);
            double py = static_cast<double>(placePos.y);
            double pz = static_cast<double>(placePos.z);
            double dist = sqrt(px*px + py*py + pz*pz);
            float surfaceR = worldRef.getRadius() + 8.0f;
            std::cout << "Block distance from center: " << dist << ", surface at: " << surfaceR
                     << ", height above surface: " << (dist - surfaceR) << std::endl;
        }
        
        // Check if the position is valid for block placement
        if (isValidPlacementPosition(placePos)) {
            // Place the block
            try {
                worldRef.setBlock(placePos.x, placePos.y, placePos.z, type);
                
                // Force any nearby chunks to regenerate collision geometry
                updateChunkCollisions(placePos);
                
                if (DebugManager::getInstance().logBlockPlacement()) {
                    std::cout << "Placed block at (" << placePos.x << ", " << placePos.y << ", " << placePos.z 
                            << ") Type: " << static_cast<int>(type) << std::endl;
                }
                return true;
            } catch (...) {
                std::cerr << "Exception during block placement" << std::endl;
                return false;
            }
        } else {
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Invalid placement position: (" << placePos.x << ", " << placePos.y << ", " << placePos.z 
                          << ") - outside of editable range" << std::endl;
            }
            return false;
        }
    } else {
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Raycast failed to hit any block" << std::endl;
        }
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
        std::cout << "Camera direction: " << player.cameraDirection.x << ", " << player.cameraDirection.y << ", " << player.cameraDirection.z << std::endl;
    }
    
    // Use increased reach distance suitable for the planet scale
    if (raycast(eyePos, player.cameraDirection, MAX_REACH, hitPos, hitNormal)) {
        // Get the block at the hit position
        Block block = worldRef.getBlock(hitPos.x, hitPos.y, hitPos.z);
        
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Raycast hit at " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << std::endl;
            std::cout << "Block type: " << static_cast<int>(block.type) << std::endl;
            
            // Debug distance from center - for range check validation
            double hx = static_cast<double>(hitPos.x);
            double hy = static_cast<double>(hitPos.y);
            double hz = static_cast<double>(hitPos.z);
            double dist = sqrt(hx*hx + hy*hy + hz*hz);
            float surfaceR = worldRef.getRadius() + 8.0f;
            std::cout << "Block distance from center: " << dist << ", surface at: " << surfaceR
                     << ", height above surface: " << (dist - surfaceR) << std::endl;
        }
        
        if (block.type == BlockType::AIR) {
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Cannot remove air at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
            }
            return false;
        }

        // Check if the position is valid for block removal
        if (isValidRemovalPosition(hitPos)) {
            // Remove the block (set to air)
            try {
                worldRef.setBlock(hitPos.x, hitPos.y, hitPos.z, BlockType::AIR);
                
                // Force any nearby chunks to regenerate collision geometry
                updateChunkCollisions(hitPos);
                
                if (DebugManager::getInstance().logBlockPlacement()) {
                    std::cout << "Removed block at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
                }
                return true;
            } catch (...) {
                std::cerr << "Exception during block removal" << std::endl;
                return false;
            }
        } else {
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Invalid removal position: (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z 
                          << ") - outside of editable range" << std::endl;
            }
            return false;
        }
    } else if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Raycast didn't hit anything" << std::endl;
    }
    return false;
}

// Helper function to update chunk collision data
void VoxelManipulator::updateChunkCollisions(const glm::ivec3& blockPos) {
    // Convert world position to chunk coordinates
    int chunkX = static_cast<int>(floor(blockPos.x / static_cast<float>(Chunk::SIZE)));
    int chunkY = static_cast<int>(floor(blockPos.y / static_cast<float>(Chunk::SIZE)));
    int chunkZ = static_cast<int>(floor(blockPos.z / static_cast<float>(Chunk::SIZE)));
    
    // Get all chunks that might need updating (the block's chunk and all adjacent chunks)
    const glm::ivec3 offsets[7] = {
        {0, 0, 0},   // Current chunk
        {1, 0, 0},   // Adjacent chunks
        {-1, 0, 0},
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1}
    };
    
    // Loop through all potential chunks to update
    for (const auto& offset : offsets) {
        int targetChunkX = chunkX + offset.x;
        int targetChunkY = chunkY + offset.y;
        int targetChunkZ = chunkZ + offset.z;
        
        // Access the chunk and force a collision geometry update
        auto& chunks = worldRef.getChunks();
        auto key = std::make_tuple(targetChunkX, targetChunkY, targetChunkZ, 1);
        auto it = chunks.find(key);
        
        if (it != chunks.end()) {
            // Mark the chunk for mesh regeneration and update immediately
            it->second->markMeshDirty();
            it->second->regenerateMesh();
            
            // Make sure to update the OpenGL buffers as well
            if (it->second->isBuffersInitialized()) {
                it->second->updateBuffers();
            } else {
                it->second->initializeBuffers();
            }
            
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Updated collision geometry for chunk (" 
                          << targetChunkX << ", " << targetChunkY << ", " << targetChunkZ << ")" << std::endl;
            }
        }
    }
}

bool VoxelManipulator::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                              glm::ivec3& hitPos, glm::vec3& hitNormal, ToolType tool) const {
    // Ensure direction is not a zero vector
    if (glm::length(direction) < 0.00001f) {
        if (DebugManager::getInstance().logRaycast()) {
            std::cerr << "Warning: Zero direction vector in raycast" << std::endl;
        }
        return false;
    }

    // Normalize the direction vector
    glm::vec3 dir = glm::normalize(direction);
    
    // Print for debugging
    if (DebugManager::getInstance().logRaycast()) {
        std::cout << "Raycast from: " << origin.x << ", " << origin.y << ", " << origin.z << std::endl;
        std::cout << "Raycast direction: " << dir.x << ", " << dir.y << ", " << dir.z << std::endl;
    }
    
    // For improved raycast accuracy, use a smaller step size
    float stepSize = 0.5f;  // Reduced from 1.0f for better precision
    float distance = 0.0f;
    glm::vec3 currentPos = origin;
    
    // Get surface radius
    float surfaceR = worldRef.getRadius() + 8.0f;
    
    // Limit maximum distance to a reasonable value
    maxDistance = glm::min(maxDistance, 100.0f);  // Reduced from 1000.0f
    
    // Max iterations to prevent infinite loops
    const int MAX_ITERATIONS = 1000;
    int iterations = 0;
    
    // Start raycast from a small initial offset to avoid self-collisions
    currentPos += dir * 0.1f;
    
    // Use the adaptive step size based on distance from surface
    while (distance < maxDistance && iterations < MAX_ITERATIONS) {
        iterations++;
        
        // Step along ray
        currentPos += dir * stepSize;
        distance += stepSize;
        
        // Get current voxel position
        glm::ivec3 voxelPos = glm::ivec3(floor(currentPos.x), floor(currentPos.y), floor(currentPos.z));
        
        // Safety check for extreme coordinates
        if (abs(voxelPos.x) > 100000 || abs(voxelPos.y) > 100000 || abs(voxelPos.z) > 100000) {
            if (DebugManager::getInstance().logRaycast()) {
                std::cerr << "Warning: Extreme coordinates in raycast, stopping early." << std::endl;
            }
            return false;
        }
        
        // Calculate distance from center using double precision
        double vx = static_cast<double>(voxelPos.x);
        double vy = static_cast<double>(voxelPos.y);
        double vz = static_cast<double>(voxelPos.z);
        double distFromCenter = sqrt(vx*vx + vy*vy + vz*vz);
        double distFromSurface = abs(distFromCenter - surfaceR);
        
        // Adjust step size based on distance from surface
        if (distFromSurface > 100.0) {
            stepSize = 10.0f; // Larger steps far from surface
        } else if (distFromSurface > 10.0) {
            stepSize = 2.0f;  // Medium steps approaching surface
        } else {
            stepSize = 0.5f;  // Small steps near surface for accuracy
        }
        
        // Check if we hit a non-air block
        Block block = worldRef.getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Extra verification to avoid floating point imprecision
            // Recheck adjacent positions to ensure we have the exact block face
            float smallStep = 0.1f;
            glm::vec3 confirmPos = currentPos - dir * smallStep;
            glm::ivec3 confirmVoxel = glm::ivec3(floor(confirmPos.x), floor(confirmPos.y), floor(confirmPos.z));
            
            // If the confirmation position is different, we're on the block boundary
            if (confirmVoxel.x != voxelPos.x || confirmVoxel.y != voxelPos.y || confirmVoxel.z != voxelPos.z) {
                // We're at a block boundary, determine which face we hit
                
                // Find the face normal by comparing the position components
                glm::vec3 diff = glm::vec3(voxelPos) - confirmPos;
                glm::vec3 faceNormal = glm::vec3(0.0f);
                
                if (abs(diff.x) > abs(diff.y) && abs(diff.x) > abs(diff.z)) {
                    faceNormal.x = diff.x > 0 ? -1.0f : 1.0f;
                } else if (abs(diff.y) > abs(diff.x) && abs(diff.y) > abs(diff.z)) {
                    faceNormal.y = diff.y > 0 ? -1.0f : 1.0f;
                } else {
                    faceNormal.z = diff.z > 0 ? -1.0f : 1.0f;
                }
                
                hitPos = voxelPos;
                hitNormal = faceNormal;
            } else {
                // We're inside the block, use a more general approach to find the face
                
                // Calculate hit normal by checking surrounding blocks
                // Move slightly back to ensure we're outside the hit block
                glm::vec3 backPos = currentPos - dir * stepSize * 0.5f;
                
                // Check all 6 directions to find which face was hit
                const glm::vec3 faceNormals[6] = {
                    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
                };
                
                // Find which face is closest to the hit point
                float minDist = maxDistance;
                int closestFace = 0;
                
                for (int i = 0; i < 6; i++) {
                    glm::vec3 faceCenter = glm::vec3(voxelPos) + glm::vec3(0.5f) + faceNormals[i] * 0.5f;
                    float dist = glm::length(faceCenter - backPos);
                    if (dist < minDist) {
                        minDist = dist;
                        closestFace = i;
                    }
                }
                
                hitPos = voxelPos;
                hitNormal = faceNormals[closestFace];
            }
            
            if (DebugManager::getInstance().logRaycast()) {
                std::cout << "Raycast hit block at " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z 
                          << " with normal " << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z 
                          << " (distance: " << distance << ")" << std::endl;
            }
            
            return true;
        }
    }
    
    if (DebugManager::getInstance().logRaycast()) {
        if (iterations >= MAX_ITERATIONS) {
            std::cerr << "Warning: Raycast reached maximum iterations" << std::endl;
        } else {
            std::cout << "Raycast hit nothing (max distance: " << maxDistance << ")" << std::endl;
        }
    }
    
    return false;
}

bool VoxelManipulator::isValidPlacementPosition(const glm::ivec3& pos) const {
    // Calculate distance from center using double precision for accuracy
    double px = static_cast<double>(pos.x);
    double py = static_cast<double>(pos.y);
    double pz = static_cast<double>(pos.z);
    double distFromCenter = sqrt(px*px + py*py + pz*pz);
    
    float surfaceR = worldRef.getRadius() + 8.0f; // Surface radius
    
    // Valid range: From below surface (crust) to 15km above sea level
    // 5.0f units below surface is the bottom of the crust
    // 15.0f units above surface is approximately 15km above sea level
    bool isWithinRange = (distFromCenter >= worldRef.getRadius() - 5.0f) && 
                        (distFromCenter <= surfaceR + 15.0f);
    
    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Placement position validation: " << pos.x << ", " << pos.y << ", " << pos.z 
                  << " (distance from center: " << distFromCenter << ")" << std::endl;
        std::cout << "Valid range: " << (worldRef.getRadius() - 5.0f) << " to " << (surfaceR + 15.0f) << std::endl;
        std::cout << "Result: " << (isWithinRange ? "Valid" : "Invalid") << std::endl;
    }
    
    return isWithinRange;
}

bool VoxelManipulator::isValidRemovalPosition(const glm::ivec3& pos) const {
    // Use the same validation criteria as for placement
    return isValidPlacementPosition(pos);
}

bool VoxelManipulator::isAdjacentToSolid(const glm::ivec3& pos) const {
    // Check all 6 adjacent positions
    const glm::ivec3 directions[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    for (const auto& dir : directions) {
        glm::ivec3 neighbor = pos + dir;
        Block block = worldRef.getBlock(neighbor.x, neighbor.y, neighbor.z);
        if (block.type != BlockType::AIR) {
            return true;
        }
    }
    return false;
}