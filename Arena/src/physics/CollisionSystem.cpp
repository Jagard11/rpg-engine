#include "physics/CollisionSystem.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

CollisionSystem::CollisionSystem()
    : m_width(1.0f)   // Increased from 0.8f to allow more space around player
    , m_height(1.8f)  // Standard player height
    , m_collisionInset(0.05f)
    , m_edgeThreshold(0.05f)
    , m_verticalStepSize(0.02f)
    , m_debugMode(true)
    , m_useGreedyMeshing(true) // Enable greedy meshing by default
{
    // Additional initialization can go here if needed
}

void CollisionSystem::init(float width, float height) {
    m_width = width;
    m_height = height;
}

bool CollisionSystem::collidesWithBlocks(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision) {
    // Use greedy meshing if enabled
    if (m_useGreedyMeshing) {
        return collidesWithBlocksGreedy(pos, velocity, world, useWiderCollision);
    }
    
    if (!world) return false;
    
    // Create collision box with a more significant inset for stability
    // Important: pos is now treated as the player's feet position (bottom center)
    glm::vec3 min, max;
    
    // Calculate facing direction for asymmetric collision box (deeper in front than behind)
    // Note: Since we don't have direct access to player facing direction here,
    // we use a symmetrical box but make it wider overall
    
    if (useWiderCollision) {
        // Use wider collision box for better ground detection, especially at chunk boundaries
        min = pos + glm::vec3(-(m_width/2 + 0.3f), 0.0f, -(m_width/2 + 0.3f));
        max = pos + glm::vec3(m_width/2 + 0.3f, m_height, m_width/2 + 0.3f);
    } else {
        // Use normal collision box with increased depth
        min = pos + glm::vec3(-(m_width/2 + 0.1f), 0.0f, -(m_width/2 + 0.1f));
        max = pos + glm::vec3(m_width/2 + 0.1f, m_height - m_collisionInset*2, m_width/2 + 0.1f);
    }
    
    // Debug output for collision checking
    static int debugCounter = 0;
    bool verboseDebug = m_debugMode && (debugCounter++ % 180 == 0); // Output detailed logs every ~3 seconds at 60fps
    
    if (verboseDebug) {
        std::cout << "==== COLLISION CHECK ====" << std::endl;
        std::cout << "Player position (feet): (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        std::cout << "Collision box min: (" << min.x << ", " << min.y << ", " << min.z << ")" << std::endl;
        std::cout << "Collision box max: (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
        std::cout << "Collision box width: " << max.x - min.x << ", height: " << max.y - min.y << ", depth: " << max.z - min.z << std::endl;
        if (useWiderCollision) {
            std::cout << "Using wider collision box for chunk boundary detection" << std::endl;
        }
    }
    
    // CRITICAL FIX: If player's feet are below ground level, consider it a collision immediately
    if (pos.y < 0.0f) {
        if (verboseDebug) {
            std::cout << "Player is below ground level (y < 0), forcing collision" << std::endl;
        }
        return true;
    }
    
    // Expand checking area slightly to catch edge cases
    int minX = static_cast<int>(std::floor(min.x - 0.05f));
    int minY = static_cast<int>(std::floor(min.y - 0.05f));
    int minZ = static_cast<int>(std::floor(min.z - 0.05f));
    int maxX = static_cast<int>(std::floor(max.x + 0.05f));
    int maxY = static_cast<int>(std::floor(max.y + 0.05f));
    int maxZ = static_cast<int>(std::floor(max.z + 0.05f));
    
    // Ensure minY is not negative to prevent issues with array indexing
    minY = std::max(0, minY);
    
    if (verboseDebug) {
        std::cout << "Checking blocks from (" << minX << ", " << minY << ", " << minZ << ") to ("
                  << maxX << ", " << maxY << ", " << maxZ << ")" << std::endl;
    }
    
    // Calculate valid block count for debug output
    int totalBlocksToCheck = 0;
    if (maxX >= minX && maxY >= minY && maxZ >= minZ) {
        totalBlocksToCheck = (maxX - minX + 1) * (maxY - minY + 1) * (maxZ - minZ + 1);
    }
    
    // Add an extra safety check for the ground below the player's feet
    bool groundCollision = false;
    if (velocity.y <= 0) {
        // Check for blocks directly below the player in a 3x3 area for better stability
        for (int x = minX - 1; x <= maxX + 1; x++) {
            for (int z = minZ - 1; z <= maxZ + 1; z++) {
                // Only check valid coordinates
                if (minY >= 1) { // Ensure we don't check below the world's bottom
                    int blockType = world->getBlock(glm::ivec3(x, minY - 1, z));
                    if (isBlockSolid(blockType)) {
                        groundCollision = true;
                        if (verboseDebug) {
                            std::cout << "Ground collision detected at (" << x << ", " << (minY-1) << ", " << z 
                                    << ") with block type " << blockType << std::endl;
                        }
                        break;
                    }
                } else if (minY == 0) {
                    // If we're at y=0, consider it ground automatically
                    groundCollision = true;
                    if (verboseDebug) {
                        std::cout << "Ground collision at world bottom (y=0)" << std::endl;
                    }
                    break;
                }
            }
            if (groundCollision) break;
        }
    }
    
    // Debug flag to count collisions
    int collisionCount = 0;
    
    // Check all block positions that intersect with the player's collision box
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                // Skip invalid coordinates
                if (y < 0) continue;
                
                glm::ivec3 blockPos(x, y, z);
                int blockType = world->getBlock(blockPos);
                
                // Use the same logic as rendering to determine solid blocks
                if (blockType != 0) { // Non-air block found
                    collisionCount++;
                    
                    // Get the precise collision box for this block
                    glm::vec3 blockMin(x, y, z);
                    glm::vec3 blockMax(x + 1.0f, y + 1.0f, z + 1.0f);
                    
                    // Use a more precise AABB collision test with a slight expansion at edges
                    if (intersectsWithBlock(min, max, blockMin, blockMax)) {
                        if (verboseDebug) {
                            std::cout << "Collision with block at (" << x << ", " << y << ", " << z 
                                      << ") with type " << blockType << std::endl;
                        }
                        return true;
                    }
                }
            }
        }
    }
    
    if (verboseDebug) {
        std::cout << "Total blocks checked: " << totalBlocksToCheck << std::endl;
        std::cout << "Solid blocks found: " << collisionCount << std::endl;
        std::cout << "Ground collision: " << (groundCollision ? "YES" : "NO") << std::endl;
        std::cout << "Final collision result: " << (groundCollision ? "TRUE (ground)" : "FALSE") << std::endl;
        std::cout << "========================" << std::endl;
    }
    
    return groundCollision; // Return true if we found ground beneath the player
}

