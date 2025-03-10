// ./src/VoxelManipulator.cpp
#include "VoxelManipulator.hpp"
#include "Debug/DebugManager.hpp"
#include <iostream>
#include <glm/geometric.hpp>
#include "World/Chunk.hpp"
#include "Utils/SphereUtils.hpp"

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
        // IMPROVED: Use proper normal to position calculation to ensure vertical building works
        glm::ivec3 placePos;
        
        // Check if we're looking up at a block (special case for vertical building)
        bool lookingUp = player.cameraDirection.y > 0.3f; // If looking upward
        
        if (abs(hitNormal.y) > 0.9f && hitNormal.y > 0) {
            // Special case: We hit the top face of a block - place directly above
            placePos = hitPos + glm::ivec3(0, 1, 0);
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Detected top face hit - placing block directly above" << std::endl;
            }
        } else if (abs(hitNormal.y) > 0.9f && hitNormal.y < 0 && lookingUp) {
            // Special case: We hit the bottom face of a block while looking up
            placePos = hitPos + glm::ivec3(0, -1, 0);
            if (DebugManager::getInstance().logBlockPlacement()) {
                std::cout << "Detected bottom face hit while looking up - placing below" << std::endl;
            }
        } else {
            // Standard case - use normal to determine direction
            placePos = hitPos + glm::ivec3(round(hitNormal.x), round(hitNormal.y), round(hitNormal.z));
        }
        
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Raycast hit at: " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << std::endl;
            std::cout << "Normal: " << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z << std::endl;
            std::cout << "Placing at: " << placePos.x << ", " << placePos.y << ", " << placePos.z << std::endl;
            
            // Debug distance from center - for range check validation
            double px = static_cast<double>(placePos.x);
            double py = static_cast<double>(placePos.y);
            double pz = static_cast<double>(placePos.z);
            double dist = sqrt(px*px + py*py + pz*pz);
            double surfaceR = SphereUtils::getSurfaceRadiusMeters();
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
            double surfaceR = SphereUtils::getSurfaceRadiusMeters();
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
    
    // Use planet-centric coordinates for raycast
    // Get distance of the origin from planet center
    double distFromCenter = glm::length(origin);
    
    // Get surface radius
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    // Calculate the surface normal at the player position (points away from center)
    glm::vec3 surfaceNormal = glm::normalize(origin);
    
    // FIXED: Always provide enough reach distance for building (minimum 5 units)
    double heightAboveSurface = distFromCenter - surfaceR;
    // Always give at least 5 units of reach regardless of height
    float adaptedMaxDistance = std::max(5.0f, std::min(maxDistance, static_cast<float>(heightAboveSurface) * 2.0f + 20.0f));
                                      
    if (DebugManager::getInstance().logRaycast()) {
        std::cout << "Height above surface: " << heightAboveSurface 
                  << ", adapted max distance: " << adaptedMaxDistance << std::endl;
    }
    
    // Start raycast from a small initial offset to avoid self-collisions
    glm::vec3 currentPos = origin + dir * 0.1f;
    float distance = 0.0f;
    
    // For Earth-scale raycast, we need to work in a local coordinate system
    // Get the player's chunk coordinates to serve as the origin
    int playerChunkX = static_cast<int>(floor(origin.x / static_cast<float>(Chunk::SIZE)));
    int playerChunkY = static_cast<int>(floor(origin.y / static_cast<float>(Chunk::SIZE)));
    int playerChunkZ = static_cast<int>(floor(origin.z / static_cast<float>(Chunk::SIZE)));
    
    // Calculate offset to local coordinate system
    float offsetX = playerChunkX * Chunk::SIZE;
    float offsetY = playerChunkY * Chunk::SIZE;
    float offsetZ = playerChunkZ * Chunk::SIZE;
    
    // Transform origin to local coordinates
    glm::vec3 localOrigin = glm::vec3(origin.x - offsetX, origin.y - offsetY, origin.z - offsetZ);
    glm::vec3 localCurrentPos = glm::vec3(currentPos.x - offsetX, currentPos.y - offsetY, currentPos.z - offsetZ);
    
    // Max iterations to prevent infinite loops
    const int MAX_ITERATIONS = 1000;
    int iterations = 0;
    
    // IMPROVED: Use a much finer step size when looking straight up or down
    // This is crucial for vertical building
    float baseStepSize = 0.1f; // Always use a smaller base step for accuracy
    float stepSize = baseStepSize;
    
    // If looking mostly up/down, use an even smaller step size for better precision
    if (abs(dir.y) > 0.7f) {
        stepSize = 0.05f;
    }
    
    while (distance < adaptedMaxDistance && iterations < MAX_ITERATIONS) {
        iterations++;
        
        // Determine step size based on distance from origin
        float localStepSize = stepSize;
        float distFromOrigin = glm::length(localCurrentPos - localOrigin);
        
        // Use slightly larger steps when further from origin, but still keep a reasonable size
        // for accurate top-face detection
        if (distFromOrigin > 10.0f) localStepSize = stepSize * 2.0f;
        if (distFromOrigin > 20.0f) localStepSize = stepSize * 3.0f;
        
        // Step along ray in local coordinates
        localCurrentPos += dir * localStepSize;
        distance += localStepSize;
        
        // Convert back to world coordinates to check for block
        glm::vec3 worldPos = glm::vec3(
            localCurrentPos.x + offsetX,
            localCurrentPos.y + offsetY,
            localCurrentPos.z + offsetZ
        );
        
        // Get current voxel position
        glm::ivec3 voxelPos = glm::ivec3(floor(worldPos.x), floor(worldPos.y), floor(worldPos.z));
        
        // Check if we hit a non-air block
        Block block = worldRef.getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Extra verification for exact face hit
            float smallStep = 0.05f; // Reduced for better precision
            glm::vec3 confirmPos = worldPos - dir * smallStep;
            glm::ivec3 confirmVoxel = glm::ivec3(floor(confirmPos.x), floor(confirmPos.y), floor(confirmPos.z));
            
            // If the confirmation position is different, we're on the block boundary
            if (confirmVoxel.x != voxelPos.x || confirmVoxel.y != voxelPos.y || confirmVoxel.z != voxelPos.z) {
                // Find the face normal by comparing the position components
                glm::vec3 diff = glm::vec3(voxelPos) + glm::vec3(0.5f) - confirmPos;
                hitNormal = glm::vec3(0.0f);
                
                // IMPROVED: Better face detection, especially for top/bottom faces
                // Focus even more on Y-axis for better vertical building detection
                
                if (abs(diff.y) >= abs(diff.x) * 0.8f && abs(diff.y) >= abs(diff.z) * 0.8f) {
                    // We hit a horizontal face (top or bottom) - prioritize this for vertical building
                    hitNormal.y = diff.y > 0 ? -1.0f : 1.0f;
                    if (DebugManager::getInstance().logRaycast()) {
                        std::cout << "Detected horizontal face hit (Y-axis)" << std::endl;
                    }
                } else if (abs(diff.x) >= abs(diff.y) && abs(diff.x) >= abs(diff.z)) {
                    hitNormal.x = diff.x > 0 ? -1.0f : 1.0f;
                } else if (abs(diff.z) >= abs(diff.x) && abs(diff.z) >= abs(diff.y)) {
                    hitNormal.z = diff.z > 0 ? -1.0f : 1.0f;
                }
                
                hitPos = voxelPos;
            } else {
                // We're inside the block, find the closest face
                const glm::vec3 faceNormals[6] = {
                    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
                };
                
                float minDist = adaptedMaxDistance;
                int closestFace = 0;
                
                for (int i = 0; i < 6; i++) {
                    glm::vec3 faceCenter = glm::vec3(voxelPos) + glm::vec3(0.5f) + faceNormals[i] * 0.5f;
                    float dist = glm::length(faceCenter - worldPos);
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
            std::cout << "Raycast hit nothing (max distance: " << adaptedMaxDistance << ")" << std::endl;
        }
    }
    
    return false;
}

bool VoxelManipulator::isValidPlacementPosition(const glm::ivec3& pos) const {
    // Use SphereUtils to determine if position is within valid building range
    // Calculate distance from center using double precision for accuracy
    double px = static_cast<double>(pos.x);
    double py = static_cast<double>(pos.y);
    double pz = static_cast<double>(pos.z);
    double distFromCenter = sqrt(px*px + py*py + pz*pz);
    
    // Get standardized surface radius
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    // Valid range: From below surface (crust) to 15km above sea level
    // Using the standardized constants from SphereUtils/PlanetConfig
    bool isWithinRange = SphereUtils::isWithinBuildRange(glm::dvec3(px, py, pz));
    
    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Placement position validation: " << pos.x << ", " << pos.y << ", " << pos.z 
                  << " (distance from center: " << distFromCenter << ")" << std::endl;
        std::cout << "Valid range: " << (surfaceR - PlanetConfig::TERRAIN_DEPTH_METERS) << " to " 
                  << (surfaceR + PlanetConfig::MAX_BUILD_HEIGHT_METERS) << std::endl;
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