bool CollisionSystem::checkGroundCollision(const glm::vec3& pos, const glm::vec3& velocity, World* world) {
    if (!world) return false;
    
    // Use a wider collision box specifically for ground detection
    glm::vec3 groundCheckPos = pos;
    groundCheckPos.y -= 0.05f; // Just a small distance below the position
    
    return collidesWithBlocks(groundCheckPos, velocity, world, true);
}

bool CollisionSystem::isPositionSafe(const glm::vec3& pos, World* world) {
    if (!world) return false;
    
    // Check if position is not inside a block
    if (collidesWithBlocks(pos, glm::vec3(0.0f), world)) {
        return false;
    }
    
    return true;
}

glm::vec3 CollisionSystem::moveWithCollision(const glm::vec3& position, const glm::vec3& movement, 
                                        const glm::vec3& velocity, World* world, 
                                        bool isFlying, bool& isOnGround) {
    if (!world) return position;
    
    // Debug flag
    static int debugCounter = 0;
    bool verboseDebug = m_debugMode && (debugCounter++ % 180 == 0); // Output every ~3 seconds at 60fps
    
    // Early exit for no movement
    if (glm::length(movement) < 0.0001f) {
        if (verboseDebug) {
            std::cout << "==== MOVE WITH COLLISION ====" << std::endl;
            std::cout << "Starting position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            std::cout << "Movement vector: (" << movement.x << ", " << movement.y << ", " << movement.z << ")" << std::endl;
            std::cout << "Final position after movement: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            std::cout << "Movement iterations: 0" << std::endl;
            std::cout << "Collisions encountered: 0" << std::endl;
            std::cout << "===========================" << std::endl;
        }
        return position;
    }
    
    // Split movement into horizontal and vertical components for better collision handling
    glm::vec3 horizontalMovement(movement.x, 0.0f, movement.z);
    glm::vec3 verticalMovement(0.0f, movement.y, 0.0f);
    
    // Capture starting position for debug and rollback
    glm::vec3 startPos = position;
    glm::vec3 currentPos = position;
    
    // Variables to track iterations and collisions
    int movementIterations = 0;
    int collisionCount = 0;
    
    if (verboseDebug) {
        std::cout << "==== MOVE WITH COLLISION ====" << std::endl;
        std::cout << "Starting position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
        std::cout << "Movement vector: (" << movement.x << ", " << movement.y << ", " << movement.z << ")" << std::endl;
    }
    
    // Pre-movement check to ensure we're not starting inside a block
    if (isInsideBlock(currentPos, world)) {
        // Try to adjust position slightly to get out of the block
        glm::vec3 adjustedPos = adjustPositionOutOfBlock(currentPos, world);
        
        if (!isInsideBlock(adjustedPos, world)) {
            if (verboseDebug) {
                std::cout << "Adjusted starting position to avoid being inside a block: " 
                         << "(" << adjustedPos.x << ", " << adjustedPos.y << ", " << adjustedPos.z << ")" << std::endl;
            }
            currentPos = adjustedPos;
        } else if (verboseDebug) {
            std::cout << "WARNING: Starting inside a block and couldn't adjust position" << std::endl;
        }
    }
    
    // Handle horizontal movement first
    if (glm::length(horizontalMovement) > 0.0001f) {
        glm::vec3 normalizedHorizontal = glm::normalize(horizontalMovement);
        const float stepSize = 0.01f; // Small step size for better precision
        float totalHorizontalDistance = glm::length(horizontalMovement);
        float distanceMoved = 0.0f;
        
        // Iterate over the horizontal movement in small steps
        while (distanceMoved < totalHorizontalDistance) {
            movementIterations++;
            
            // Calculate the next step position
            glm::vec3 step = normalizedHorizontal * std::min(stepSize, totalHorizontalDistance - distanceMoved);
            glm::vec3 nextPos = currentPos + step;
            
            // Check for collisions at the next position
            if (collidesWithBlocks(nextPos, velocity, world)) {
                collisionCount++;
                
                // Try to slide along the obstacle
                glm::vec3 slideX(step.x, 0, 0);
                glm::vec3 slideZ(0, 0, step.z);
                
                glm::vec3 nextPosX = currentPos + slideX;
                glm::vec3 nextPosZ = currentPos + slideZ;
                
                bool canMoveX = !collidesWithBlocks(nextPosX, velocity, world);
                bool canMoveZ = !collidesWithBlocks(nextPosZ, velocity, world);
                
                if (canMoveX) {
                    currentPos = nextPosX;
                    distanceMoved += glm::length(slideX);
                } else if (canMoveZ) {
                    currentPos = nextPosZ;
                    distanceMoved += glm::length(slideZ);
                } else {
                    // Can't move in either direction, stop horizontal movement
                    break;
                }
            } else {
                // No collision, proceed with the step
                currentPos = nextPos;
                distanceMoved += stepSize;
            }
            
            // Safety check to prevent infinite loops
            if (movementIterations > 1000) {
                std::cout << "WARNING: Excessive movement iterations, stopping early." << std::endl;
                break;
            }
        }
    }
    
    // Now handle vertical movement
    if (std::abs(verticalMovement.y) > 0.0001f) {
        glm::vec3 nextPos = currentPos + verticalMovement;
        
        // Check for vertical collisions
        if (collidesWithBlocks(nextPos, velocity, world)) {
            collisionCount++;
            
            // For vertical collision, we want to place the player precisely at the collision point
            // The collidesWithBlocksGreedy function will adjust the y position for ground collisions
            
            // If moving down, check for ground
            if (verticalMovement.y < 0) {
                isOnGround = true;
                
                // The exact y position should be set by collidesWithBlocksGreedy
                // Here we're just detecting the ground collision state
                collidesWithBlocks(currentPos, glm::vec3(0, -1, 0), world, true);
            } 
            // If moving up, find the ceiling point
            else {
                // For ceiling collisions, we'd need a more sophisticated system
                // For now, just stop vertical movement
            }
        } else {
            // No collision, complete the vertical movement
            currentPos = nextPos;
            
            // Check if we're on ground after the movement
            isOnGround = checkGroundCollision(currentPos, velocity, world);
        }
    } else {
        // Even if no vertical movement, check if we're on ground
        isOnGround = checkGroundCollision(currentPos, velocity, world);
    }
    
    // Final position check
    if (isInsideBlock(currentPos, world)) {
        // This shouldn't happen with proper collision detection, but just in case
        if (verboseDebug) {
            std::cout << "WARNING: Player inside block after movement. Attempting to adjust position." << std::endl;
        }
        
        // Try to adjust position to be outside of blocks
        glm::vec3 adjustedPos = adjustPositionOutOfBlock(currentPos, world);
        
        // Only use adjusted position if it's actually not inside a block
        if (!isInsideBlock(adjustedPos, world)) {
            currentPos = adjustedPos;
            if (verboseDebug) {
                std::cout << "Successfully adjusted position to avoid blocks" << std::endl;
            }
        } else {
            // If we can't fix it, revert to start position
            std::cout << "WARNING: Player inside block after movement. Reverting to start position." << std::endl;
            currentPos = startPos;
        }
    }
    
    // Add redundant checks after movement
    if (currentPos.y < 0.0f) {
        currentPos.y = 0.01f; // Place slightly above 0
        isOnGround = true;
        if (verboseDebug) {
            std::cout << "Prevented falling below y=0 after movement" << std::endl;
        }
    }
    
    if (verboseDebug) {
        std::cout << "Final position after movement: (" << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << ")" << std::endl;
        std::cout << "Movement iterations: " << movementIterations << std::endl;
        std::cout << "Collisions encountered: " << collisionCount << std::endl;
        std::cout << "Is on ground: " << (isOnGround ? "YES" : "NO") << std::endl;
        std::cout << "===========================" << std::endl;
    }
    
    return currentPos;
}

glm::vec3 CollisionSystem::getMinBounds(const glm::vec3& pos) const {
    // Match the collision box used in collidesWithBlocks for consistency
    // Important: pos is now treated as the player's feet position
    return pos + glm::vec3(-(m_width/2 + 0.1f), 0.0f, -(m_width/2 + 0.1f));
}

glm::vec3 CollisionSystem::getMaxBounds(const glm::vec3& pos) const {
    // Match the collision box used in collidesWithBlocks for consistency
    // Important: pos is now treated as the player's feet position
    return pos + glm::vec3(m_width/2 + 0.1f, m_height - m_collisionInset*2, m_width/2 + 0.1f);
}

void CollisionSystem::adjustPositionAtBlockBoundaries(glm::vec3& pos, bool verbose) {
    // Check if we're very close to block boundaries and adjust slightly
    float xFraction = pos.x - std::floor(pos.x);
    float zFraction = pos.z - std::floor(pos.z);
    
    glm::vec3 adjustment(0.0f);
    bool madeAdjustment = false;
    
    // Determine if we need to adjust position (very close to block boundaries)
    if (xFraction < 0.02f) {
        adjustment.x = 0.03f;
        madeAdjustment = true;
    } else if (xFraction > 0.98f) {
        adjustment.x = -0.03f;
        madeAdjustment = true;
    }
    
    if (zFraction < 0.02f) {
        adjustment.z = 0.03f;
        madeAdjustment = true;
    } else if (zFraction > 0.98f) {
        adjustment.z = -0.03f;
        madeAdjustment = true;
    }
    
    // Apply adjustment if needed
    if (madeAdjustment) {
        if (verbose) {
            std::cout << "Adjusted position away from block boundary by " 
                     << "(" << adjustment.x << ", " << adjustment.y << ", " << adjustment.z << ")" << std::endl;
        }
        pos += adjustment;
    }
}

glm::vec3 CollisionSystem::findSafePosition(const glm::vec3& pos, World* world, float startHeight) {
    if (!world) return pos;
    
    // If we're already at a safe position, return it
    if (isPositionSafe(pos, world)) {
        return pos;
    }
    
    // Try to find a safe position above the current position
    glm::vec3 testPos = pos;
    testPos.y = startHeight; // Start from this height
    
    // First ensure the starting position is not inside a block
    while (collidesWithBlocks(testPos, glm::vec3(0.0f), world) && testPos.y < startHeight + 10.0f) {
        testPos.y += 1.0f;
    }
    
    // Now scan downward to find a safe spot with ground underneath
    bool foundSafeTile = false;
    for (float testY = testPos.y; testY > 1.0f; testY -= 1.0f) {
        testPos.y = testY;
        
        if (!collidesWithBlocks(testPos, glm::vec3(0.0f), world)) {
            // Check if there's a block below
            glm::vec3 groundCheckPos = testPos;
            groundCheckPos.y -= 1.0f;
            
            if (collidesWithBlocks(groundCheckPos, glm::vec3(0.0f, -1.0f, 0.0f), world, true)) {
                // Found a safe position with ground underneath
                foundSafeTile = true;
                break;
            }
        }
    }
    
    // If we couldn't find a safe position, use the original starting height
    if (!foundSafeTile) {
        testPos = pos;
        testPos.y = startHeight;
        
        // Ensure we're at the center of a block horizontally
        testPos.x = std::round(testPos.x);
        testPos.z = std::round(testPos.z);
    }
    
    return testPos;
}

bool CollisionSystem::isBlockSolid(int blockType) const {
    // In this implementation, any block with type > 0 is considered solid
    return blockType > 0;
}

bool CollisionSystem::intersectsWithBlock(const glm::vec3& min, const glm::vec3& max, 
                                        const glm::vec3& blockMin, const glm::vec3& blockMax) const {
    // Use a more precise AABB collision test with a slight expansion at edges
    // This helps catch collisions at block seams
    return (min.x <= blockMax.x + 0.02f && max.x >= blockMin.x - 0.02f &&
            min.y <= blockMax.y + 0.02f && max.y >= blockMin.y - 0.02f &&
            min.z <= blockMax.z + 0.02f && max.z >= blockMin.z - 0.02f);
}

bool CollisionSystem::isInsideBlock(const glm::vec3& pos, World* world) {
    if (!world) return false;
    
    // Check if the center of the player is inside a solid block
    glm::ivec3 blockPos(
        std::floor(pos.x),
        std::floor(pos.y),
        std::floor(pos.z)
    );
    
    // Check the block at the player's position
    int blockType = world->getBlock(blockPos);
    
    // Also check the block at the player's head level
    glm::ivec3 headPos = blockPos;
    headPos.y += 1;
    int headBlockType = world->getBlock(headPos);
    
    return (blockType > 0 || headBlockType > 0);
}

glm::vec3 CollisionSystem::adjustPositionOutOfBlock(const glm::vec3& pos, World* world) {
    if (!world) return pos;
    
    glm::vec3 adjustedPos = pos;
    const float adjustAmount = 0.05f;
    
    // Try adjusting in various directions to find a non-colliding position
    // Directions to try: up (most likely to work), then cardinal directions
    std::vector<glm::vec3> directions = {
        glm::vec3(0, 1, 0),     // Up
        glm::vec3(0, -1, 0),    // Down
        glm::vec3(1, 0, 0),     // Right
        glm::vec3(-1, 0, 0),    // Left
        glm::vec3(0, 0, 1),     // Forward
        glm::vec3(0, 0, -1),    // Back
        // Diagonals with smaller adjustments
        glm::vec3(0.7f, 0.7f, 0),     // Up-Right
        glm::vec3(-0.7f, 0.7f, 0),    // Up-Left
        glm::vec3(0, 0.7f, 0.7f),     // Up-Forward
        glm::vec3(0, 0.7f, -0.7f)     // Up-Back
    };
    
    // Try each direction with increasing distance
    for (float distance = adjustAmount; distance <= adjustAmount * 3; distance += adjustAmount) {
        for (const auto& dir : directions) {
            glm::vec3 testPos = pos + dir * distance;
            
            // Check if this position is free of blocks
            if (!isInsideBlock(testPos, world)) {
                return testPos;
            }
        }
    }
    
    // If all else fails, try moving up significantly 
    adjustedPos.y += 1.0f;
    
    // Still inside? Try finding a completely new position
    if (isInsideBlock(adjustedPos, world)) {
        // Final attempt - move to block center and up by player height + some margin
        adjustedPos.x = std::round(pos.x) + 0.5f;
        adjustedPos.z = std::round(pos.z) + 0.5f;
        
        // Find the ground level at this position by scanning from below
        adjustedPos.y = 0.0f;
        while (adjustedPos.y < 256.0f) {  // World height limit
            if (!isInsideBlock(adjustedPos, world)) {
                adjustedPos.y += m_height + 0.2f;  // Put player above block
                return adjustedPos;
            }
            adjustedPos.y += 1.0f;
        }
    }
    
    return adjustedPos;
}

bool CollisionSystem::collidesWithBlocksGreedy(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision) {
    if (!world) return false;

    // Debug counter for output throttling
    static int debugCounter = 0;
    bool verboseDebug = m_debugMode && (debugCounter++ % 120 == 0); // Output logs every ~2 seconds

    // CRITICAL FIX: Treat position as the player's feet, and adjust the collision box calculation
    // The collision box should be centered horizontally around the player, but start at their feet
    glm::vec3 min, max;
    
    // Significantly increase horizontal insets to prevent camera clipping through walls
    // For vertical box, keep it close to player height to allow proper jumping
    
    if (useWiderCollision) {
        // Use a much wider collision box for chunk boundary detection and wall clipping prevention
        min = pos + glm::vec3(-(m_width/2 + 0.5f), 0.0f, -(m_width/2 + 0.5f));
        max = pos + glm::vec3(m_width/2 + 0.5f, m_height, m_width/2 + 0.5f);
    } else {
        // Use normal collision box but still with increased width to prevent camera clipping
        min = pos + glm::vec3(-(m_width/2 + 0.3f), 0.0f, -(m_width/2 + 0.3f));
        max = pos + glm::vec3(m_width/2 + 0.3f, m_height, m_width/2 + 0.3f);
    }
    
    if (verboseDebug) {
        std::cout << "==== GREEDY COLLISION CHECK ====" << std::endl;
        std::cout << "Player position (feet): (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        std::cout << "Collision box min: (" << min.x << ", " << min.y << ", " << min.z << ")" << std::endl;
        std::cout << "Collision box max: (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
        std::cout << "Collision box width: " << (max.x - min.x) << ", height: " << (max.y - min.y) 
                  << ", depth: " << (max.z - min.z) << std::endl;
        
        if (useWiderCollision) {
            std::cout << "Using wider collision box for chunk boundary detection" << std::endl;
        }
    }
    
    // CRITICAL FIX: Check if the player's feet are below ground level
    // This is a special case to immediately register as a collision
    if (pos.y < 0) {
        if (verboseDebug) {
            std::cout << "Player below world bottom, collision detected" << std::endl;
        }
        
        // Force the player to be above y=0
        const_cast<glm::vec3&>(pos).y = 0.1f;
        return true;
    }
    
    // Track the highest ground position for positioning the player properly
    float highestGroundY = -1.0f;
    
    // Calculate min/max chunk coordinates for checking collisions
    int minChunkX = static_cast<int>(std::floor(min.x / world->CHUNK_SIZE));
    int maxChunkX = static_cast<int>(std::floor(max.x / world->CHUNK_SIZE));
    int minChunkY = static_cast<int>(std::floor(min.y / world->CHUNK_HEIGHT));
    int maxChunkY = static_cast<int>(std::floor(max.y / world->CHUNK_HEIGHT));
    int minChunkZ = static_cast<int>(std::floor(min.z / world->CHUNK_SIZE));
    int maxChunkZ = static_cast<int>(std::floor(max.z / world->CHUNK_SIZE));
    
    if (verboseDebug) {
        std::cout << "Checking chunks from (" << minChunkX << ", " << minChunkY << ", " << minChunkZ << ") to ("
                  << maxChunkX << ", " << maxChunkY << ", " << maxChunkZ << ")" << std::endl;
    }
    
    // Count collision volumes checked and collisions found for debugging
    int volumesChecked = 0;
    int collisionsFound = 0;
    
    // OPTIMIZATION: Cache chunk access for better performance
    std::vector<const Chunk*> chunksToCheck;
    chunksToCheck.reserve((maxChunkX - minChunkX + 1) * (maxChunkY - minChunkY + 1) * (maxChunkZ - minChunkZ + 1));
    
    // First gather all chunks that need to be checked
    const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosHash>& chunks = world->getChunks();
    for (int chunkX = minChunkX; chunkX <= maxChunkX; chunkX++) {
        for (int chunkZ = minChunkZ; chunkZ <= maxChunkZ; chunkZ++) {
            for (int chunkY = minChunkY; chunkY <= maxChunkY; chunkY++) {
                glm::ivec3 chunkPos(chunkX, chunkY, chunkZ);
                auto it = chunks.find(chunkPos);
                if (it != chunks.end()) {
                    chunksToCheck.push_back(it->second.get());
                }
            }
        }
    }
    
    // Check collision with each chunk's collision mesh
    for (const Chunk* chunk : chunksToCheck) {
        // Get collision mesh for this chunk
        std::vector<AABB> collisionMesh = chunk->buildColliderMesh();
        
        if (verboseDebug && !collisionMesh.empty()) {
            glm::ivec3 chunkPos = chunk->getPosition();
            std::cout << "Chunk (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") has " 
                      << collisionMesh.size() << " collision volumes" << std::endl;
        }
        
        // Test collision against each volume in the mesh
        volumesChecked += collisionMesh.size();
        
        for (const AABB& volume : collisionMesh) {
            // Special case for ground detection - improved to be more precise
            if (velocity.y <= 0 && min.y <= volume.max.y && pos.y >= volume.min.y && pos.y <= volume.max.y + 0.2f) {
                // Check if we're over this volume - use a slightly larger area for stability
                if (min.x - 0.1f <= volume.max.x && max.x + 0.1f >= volume.min.x && 
                    min.z - 0.1f <= volume.max.z && max.z + 0.1f >= volume.min.z) {
                    // This is a potential ground - track the highest one
                    if (volume.max.y > highestGroundY) {
                        highestGroundY = volume.max.y;
                    }
                    
                    if (verboseDebug) {
                        std::cout << "Ground detection: Volume at (" << volume.min.x << ", " << volume.min.y << ", " << volume.min.z 
                                << ") to (" << volume.max.x << ", " << volume.max.y << ", " << volume.max.z << ")" << std::endl;
                    }
                }
            }
            
            // Check for collision with this volume
            if (intersectsWithAABB(min, max, volume.min, volume.max)) {
                collisionsFound++;
                
                if (verboseDebug) {
                    std::cout << "Collision with volume at (" << volume.min.x << ", " << volume.min.y << ", " << volume.min.z 
                            << ") to (" << volume.max.x << ", " << volume.max.y << ", " << volume.max.z << ")" << std::endl;
                }
                
                // If we found a collision, we can stop here unless we need to check ground
                if (velocity.y > 0 || highestGroundY == -1.0f) {
                    return true;
                }
            }
        }
    }
    
    // Check if we found a valid ground position
    bool groundCollision = (highestGroundY != -1.0f);
    
    // If we found ground, adjust player position to rest exactly on top of it
    if (groundCollision && !useWiderCollision) {
        // This casting hack allows us to modify the position even though it's a const parameter
        glm::vec3& mutablePos = const_cast<glm::vec3&>(pos);
        
        // Place the player's feet exactly on top of the highest ground with a significant offset
        // This prevents sinking through the surface due to floating point precision issues
        mutablePos.y = highestGroundY + 0.01f; // Increased from 0.001f to provide more clearance
    }
    
    // FAILSAFE: If no chunks were checked, assume we're falling through the world
    if (chunksToCheck.empty() && velocity.y < 0 && pos.y < 100) {
        // Prevent falling through the world when chunks haven't loaded yet
        glm::vec3& mutablePos = const_cast<glm::vec3&>(pos);
        // Keep player at current height until chunks load
        mutablePos.y = std::max(mutablePos.y, 0.1f); 
        
        if (verboseDebug) {
            std::cout << "FAILSAFE: No chunks loaded, preventing fall-through at y=" << mutablePos.y << std::endl;
        }
        
        return true;
    }
    
    if (verboseDebug) {
        std::cout << "Collision volumes checked: " << volumesChecked << std::endl;
        std::cout << "Collisions found: " << collisionsFound << std::endl;
        std::cout << "Highest ground Y: " << highestGroundY << std::endl;
        std::cout << "Final collision result: " << ((collisionsFound > 0 || groundCollision) ? "TRUE" : "FALSE") << std::endl;
        std::cout << "=============================" << std::endl;
    }
    
    return collisionsFound > 0 || groundCollision;
}

bool CollisionSystem::intersectsWithAABB(const glm::vec3& min1, const glm::vec3& max1,
                                     const glm::vec3& min2, const glm::vec3& max2) const {
    // Increased epsilon values to prevent sinking and improve collision detection
    const float epsilonXZ = 0.02f;
    const float epsilonY = 0.01f;  // Increased from 0.001f to prevent sinking
    
    return (min1.x <= max2.x + epsilonXZ && max1.x >= min2.x - epsilonXZ &&
            min1.y <= max2.y + epsilonY && max1.y >= min2.y - epsilonY &&
            min1.z <= max2.z + epsilonXZ && max1.z >= min2.z - epsilonXZ);
} 