#include "physics/CollisionSystem.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

CollisionSystem::CollisionSystem()
    : m_width(0.6f)   // Base width - actual collision uses smaller values
    , m_height(1.8f)  // Standard player height
    , m_collisionBox(0.25f, 1.8f, 0.25f) // Default collision box size
    , m_collisionInset(0.05f)
    , m_edgeThreshold(0.05f)
    , m_verticalStepSize(0.02f)
    , m_debugMode(true)
    , m_useGreedyMeshing(true) // Enable greedy meshing by default
    , m_collisionEnabled(true)
{
    // Additional initialization can go here if needed
}

void CollisionSystem::init(float width, float height) {
    // Always use a fixed width for consistent collision 
    m_width = width;
    m_height = height;
    
    // Initialize the collision box with appropriate dimensions
    m_collisionBox = CollisionBox(0.25f, height, 0.25f);
}

bool CollisionSystem::collidesWithBlocks(const glm::vec3& pos, const glm::vec3& velocity, World* world, bool useWiderCollision) {
    // Use greedy meshing if enabled
    if (m_useGreedyMeshing) {
        return collidesWithBlocksGreedy(pos, velocity, world, useWiderCollision);
    }
    
    if (!world) return false;
    
    // Create collision box with consistent size
    // Important: pos is now treated as the player's feet position (bottom center)
    glm::vec3 min, max;
    
    // Get the appropriate collision box based on movement characteristics
    CollisionBox activeBox = m_collisionBox;
    
    // When falling, use a slightly different collision box to prevent getting stuck on walls
    bool isFalling = velocity.y < 0;
    
    // Apply different collision strategies based on movement
    if (isFalling) {
        // Use a smaller collision box when falling to avoid getting stuck on edges
        activeBox = m_collisionBox.getSmallerBox(0.2f);
        // Add a bit of vertical offset when falling to avoid catching on edges
        activeBox.offset.y = 0.2f;
    } else if (std::abs(velocity.y) < 0.1f && (std::abs(velocity.x) > 0.01f || std::abs(velocity.z) > 0.01f)) {
        // If moving horizontally with little vertical movement, use a slightly narrower box
        activeBox = m_collisionBox.getSmallerBox(0.1f);
    }
    
    // For ground detection, use a slightly wider box
    if (useWiderCollision) {
        // Expand the collision box slightly
        activeBox.width *= 1.1f;
        activeBox.depth *= 1.1f;
    }
    
    // Calculate the bounds based on our active collision box
    min = activeBox.getMin(pos);
    max = activeBox.getMax(pos);
    
    // Debug output for collision checking
    static int debugCounter = 0;
    bool verboseDebug = m_debugMode && (debugCounter++ % 180 == 0); // Output detailed logs every ~3 seconds at 60fps
    
    if (verboseDebug) {
        std::cout << "==== COLLISION CHECK ====" << std::endl;
        std::cout << "Player position (feet): (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        std::cout << "Collision box min: (" << min.x << ", " << min.y << ", " << min.z << ")" << std::endl;
        std::cout << "Collision box max: (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
        std::cout << "Collision box width: " << max.x - min.x << ", height: " << max.y - min.y << ", depth: " << max.z - min.z << std::endl;
        std::cout << "Is falling: " << (isFalling ? "YES" : "NO") << std::endl;
        std::cout << "Using wider collision: " << (useWiderCollision ? "YES (ground detection)" : "NO") << std::endl;
    }
    
    // CRITICAL FIX: If player's feet are below ground level, consider it a collision immediately
    if (pos.y < 0.0f) {
        if (verboseDebug) {
            std::cout << "Player is below ground level (y < 0), forcing collision" << std::endl;
        }
        return true;
    }
    
    // Expand checking area slightly to catch edge cases
    int minX = static_cast<int>(std::floor(min.x - 0.01f));
    int minY = static_cast<int>(std::floor(min.y - 0.01f));
    int minZ = static_cast<int>(std::floor(min.z - 0.01f));
    int maxX = static_cast<int>(std::floor(max.x + 0.01f));
    int maxY = static_cast<int>(std::floor(max.y + 0.01f));
    int maxZ = static_cast<int>(std::floor(max.z + 0.01f));
    
    // Ensure minY is not negative to prevent issues with array indexing
    minY = std::max(0, minY);
    
    if (verboseDebug) {
        std::cout << "Checking blocks from (" << minX << ", " << minY << ", " << minZ << ") to ("
                  << maxX << ", " << maxY << ", " << maxZ << ")" << std::endl;
    }
    
    // Add a special case for detecting ground collision explicitly
    // This helps distinguish between ground collision (should stop vertical movement)
    // and wall collision (should allow sliding)
    bool groundCollision = false;
    if (velocity.y <= 0 && useWiderCollision) {
        // Check for blocks directly below the player in a wider area to ensure we don't miss ground
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
        
        // If we are checking for ground and found it, return immediately
        if (groundCollision) {
            if (verboseDebug) {
                std::cout << "Used ground-specific collision detection, returned: TRUE" << std::endl;
                std::cout << "========================" << std::endl;
            }
            return true;
        }
    }
    
    // Debug flag to count collisions
    int collisionCount = 0;
    
    // If this is a falling check (vertical movement) and not a ground check,
    // we want to be more lenient with wall collisions to allow sliding
    bool ignoreSideCollisions = isFalling && !useWiderCollision && velocity.y < -0.1f;
    
    // Main collision check - look at all blocks in the expanded bounds
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                // Skip invalid coordinates
                if (y < 0) continue;
                
                glm::ivec3 blockPos(x, y, z);
                int blockType = world->getBlock(blockPos);
                
                // Use the same logic as rendering to determine solid blocks
                if (isBlockSolid(blockType)) { // Non-air block found
                    collisionCount++;
                    
                    // Get the precise collision box for this block
                    glm::vec3 blockMin(x, y, z);
                    glm::vec3 blockMax(x + 1.0f, y + 1.0f, z + 1.0f);
                    
                    // Special handling for falling: if we're falling and this is a side block,
                    // we want to be more lenient to allow sliding
                    if (ignoreSideCollisions) {
                        // Check if this is a side/wall block or a block below
                        bool isSideBlock = (pos.y > y + 0.5f);  // Player feet are above the middle of the block
                        
                        if (isSideBlock) {
                            // For side blocks during falling, use a reduced collision box
                            glm::vec3 reducedMin = pos + glm::vec3(-activeBox.width * 0.7f, activeBox.offset.y, -activeBox.width * 0.7f);
                            glm::vec3 reducedMax = pos + glm::vec3(activeBox.width * 0.7f, m_height - 0.05f, activeBox.width * 0.7f);
                            
                            // Use the reduced box for side collisions when falling
                            if (!intersectsWithBlock(reducedMin, reducedMax, blockMin, blockMax)) {
                                // Skip this collision - allows sliding down beside walls
                                continue;
                            }
                        }
                    }
                    
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
        std::cout << "Solid blocks found: " << collisionCount << std::endl;
        std::cout << "Final collision result: " << (groundCollision ? "TRUE (ground)" : "FALSE") << std::endl;
        std::cout << "========================" << std::endl;
    }
    
    return groundCollision;
}

bool CollisionSystem::checkGroundCollision(const glm::vec3& pos, const glm::vec3& velocity, World* world) {
    if (!world) return false;
    
    // Use the collision box dimensions for ground detection
    const float groundCheckWidth = m_collisionBox.width * 0.9f; // Slightly smaller than collision width
    glm::vec3 groundCheckPos = pos;
    const float groundCheckDepth = 0.1f; // How far below the player to check for ground
    
    // First do a direct center point check
    int centerBlockType = world->getBlock(glm::ivec3(
        std::floor(groundCheckPos.x), 
        std::floor(groundCheckPos.y - groundCheckDepth), 
        std::floor(groundCheckPos.z)));
    
    // If there's a block directly below center, we're on ground
    if (isBlockSolid(centerBlockType)) {
        return true;
    }
    
    // Edge detection - check if we're partially standing on an edge 
    // We want the player to be able to stand on edges if more than half the player width is supported
    
    // Check a circle around the player's feet - 6 points for better precision
    const int numSamplePoints = 6;
    const float radius = groundCheckWidth * 0.75f; // 75% of collision width
    int solidPoints = 0;
    
    for (int i = 0; i < numSamplePoints; i++) {
        float angle = (float)i * (2.0f * M_PI / numSamplePoints);
        float sampleX = groundCheckPos.x + radius * cos(angle);
        float sampleZ = groundCheckPos.z + radius * sin(angle);
        
        int blockType = world->getBlock(glm::ivec3(
            std::floor(sampleX),
            std::floor(groundCheckPos.y - groundCheckDepth),
            std::floor(sampleZ)));
            
        if (isBlockSolid(blockType)) {
            solidPoints++;
        }
    }
    
    // If we have a significant number of points on solid ground, consider the player grounded
    // This allows standing on edges but prevents standing on very small corners
    if (solidPoints >= numSamplePoints / 3) {
        return true;
    }
    
    // For edges, also check if we're moving toward the edge to improve stability
    if (solidPoints > 0 && velocity.y <= 0.0f) {
        // Check a smaller inner circle for more precise ground detection
        const float innerRadius = groundCheckWidth * 0.4f;
        int innerSolidPoints = 0;
        
        for (int i = 0; i < numSamplePoints; i++) {
            float angle = (float)i * (2.0f * M_PI / numSamplePoints);
            float sampleX = groundCheckPos.x + innerRadius * cos(angle);
            float sampleZ = groundCheckPos.z + innerRadius * sin(angle);
            
            int blockType = world->getBlock(glm::ivec3(
                std::floor(sampleX),
                std::floor(groundCheckPos.y - groundCheckDepth),
                std::floor(sampleZ)));
                
            if (isBlockSolid(blockType)) {
                innerSolidPoints++;
            }
        }
        
        // If we have any inner points on solid ground, we can stand
        if (innerSolidPoints > 0) {
            return true;
        }
    }
    
    // Not standing on ground
    return false;
}

bool CollisionSystem::isPositionSafe(const glm::vec3& pos, World* world) {
    if (!world) return false;
    
    // Check if position is not inside a block
    if (collidesWithBlocks(pos, glm::vec3(0.0f), world)) {
        return false;
    }
    
    return true;
}

glm::vec3 CollisionSystem::moveWithCollision(const glm::vec3& currentPos, const glm::vec3& movement, 
                              const glm::vec3& velocity, World* world, bool isFlying, bool& isOnGround) {
    // If collision is disabled, just move freely
    if (!m_collisionEnabled) {
        isOnGround = false;
        return currentPos + movement;
    }
    
    if (!world) return currentPos;
    
    // Debug flag
    static int debugCounter = 0;
    bool verboseDebug = m_debugMode && (debugCounter++ % 180 == 0); // Output every ~3 seconds at 60fps
    
    // Early exit for no movement
    if (glm::length(movement) < 0.0001f) {
        if (verboseDebug) {
            std::cout << "==== MOVE WITH COLLISION ====" << std::endl;
            std::cout << "Starting position: (" << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << ")" << std::endl;
            std::cout << "Movement vector: (" << movement.x << ", " << movement.y << ", " << movement.z << ")" << std::endl;
            std::cout << "Final position after movement: (" << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << ")" << std::endl;
            std::cout << "Movement iterations: 0" << std::endl;
            std::cout << "Collisions encountered: 0" << std::endl;
            std::cout << "===========================" << std::endl;
        }
        return currentPos;
    }
    
    // Split movement into horizontal and vertical components for better collision handling
    glm::vec3 horizontalMovement(movement.x, 0.0f, movement.z);
    glm::vec3 verticalMovement(0.0f, movement.y, 0.0f);
    
    // Capture starting position for debug and rollback
    glm::vec3 startPos = currentPos;
    glm::vec3 newPos = currentPos;
    
    // Variables to track iterations and collisions
    int movementIterations = 0;
    int collisionCount = 0;
    
    if (verboseDebug) {
        std::cout << "==== MOVE WITH COLLISION ====" << std::endl;
        std::cout << "Starting position: (" << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << ")" << std::endl;
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
            newPos = adjustedPos;
        } else if (verboseDebug) {
            std::cout << "WARNING: Starting inside a block and couldn't adjust position" << std::endl;
        }
    }
    
    // Process vertical movement first to prevent getting stuck when landing next to a wall
    if (glm::length(verticalMovement) > 0.0001f) {
        float verticalDistance = glm::length(verticalMovement);
        float verticalStepSize = m_verticalStepSize; // Use the class member for consistency
        int numVerticalSteps = std::max(1, static_cast<int>(verticalDistance / verticalStepSize) * 2); // Double step resolution
        glm::vec3 verticalStep = verticalMovement / static_cast<float>(numVerticalSteps);
        bool movingUp = verticalMovement.y > 0.0f;
        
        for (int i = 0; i < numVerticalSteps; i++) {
            movementIterations++;
            
            // Try the vertical step
            glm::vec3 nextPos = newPos + verticalStep;
            
            // CRITICAL FIX: For vertical movement, we need to handle collisions differently
            // We should only stop vertical movement when hitting the ground (below) or ceiling (above)
            // For walls, we should allow sliding down instead of getting stuck
            
            if (movingUp) {
                // When moving up, we only care about ceiling collisions
                // Use a reduced collision box that only captures the player's head
                glm::vec3 ceilingCheckPos = nextPos;
                ceilingCheckPos.y = nextPos.y + m_height - 0.1f; // Check near the top of the player
                
                // First, check for wall collisions on the sides that might be stopping upward movement
                bool hittingWall = false;
                glm::vec3 sideNormal(0.0f);
                
                // Check sides for walls when moving up
                const float sideCheckDistance = 0.35f;
                const float sideNudgeAmount = 0.15f;
                
                // Check in 4 horizontal directions for walls
                const glm::vec3 checkDirections[4] = {
                    glm::vec3(1.0f, 0.0f, 0.0f),  // +X
                    glm::vec3(-1.0f, 0.0f, 0.0f), // -X
                    glm::vec3(0.0f, 0.0f, 1.0f),  // +Z
                    glm::vec3(0.0f, 0.0f, -1.0f)  // -Z
                };
                
                for (int dir = 0; dir < 4; dir++) {
                    // Check if there's a wall in this direction
                    glm::vec3 sideCheckPos = nextPos + checkDirections[dir] * 0.3f; // Check near the player's sides
                    
                    glm::ivec3 blockPos(
                        std::floor(sideCheckPos.x),
                        std::floor(sideCheckPos.y + 0.5f), // Check at middle height
                        std::floor(sideCheckPos.z)
                    );
                    
                    int blockType = world->getBlock(blockPos);
                    if (isBlockSolid(blockType)) {
                        hittingWall = true;
                        sideNormal -= checkDirections[dir]; // Accumulate normals pointing away from walls
                    }
                }
                
                // If hitting a wall while moving up, try to slide along it
                if (hittingWall && glm::length(sideNormal) > 0.001f) {
                    sideNormal = glm::normalize(sideNormal);
                    
                    // Calculate slide vector - push away from wall and maintain upward movement
                    glm::vec3 slideVector = sideNormal * 0.1f;
                    glm::vec3 slidingNextPos = nextPos + slideVector;
                    
                    // Only use this if it doesn't cause another collision
                    if (!collidesWithBlocks(slidingNextPos, velocity, world, false)) {
                        nextPos = slidingNextPos;
                        if (verboseDebug) {
                            std::cout << "Applied wall slide while moving up" << std::endl;
                        }
                    }
                }
                
                // Check if there's a solid block above the player's head
                glm::ivec3 blockPos(
                    std::floor(ceilingCheckPos.x),
                    std::floor(ceilingCheckPos.y),
                    std::floor(ceilingCheckPos.z)
                );
                
                bool ceilingCollision = false;
                int blockType = world->getBlock(blockPos);
                if (isBlockSolid(blockType)) {
                    // We've hit a ceiling, check if we can nudge around it
                    const float edgeCheckDistance = 0.35f;
                    const float nudgeAmount = 0.15f;
                    
                    bool edgeFound = false;
                    glm::vec3 edgeDirection(0.0f);
                    
                    // Check in 4 horizontal directions for a possible edge
                    const glm::vec3 checkDirections[4] = {
                        glm::vec3(1.0f, 0.0f, 0.0f),  // +X
                        glm::vec3(-1.0f, 0.0f, 0.0f), // -X
                        glm::vec3(0.0f, 0.0f, 1.0f),  // +Z
                        glm::vec3(0.0f, 0.0f, -1.0f)  // -Z
                    };
                    
                    for (int dir = 0; dir < 4; dir++) {
                        // Check if there's space in this direction
                        glm::vec3 edgeCheckPos = nextPos + checkDirections[dir] * edgeCheckDistance;
                        
                        // If there's no collision in this direction and above, it might be an edge
                        if (!collidesWithBlocks(edgeCheckPos, velocity, world, false)) {
                            edgeDirection = checkDirections[dir];
                            edgeFound = true;
                            break;
                        }
                    }
                    
                    // If we found a potential edge, try nudging the player in that direction
                    if (edgeFound) {
                        glm::vec3 nudgedPos = newPos + edgeDirection * nudgeAmount + verticalStep;
                        
                        if (!collidesWithBlocks(nudgedPos, velocity, world, false)) {
                            // We've successfully nudged the player past the obstacle
                            nextPos = nudgedPos;
                            if (verboseDebug) {
                                std::cout << "Applied ceiling edge nudge" << std::endl;
                            }
                        } else {
                            // If nudging didn't work, we hit a solid ceiling
                            ceilingCollision = true;
                            if (verboseDebug) {
                                std::cout << "Hit solid ceiling" << std::endl;
                            }
                        }
                    } else {
                        // No edge found, solid ceiling
                        ceilingCollision = true;
                        if (verboseDebug) {
                            std::cout << "Hit solid ceiling, no edge found" << std::endl;
                        }
                    }
                }
                
                if (ceilingCollision) {
                    // We've hit a solid ceiling with no way around it
                    // Adjust position to be just below the ceiling
                    nextPos.y = std::floor(ceilingCheckPos.y) - m_height + 0.01f;
                    
                    // Also apply a reverse velocity immediately to prevent sticking
                    // This helps especially when jumping directly into a ceiling
                    if (velocity.y > 0.5f) {
                        const_cast<glm::vec3&>(velocity).y = -velocity.y * 0.1f; // Apply negative bounce with dampening
                        if (verboseDebug) {
                            std::cout << "Applied ceiling bounce to prevent sticking" << std::endl;
                        }
                    }
                    
                    break;
                }
            } 
            else { // Moving down
                // CRITICAL CHANGE: For downward movement, we only want to stop if hitting the ground
                // not if touching a wall. This allows sliding down walls.
                
                // First, check for ground collision specifically
                bool groundCollision = checkGroundCollision(nextPos, velocity, world);
                
                if (groundCollision) {
                    // We've hit the ground, stop vertical movement
                    isOnGround = true;
                    // Adjust position to be just above the ground
                    float groundY = std::floor(nextPos.y);
                    nextPos.y = groundY + 0.01f; // Small offset to ensure we're above the ground
                    if (verboseDebug) {
                        std::cout << "Hit ground at y=" << groundY << std::endl;
                    }
                    break;
                }
                
                // Enhanced wall collision detection during falling
                bool wallCollision = false;
                glm::vec3 nudgeDirection(0.0f);
                int wallCollisionCount = 0;
                
                // Use a cylinder-like collision detection with multiple sample points
                const float collisionWidth = 0.3f;
                const glm::vec3 sideOffsets[4] = {
                    glm::vec3(-collisionWidth, 0.5f, 0),  // Left
                    glm::vec3(collisionWidth, 0.5f, 0),   // Right
                    glm::vec3(0, 0.5f, -collisionWidth),  // Back
                    glm::vec3(0, 0.5f, collisionWidth)    // Front
                };
                
                // Check each side for wall collisions
                for (int side = 0; side < 4; side++) {
                    glm::vec3 checkPos = nextPos + sideOffsets[side];
                    glm::ivec3 blockPos(
                        std::floor(checkPos.x),
                        std::floor(checkPos.y),
                        std::floor(checkPos.z)
                    );
                    
                    int blockType = world->getBlock(blockPos);
                    if (isBlockSolid(blockType)) {
                        wallCollisionCount++;
                        
                        // Calculate direction to nudge the player away from the wall
                        nudgeDirection -= sideOffsets[side];
                        wallCollision = true;
                    }
                }
                
                // Check for vertical edges specifically (diagonal corners) at multiple heights
                bool verticalEdgeDetected = false;
                glm::vec3 edgeAvoidanceNormal(0.0f);
                
                // Check corners to detect edges
                const glm::vec3 cornerOffsets[4] = {
                    glm::vec3(-collisionWidth, 0.5f, -collisionWidth),  // Back-Left
                    glm::vec3(collisionWidth, 0.5f, -collisionWidth),   // Back-Right
                    glm::vec3(-collisionWidth, 0.5f, collisionWidth),   // Front-Left
                    glm::vec3(collisionWidth, 0.5f, collisionWidth)     // Front-Right
                };
                
                int cornerCollisions = 0;
                for (int corner = 0; corner < 4; corner++) {
                    glm::vec3 checkPos = nextPos + cornerOffsets[corner];
                    glm::ivec3 blockPos(
                        std::floor(checkPos.x),
                        std::floor(checkPos.y),
                        std::floor(checkPos.z)
                    );
                    
                    int blockType = world->getBlock(blockPos);
                    if (isBlockSolid(blockType)) {
                        cornerCollisions++;
                        edgeAvoidanceNormal -= glm::normalize(glm::vec3(cornerOffsets[corner].x, 0, cornerOffsets[corner].z));
                    }
                }
                
                // If we have corner collisions but not many side collisions,
                // it's likely we're hitting vertical edges
                if (cornerCollisions > 0 && wallCollisionCount <= 1) {
                    verticalEdgeDetected = true;
                    if (verboseDebug) {
                        std::cout << "Vertical edge detected during falling" << std::endl;
                    }
                }
                
                // If we found vertical edge collision, handle it specially
                if (verticalEdgeDetected && glm::length(edgeAvoidanceNormal) > 0.001f) {
                    edgeAvoidanceNormal = glm::normalize(edgeAvoidanceNormal);
                    
                    // Try stronger horizontal nudges for edges
                    const float edgeNudgeDistances[] = {0.05f, 0.1f, 0.15f};
                    bool nudgeSuccessful = false;
                    
                    for (float nudgeDistance : edgeNudgeDistances) {
                        // Apply horizontal nudge away from the edge
                        glm::vec3 nudgedPos = nextPos + edgeAvoidanceNormal * nudgeDistance;
                        
                        // Check if the nudge resolves the collision
                        if (!collidesWithBlocks(nudgedPos, velocity, world, false)) {
                            // Successfully nudged away from edge
                            nextPos = nudgedPos;
                            nudgeSuccessful = true;
                            
                            if (verboseDebug) {
                                std::cout << "Successfully nudged away from vertical edge by " 
                                          << nudgeDistance << std::endl;
                            }
                                break;
                        }
                    }
                    
                    // If standard nudging didn't work, try with a slight upward component
                    if (!nudgeSuccessful) {
                        // Try nudge with upward component
                        glm::vec3 upwardEdgeNormal = glm::normalize(edgeAvoidanceNormal + glm::vec3(0.0f, 0.1f, 0.0f));
                        
                        for (float nudgeDistance : edgeNudgeDistances) {
                            glm::vec3 nudgedPos = nextPos + upwardEdgeNormal * nudgeDistance;
                            
                            if (!collidesWithBlocks(nudgedPos, velocity, world, false)) {
                                // Successfully nudged with upward component
                                nextPos = nudgedPos;
                                nudgeSuccessful = true;
                                
                                if (verboseDebug) {
                                    std::cout << "Successfully nudged away from vertical edge with upward component by " 
                                              << nudgeDistance << std::endl;
                                }
                                break;
                        }
                    }
                }
                
                    // If we successfully nudged, continue falling
                    if (nudgeSuccessful) {
                        newPos = nextPos;
                        continue;
                    }
                }
                
                // If we're colliding with walls while moving down, try to slide along them
                // This is the critical part that allows sliding down walls
                if (wallCollision && !groundCollision) {
                    if (wallCollisionCount >= 3) {
                        // If we're in a corner with multiple walls, we should slow down
                        // but still allow some downward movement to prevent getting stuck
                        nextPos.y = newPos.y + verticalStep.y * 0.5f; // Reduced vertical movement
                        
                        if (verboseDebug) {
                            std::cout << "Multiple wall collisions, slowing descent" << std::endl;
                        }
                    } else if (glm::length(nudgeDirection) > 0.001f) {
                        // We have a clear direction to nudge
                        nudgeDirection = glm::normalize(nudgeDirection);
                        
                        // Apply horizontal nudge and continue downward movement
                        float nudgeAmount = 0.05f;
                        glm::vec3 nudgedPos = nextPos + nudgeDirection * nudgeAmount;
                        
                        // Check if the nudge resolves the collision
                    if (!collidesWithBlocks(nudgedPos, velocity, world, false)) {
                            // Successfully nudged away from wall
                        nextPos = nudgedPos;
                            
                        if (verboseDebug) {
                                std::cout << "Successfully nudged away from wall while sliding down" << std::endl;
                        }
                    } else {
                            // If nudging didn't work, still allow some downward movement
                            // but keep horizontal position the same to stay close to the wall
                            nextPos.x = newPos.x;
                            nextPos.z = newPos.z;
                            
                            if (verboseDebug) {
                                std::cout << "Sliding straight down wall" << std::endl;
                            }
                        }
                    } else {
                        // No clear nudge direction, maintain vertical movement but keep horizontal position
                        nextPos.x = newPos.x;
                        nextPos.z = newPos.z;
                        
                        if (verboseDebug) {
                            std::cout << "Sliding straight down wall (no nudge direction)" << std::endl;
                        }
                    }
                }
                
                // Apply the position update after wall sliding logic
                newPos = nextPos;
            }
            
            // No ground collision, proceed with the step
            newPos = nextPos;
        }
    } else {
        // Even if no vertical movement, check if we're on ground
        isOnGround = checkGroundCollision(newPos, velocity, world);
    }
    
    // Process horizontal movement after vertical position is established
    if (glm::length(horizontalMovement) > 0.0001f) {
        // Calculate how many steps to use based on movement distance
        float totalHorizontalDistance = glm::length(horizontalMovement);
        float stepSize = 0.05f; // Smaller steps for more precise collision detection
        int numSteps = std::max(1, static_cast<int>(totalHorizontalDistance / stepSize) * 2); // Double step resolution
        glm::vec3 stepVector = horizontalMovement / static_cast<float>(numSteps);
        
        // Apply movement in steps
        for (int i = 0; i < numSteps; i++) {
            movementIterations++;
            
            // Try moving in current direction
            glm::vec3 nextPos = newPos + stepVector;
            
            // Check if we're very close to a wall before moving
            // This helps prevent getting stuck in walls in the first place
            bool veryCloseToWall = false;
            glm::vec3 wallNormal(0.0f);
            
            // Cast rays in multiple directions to detect nearby walls before moving
            const float wallDetectionDistance = 0.075f; // Detect walls very close to player
            
            // Get the current collision box dimensions
            const float collisionWidth = m_collisionBox.width;
            const float collisionDepth = m_collisionBox.depth;
            
            // Points to check around the player's body
            const glm::vec3 checkPoints[] = {
                glm::vec3(0.0f, 0.5f, 0.0f),                    // Center
                glm::vec3(-collisionWidth * 0.9f, 0.5f, 0.0f),  // Left
                glm::vec3(collisionWidth * 0.9f, 0.5f, 0.0f),   // Right
                glm::vec3(0.0f, 0.5f, -collisionDepth * 0.9f),  // Back
                glm::vec3(0.0f, 0.5f, collisionDepth * 0.9f)    // Front
            };
            
            // Check points in all 4 horizontal directions
            const glm::vec3 checkDirs[] = {
                glm::vec3(1.0f, 0.0f, 0.0f),   // +X
                glm::vec3(-1.0f, 0.0f, 0.0f),  // -X
                glm::vec3(0.0f, 0.0f, 1.0f),   // +Z
                glm::vec3(0.0f, 0.0f, -1.0f)   // -Z
            };
            
            for (int p = 0; p < 5; p++) {  // For each check point
                for (int d = 0; d < 4; d++) {  // For each direction
                    glm::vec3 checkPos = newPos + checkPoints[p];
                    glm::vec3 checkEnd = checkPos + checkDirs[d] * wallDetectionDistance;
                    
                    // Check if the end point is inside a block
                    glm::ivec3 blockPos(
                        std::floor(checkEnd.x),
                        std::floor(checkEnd.y),
                        std::floor(checkEnd.z)
                    );
                    
                    int blockType = world->getBlock(blockPos);
                    if (isBlockSolid(blockType)) {
                        veryCloseToWall = true;
                        wallNormal = -checkDirs[d]; // Point away from wall
                        break;
                    }
                }
                if (veryCloseToWall) break;
            }
            
            // If we're very close to a wall, add a small nudge away from it
            if (veryCloseToWall && glm::length(wallNormal) > 0.0f) {
                // Calculate a small amount to nudge away from the wall
                glm::vec3 nudgeVector = wallNormal * 0.02f;
                nextPos += nudgeVector;
                
                if (verboseDebug) {
                    std::cout << "Added wall nudge to prevent sticking: "
                             << nudgeVector.x << ", " << nudgeVector.y << ", " << nudgeVector.z << std::endl;
                }
            }
            
            // Pre-check for preventing wall clipping - look ahead to see if we're about to hit a wall
            bool willHitWall = false;
            
            // Cast rays in the movement direction to detect walls before collision
            const float rayDistance = glm::length(stepVector) * 1.2f; // Slightly longer than step
            const glm::vec3 rayOrigins[] = {
                newPos + glm::vec3(0.0f, 0.5f, 0.0f),                    // Center
                newPos + glm::vec3(-collisionWidth * 0.8f, 0.5f, 0.0f),  // Left
                newPos + glm::vec3(collisionWidth * 0.8f, 0.5f, 0.0f),   // Right
                newPos + glm::vec3(0.0f, 0.5f, -collisionDepth * 0.8f),  // Back
                newPos + glm::vec3(0.0f, 0.5f, collisionDepth * 0.8f)    // Front
            };
            
            // Normalize movement direction for ray casting
            glm::vec3 moveDir = glm::normalize(horizontalMovement);
            
            for (int ray = 0; ray < 5; ray++) {
                glm::vec3 rayEnd = rayOrigins[ray] + moveDir * rayDistance;
                
                // Check if we hit a block along this ray
                glm::ivec3 blockPos(
                    std::floor(rayEnd.x),
                    std::floor(rayEnd.y),
                    std::floor(rayEnd.z)
                );
                
                int blockType = world->getBlock(blockPos);
                if (isBlockSolid(blockType)) {
                    willHitWall = true;
                    
                    // Calculate wall normal - on which axis we're hitting the wall
                    float xDist = std::min(fabs(rayEnd.x - blockPos.x), fabs(rayEnd.x - (blockPos.x + 1.0f)));
                    float zDist = std::min(fabs(rayEnd.z - blockPos.z), fabs(rayEnd.z - (blockPos.z + 1.0f)));
                    
                    if (xDist < zDist) {
                        // Hitting x-axis wall
                        wallNormal.x = rayEnd.x < blockPos.x + 0.5f ? -1.0f : 1.0f;
                    } else {
                        // Hitting z-axis wall
                        wallNormal.z = rayEnd.z < blockPos.z + 0.5f ? -1.0f : 1.0f;
                    }
                    
                    if (verboseDebug) {
                        std::cout << "Ray " << ray << " detected wall at (" 
                                  << blockPos.x << ", " << blockPos.y << ", " << blockPos.z 
                                  << ") with normal " << wallNormal.x << ", " << wallNormal.y 
                                  << ", " << wallNormal.z << std::endl;
                    }
                    break;
                }
            }
            
            // If we're going to hit a wall, adjust movement to slide along it
            if (willHitWall && glm::length(wallNormal) > 0.0f) {
                // Project movement onto the wall plane to slide along it
                glm::vec3 slideDir = stepVector - wallNormal * glm::dot(stepVector, wallNormal);
                float slideMagnitude = glm::length(slideDir);
                
                if (slideMagnitude > 0.001f) {
                    // Use the slide direction instead
                    slideDir = glm::normalize(slideDir) * glm::length(stepVector) * 0.8f; // Reduce speed when sliding
                    nextPos = newPos + slideDir;
                    
                    if (verboseDebug) {
                        std::cout << "Adjusting movement to slide along wall, new direction: " 
                                  << slideDir.x << ", " << slideDir.y << ", " << slideDir.z << std::endl;
                    }
                } else {
                    // Can't slide, stop movement
                    if (verboseDebug) {
                        std::cout << "Cannot slide along wall, stopping movement" << std::endl;
                    }
                    break;
                }
            }
            
            // Check for collision at new position - for horizontal movement we do want to stop
            // if we hit a wall, but we should still try to slide along it
            if (collidesWithBlocks(nextPos, velocity, world, false)) {
                collisionCount++;
                
                // Handle edge case - try sliding along walls instead of stopping completely
                glm::vec3 xMovement(stepVector.x, 0.0f, 0.0f);
                glm::vec3 zMovement(0.0f, 0.0f, stepVector.z);
                
                // Try moving along X axis only
                glm::vec3 xSlidePos = newPos + xMovement;
                bool xCollides = collidesWithBlocks(xSlidePos, velocity, world, false);
                
                // Try moving along Z axis only
                glm::vec3 zSlidePos = newPos + zMovement;
                bool zCollides = collidesWithBlocks(zSlidePos, velocity, world, false);
                
                // Prevent wall-sticking by ensuring we move away from walls
                bool foundSlideDirection = false;
                
                // If one direction is clear but not the other, allow sliding
                if (!xCollides && zCollides) {
                    nextPos = xSlidePos;
                    foundSlideDirection = true;
                    if (verboseDebug) {
                        std::cout << "Sliding along X axis due to Z collision" << std::endl;
                    }
                } else if (xCollides && !zCollides) {
                    nextPos = zSlidePos;
                    foundSlideDirection = true;
                    if (verboseDebug) {
                        std::cout << "Sliding along Z axis due to X collision" << std::endl;
                    }
                }
                
                // If we couldn't slide in either direction
                if (!foundSlideDirection) {
                    // Corner case: Try step-up if player is moving horizontally and near the ground
                    if (isOnGround || std::abs(currentPos.y - std::floor(currentPos.y + 0.1f)) < 0.3f) {
                        // Check if there's a small step up we can climb (up to 0.5 blocks high)
                        glm::vec3 stepUpPos = newPos + glm::vec3(0.0f, 0.5f, 0.0f) + stepVector;
                        
                        if (!collidesWithBlocks(stepUpPos, velocity, world, false)) {
                            // Check if there's ground beneath this position
                            glm::vec3 groundCheckPos = stepUpPos + glm::vec3(0.0f, -0.1f, 0.0f);
                            if (collidesWithBlocks(groundCheckPos, velocity, world, true)) {
                                // We found a step we can climb! Move up and forward
                                nextPos = stepUpPos;
                                if (verboseDebug) {
                                    std::cout << "Stepping up onto ledge" << std::endl;
                                }
                            } else {
                                // If we can't step up, let's try a slight corner smoothing
                                // This helps with diagonal movement at corners
                                glm::vec3 cornerAdjustment = glm::normalize(stepVector) * 0.05f; // Small adjustment
                                glm::vec3 adjustedPos = newPos + cornerAdjustment;
                                
                                // Check if this small adjustment helps avoid the collision
                                if (!collidesWithBlocks(adjustedPos, velocity, world, false)) {
                                    nextPos = adjustedPos;
                                    if (verboseDebug) {
                                        std::cout << "Applied corner smoothing adjustment" << std::endl;
                                    }
                                } else {
                                    // Apply wall repulsion to prevent sticking to walls
                                    // Move slightly away from the wall in the opposite direction
                                    glm::vec3 repulsionDirection = -glm::normalize(stepVector) * 0.02f;
                                    glm::vec3 repulsedPos = newPos + repulsionDirection;
                                    
                                    if (!collidesWithBlocks(repulsedPos, velocity, world, false)) {
                                        nextPos = repulsedPos;
                                        if (verboseDebug) {
                                            std::cout << "Applied wall repulsion" << std::endl;
                                        }
                                    } else {
                                        // If standard repulsion doesn't work, try detecting which wall we're stuck in
                                        // and push away from it specifically
                                        glm::vec3 stuckPosition = newPos;
                                        glm::vec3 potentialEscapeDir(0.0f);
                                        
                                        // Check in cardinal directions to find a wall we might be stuck in
                                        const glm::vec3 cardinalDirs[] = {
                                            glm::vec3(1.0f, 0.0f, 0.0f),   // +X
                                            glm::vec3(-1.0f, 0.0f, 0.0f),  // -X
                                            glm::vec3(0.0f, 0.0f, 1.0f),   // +Z
                                            glm::vec3(0.0f, 0.0f, -1.0f)   // -Z
                                        };
                                        
                                        // Test each direction with increased distance to escape walls
                                        for (int dir = 0; dir < 4; dir++) {
                                            // Test if we're very close to a wall in this direction
                                            glm::vec3 closeTestPos = stuckPosition + cardinalDirs[dir] * 0.05f;
                                            
                                            glm::ivec3 blockPos(
                                                std::floor(closeTestPos.x),
                                                std::floor(closeTestPos.y),
                                                std::floor(closeTestPos.z)
                                            );
                                            
                                            int blockType = world->getBlock(blockPos);
                                            if (isBlockSolid(blockType)) {
                                                // We found a wall, try pushing away in the opposite direction
                                                potentialEscapeDir = -cardinalDirs[dir] * 0.06f; // Stronger push
                                                
                                                // Test if this escape works
                                                glm::vec3 escapePos = stuckPosition + potentialEscapeDir;
                                                if (!collidesWithBlocks(escapePos, velocity, world, false)) {
                                                    nextPos = escapePos;
                                                    if (verboseDebug) {
                                                        std::cout << "Applied enhanced wall escape in direction: " 
                                                                << potentialEscapeDir.x << ", " << potentialEscapeDir.y 
                                                                << ", " << potentialEscapeDir.z << std::endl;
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                        
                                        // If we still couldn't find an escape, as a last resort,
                                        // try moving upward slightly which often helps with vertical walls
                                        if (glm::length(potentialEscapeDir) < 0.001f) {
                                            glm::vec3 upwardEscapePos = stuckPosition + glm::vec3(0.0f, 0.1f, 0.0f);
                                            if (!collidesWithBlocks(upwardEscapePos, velocity, world, false)) {
                                                nextPos = upwardEscapePos;
                                                if (verboseDebug) {
                                                    std::cout << "Applied upward escape as last resort" << std::endl;
                                                }
                                            } else {
                                                // Can't move in any direction, stop completely at current position
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            // Neither stepping up nor sliding works
                            break;
                        }
                    }
                    else {
                        // Not near ground and can't slide on either axis
                        break;
                    }
                }
            }
            
            // Use the new position if it doesn't put us inside a block
            if (!isInsideBlock(nextPos, world)) {
                newPos = nextPos;
            } else {
                // Emergency case: we're about to enter a block, stop movement
                if (verboseDebug) {
                    std::cout << "WARNING: Movement would place player inside block, stopping" << std::endl;
                }
                break;
            }
        }
    }
    
    // Final position check
    if (isInsideBlock(newPos, world)) {
        // This shouldn't happen with proper collision detection, but just in case
        if (verboseDebug) {
            std::cout << "WARNING: Player inside block after movement. Attempting to adjust position." << std::endl;
        }
        
        // First try to correct vertical wall collisions
        correctVerticalWallCollision(newPos, velocity, world);
        
        // If still inside a block, try more general methods
        if (isInsideBlock(newPos, world)) {
            // Try to adjust position to be outside of blocks
            glm::vec3 adjustedPos = adjustPositionOutOfBlock(newPos, world);
            
            // Only use adjusted position if it's actually not inside a block
            if (!isInsideBlock(adjustedPos, world)) {
                newPos = adjustedPos;
                if (verboseDebug) {
                    std::cout << "Successfully adjusted position to avoid blocks" << std::endl;
                }
            } else {
                // If we can't fix it, revert to start position
                std::cout << "WARNING: Player inside block after movement. Reverting to start position." << std::endl;
                newPos = startPos;
            }
        } else if (verboseDebug) {
            std::cout << "Successfully corrected vertical wall collision" << std::endl;
        }
    }
    
    // Add redundant checks after movement
    if (newPos.y < 0.0f) {
        newPos.y = 0.01f; // Place slightly above 0
        isOnGround = true;
        if (verboseDebug) {
            std::cout << "Prevented falling below y=0 after movement" << std::endl;
        }
    }
    
    if (verboseDebug) {
        std::cout << "Final position after movement: (" << newPos.x << ", " << newPos.y << ", " << newPos.z << ")" << std::endl;
        std::cout << "Movement iterations: " << movementIterations << std::endl;
        std::cout << "Collisions encountered: " << collisionCount << std::endl;
        std::cout << "Is on ground: " << (isOnGround ? "YES" : "NO") << std::endl;
        std::cout << "===========================" << std::endl;
    }
    
    return newPos;
}

glm::vec3 CollisionSystem::getMinBounds(const glm::vec3& pos, const glm::vec3& forward) const {
    // Calculate the base collision box points relative to the player's position
    glm::vec3 baseMin = glm::vec3(-m_width/2, 0.0f, -m_width/2);
    glm::vec3 baseMax = glm::vec3(m_width/2, m_height, m_width/2);
    
    // Rotate the points based on the player's forward direction
    glm::vec3 rotatedMin = rotatePoint(baseMin, forward);
    glm::vec3 rotatedMax = rotatePoint(baseMax, forward);
    
    // Return the minimum point after rotation
    return pos + glm::vec3(
        std::min(rotatedMin.x, rotatedMax.x),
        std::min(rotatedMin.y, rotatedMax.y),
        std::min(rotatedMin.z, rotatedMax.z)
    );
}

glm::vec3 CollisionSystem::getMaxBounds(const glm::vec3& pos, const glm::vec3& forward) const {
    // Calculate the base collision box points relative to the player's position
    glm::vec3 baseMin = glm::vec3(-m_width/2, 0.0f, -m_width/2);
    glm::vec3 baseMax = glm::vec3(m_width/2, m_height, m_width/2);
    
    // Rotate the points based on the player's forward direction
    glm::vec3 rotatedMin = rotatePoint(baseMin, forward);
    glm::vec3 rotatedMax = rotatePoint(baseMax, forward);
    
    // Return the maximum point after rotation
    return pos + glm::vec3(
        std::max(rotatedMin.x, rotatedMax.x),
        std::max(rotatedMin.y, rotatedMax.y),
        std::max(rotatedMin.z, rotatedMax.z)
    );
}

void CollisionSystem::adjustPositionAtBlockBoundaries(glm::vec3& pos, bool verbose) {
    // Check if we're very close to block boundaries and adjust slightly
    float xFraction = pos.x - std::floor(pos.x);
    float zFraction = pos.z - std::floor(pos.z);
    
    // Check for positions exactly at chunk boundaries (multiples of 16)
    // Use more precise detection for chunk boundaries
    bool atChunkBoundaryX = std::abs(std::fmod(pos.x, World::CHUNK_SIZE)) < 0.015f || 
                           std::abs(std::fmod(pos.x, World::CHUNK_SIZE) - World::CHUNK_SIZE) < 0.015f;
    bool atChunkBoundaryZ = std::abs(std::fmod(pos.z, World::CHUNK_SIZE)) < 0.015f || 
                           std::abs(std::fmod(pos.z, World::CHUNK_SIZE) - World::CHUNK_SIZE) < 0.015f;
    bool atChunkBoundaryY = std::abs(std::fmod(pos.y, World::CHUNK_HEIGHT)) < 0.015f || 
                           std::abs(std::fmod(pos.y, World::CHUNK_HEIGHT) - World::CHUNK_HEIGHT) < 0.015f;
    
    glm::vec3 adjustment(0.0f);
    bool madeAdjustment = false;
    
    // Only adjust if we're at a chunk boundary
    if (atChunkBoundaryX || atChunkBoundaryZ || atChunkBoundaryY) {
        // Determine direction based on which side of the chunk boundary we're on
        if (atChunkBoundaryX) {
            float xChunkRemainder = std::fmod(pos.x, World::CHUNK_SIZE);
            // Push away from the boundary
            if (xChunkRemainder < 0.015f) {
                adjustment.x = 0.025f; // Small push inward from lower boundary
            } else if (xChunkRemainder > (World::CHUNK_SIZE - 0.015f)) {
                adjustment.x = -0.025f; // Small push inward from upper boundary
            }
            madeAdjustment = true;
        }
        
        if (atChunkBoundaryZ) {
            float zChunkRemainder = std::fmod(pos.z, World::CHUNK_SIZE);
            // Push away from the boundary
            if (zChunkRemainder < 0.015f) {
                adjustment.z = 0.025f; // Small push inward from lower boundary
            } else if (zChunkRemainder > (World::CHUNK_SIZE - 0.015f)) {
                adjustment.z = -0.025f; // Small push inward from upper boundary
            }
            madeAdjustment = true;
        }
        
        if (atChunkBoundaryY) {
            float yChunkRemainder = std::fmod(pos.y, World::CHUNK_HEIGHT);
            // Push away from the boundary
            if (yChunkRemainder < 0.015f) {
                adjustment.y = 0.025f; // Small push upward from lower boundary
            } else if (yChunkRemainder > (World::CHUNK_HEIGHT - 0.015f)) {
                adjustment.y = -0.025f; // Small push downward from upper boundary
            }
            madeAdjustment = true;
        }
    }
    // Only adjust for block boundaries if we're not at a chunk boundary
    // and if we're very close to a block boundary
    else if (xFraction < 0.015f || xFraction > 0.985f || 
            zFraction < 0.015f || zFraction > 0.985f) {
        // Use a much smaller adjustment for block boundaries
        if (xFraction < 0.015f) {
            adjustment.x = 0.02f;
            madeAdjustment = true;
        } else if (xFraction > 0.985f) {
            adjustment.x = -0.02f;
            madeAdjustment = true;
        }
        
        if (zFraction < 0.015f) {
            adjustment.z = 0.02f;
            madeAdjustment = true;
        } else if (zFraction > 0.985f) {
            adjustment.z = -0.02f;
            madeAdjustment = true;
        }
    }
    
    // Apply adjustment if needed
    if (madeAdjustment) {
        // Use a small-magnitude consistent adjustment to reduce jitter
        // Normalize adjustment if it's not zero
        if (glm::length(adjustment) > 0.0f) {
            adjustment = glm::normalize(adjustment) * 0.025f;
        }
        
        if (verbose) {
            std::cout << "Adjusted position away from ";
            if (atChunkBoundaryX || atChunkBoundaryZ || atChunkBoundaryY) {
                std::cout << "chunk boundary";
            } else {
                std::cout << "block boundary";
            }
            std::cout << " by (" << adjustment.x << ", " << adjustment.y << ", " << adjustment.z << ")" << std::endl;
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
    // This should match the rendering and world logic for solid blocks
    return blockType > 0;
}

bool CollisionSystem::intersectsWithBlock(const glm::vec3& min, const glm::vec3& max, 
                                        const glm::vec3& blockMin, const glm::vec3& blockMax) const {
    // Use a more precise AABB collision test with different expansion values
    // for horizontal and vertical axes to prevent getting stuck on walls
    const float horizontalEpsilon = 0.02f;
    const float verticalEpsilon = 0.01f;
    
    // Calculate which block face we're colliding with to handle vertical walls better
    // These deltas represent how much our collision box overlaps the block
    float deltaMinX = blockMax.x - min.x;  // Penetration from right face
    float deltaMaxX = max.x - blockMin.x;  // Penetration from left face
    float deltaMinZ = blockMax.z - min.z;  // Penetration from back face
    float deltaMaxZ = max.z - blockMin.z;  // Penetration from front face
    float deltaMinY = blockMax.y - min.y;  // Penetration from top face
    float deltaMaxY = max.y - blockMin.y;  // Penetration from bottom face
    
    // Determine if we're colliding with a vertical wall (side of block)
    // For a wall collision, the y-penetration is typically larger
    bool isVerticalWallCollision = 
        (std::min(deltaMinY, deltaMaxY) > std::min(deltaMinX, deltaMaxX) || 
         std::min(deltaMinY, deltaMaxY) > std::min(deltaMinZ, deltaMaxZ));
        
    // Use a larger epsilon for vertical walls to prevent sticking
    float adjustedHorizontalEpsilon = isVerticalWallCollision ? 0.045f : horizontalEpsilon; // Increased from 0.035f to 0.045f
    
    return (min.x <= blockMax.x + adjustedHorizontalEpsilon && max.x >= blockMin.x - adjustedHorizontalEpsilon &&
            min.y <= blockMax.y + verticalEpsilon && max.y >= blockMin.y - verticalEpsilon &&
            min.z <= blockMax.z + adjustedHorizontalEpsilon && max.z >= blockMin.z - adjustedHorizontalEpsilon);
}

bool CollisionSystem::isInsideBlock(const glm::vec3& pos, World* world) {
    if (!world) return false;
    
    // Use a scaled down version of our collision box for more precise inside checks
    CollisionBox checkBox = m_collisionBox.getSmallerBox(0.2f);
    
    // Check points around the player's body at different heights
    const float heights[] = { 0.1f, 0.9f, checkBox.height - 0.1f };
    
    // Check center at each height level
    for (const float height : heights) {
        glm::vec3 checkPos = pos + glm::vec3(0.0f, height, 0.0f);
        glm::ivec3 blockPos(
            std::floor(checkPos.x),
            std::floor(checkPos.y),
            std::floor(checkPos.z)
        );
        
        int blockType = world->getBlock(blockPos);
        if (isBlockSolid(blockType)) {
            return true;
        }
    }
    
    // Check horizontal points at mid-height
    const float midHeight = 0.9f;
    const float checkRadius = checkBox.width * 0.5f;
    
    // Check points around middle height (torso)
    const glm::vec3 horizontalOffsets[] = {
        glm::vec3(checkRadius, midHeight, 0.0f),     // Right
        glm::vec3(-checkRadius, midHeight, 0.0f),    // Left
        glm::vec3(0.0f, midHeight, checkRadius),     // Front
        glm::vec3(0.0f, midHeight, -checkRadius)     // Back
    };
    
    for (const auto& offset : horizontalOffsets) {
        glm::vec3 checkPos = pos + offset;
    glm::ivec3 blockPos(
            std::floor(checkPos.x),
            std::floor(checkPos.y),
            std::floor(checkPos.z)
        );
        
    int blockType = world->getBlock(blockPos);
        if (isBlockSolid(blockType)) {
            return true;
        }
    }
    
    return false;
}

glm::vec3 CollisionSystem::adjustPositionOutOfBlock(const glm::vec3& pos, World* world) {
    if (!world) return pos;
    
    glm::vec3 adjustedPos = pos;
    const float adjustAmount = 0.1f; // Increased from 0.05f for better wall escape
    
    // Try adjusting in various directions to find a non-colliding position
    // Directions to try: up (most likely to work), then cardinal directions
    std::vector<glm::vec3> directions = {
        glm::vec3(0, 1, 0),     // Up - prioritize this direction
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
    
    // Get block at player's position to identify the wall type
    glm::ivec3 blockPos(
        std::floor(pos.x),
        std::floor(pos.y + 0.5f), // Check block at torso level
        std::floor(pos.z)
    );
    
    // Try each direction with increasing distance
    for (float distance = adjustAmount; distance <= adjustAmount * 5; distance += adjustAmount) {
        for (const auto& dir : directions) {
            glm::vec3 testPos = pos + dir * distance;
            
            // Check if this position is free of blocks
            if (!isInsideBlock(testPos, world)) {
                // Do a secondary check to make sure we're not stuck in corners
                bool validPosition = true;
                
                // Check a few points around the player to ensure they're all free
                const float checkRadius = 0.2f;
                const int numChecks = 4;
                
                for (int i = 0; i < numChecks; i++) {
                    float angle = (float)i * (2.0f * M_PI / numChecks);
                    glm::vec3 checkPos = testPos + glm::vec3(
                        checkRadius * cos(angle),
                        0.0f,
                        checkRadius * sin(angle)
                    );
                    
                    if (isInsideBlock(checkPos, world)) {
                        validPosition = false;
                        break;
                    }
                }
                
                if (validPosition) {
                    // Make sure this position doesn't put us inside the ground
                    if (testPos.y < 0.1f) {
                        testPos.y = 0.1f; // Ensure minimal clearance from the ground
                    }
                    
                return testPos;
                }
            }
        }
    }
    
    // If all else fails, try moving up significantly 
    adjustedPos.y += 1.5f;
    
    // Still inside? Try finding a completely new position
    if (isInsideBlock(adjustedPos, world)) {
        // Final attempt - move to block center and up by player height + some margin
        adjustedPos.x = std::round(pos.x) + 0.5f;
        adjustedPos.z = std::round(pos.z) + 0.5f;
        
        // Find the ground level at this position by scanning from below
        adjustedPos.y = 0.0f;
        while (adjustedPos.y < 256.0f) {  // World height limit
            if (!isInsideBlock(adjustedPos, world)) {
                adjustedPos.y += 0.5f;  // Small clearance 
                return adjustedPos;
            }
            adjustedPos.y += 1.0f;
        }
    }
    
    return adjustedPos;
}

bool CollisionSystem::collidesWithBlocksGreedy(const glm::vec3& startPos, const glm::vec3& endPos, World* world, bool updateCoyoteTime)
{
    // ... existing code ...

    // Check each chunk for potential collisions
    const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>>& chunks = world->getChunks();
    
    // ... rest of the code ...
    
    return false; // Default return if no collision is found
}

bool CollisionSystem::intersectsWithAABB(const glm::vec3& min1, const glm::vec3& max1,
                                     const glm::vec3& min2, const glm::vec3& max2, float margin) const {
    // Use different epsilon values for horizontal and vertical axes
    const float horizontalEpsilon = 0.02f;
    const float verticalEpsilon = 0.01f;
    
    return (min1.x <= max2.x + horizontalEpsilon + margin && max1.x >= min2.x - horizontalEpsilon - margin &&
            min1.y <= max2.y + verticalEpsilon + margin && max1.y >= min2.y - verticalEpsilon - margin &&
            min1.z <= max2.z + horizontalEpsilon + margin && max1.z >= min2.z - horizontalEpsilon - margin);
}

glm::vec3 CollisionSystem::rotatePoint(const glm::vec3& point, const glm::vec3& forward) const {
    // Calculate right and up vectors from forward
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    
    // Create rotation matrix
    glm::mat3 rotationMatrix(
        right,
        up,
        forward
    );
    
    // Rotate the point
    return rotationMatrix * point;
}

bool CollisionSystem::pointInsideBlockWithMargin(const glm::vec3& point, const glm::vec3& blockMin, 
                                              const glm::vec3& blockMax, float margin) const {
    // Check if a point is inside a block with a specified margin
    // This is used for more precise edge collision detection
    return (point.x >= blockMin.x - margin && point.x <= blockMax.x + margin &&
            point.y >= blockMin.y - margin && point.y <= blockMax.y + margin &&
            point.z >= blockMin.z - margin && point.z <= blockMax.z + margin);
}

// New method for detecting and correcting wall collisions
void CollisionSystem::correctVerticalWallCollision(glm::vec3& position, const glm::vec3& velocity, World* world) {
    if (!world) return;
    
    // Only apply this correction when moving horizontally and not in flying mode
    if (std::abs(velocity.y) > 0.2f) return; // Skip if moving significantly vertically
    
    // Check if we're colliding with a wall
    if (!collidesWithBlocks(position, velocity, world, false)) return;
    
    // We're colliding, check if it's a vertical wall by checking nearby points
    // around player's body at multiple heights
    const float wallCheckDistance = 0.1f;
    
    // Check multiple height levels to better detect walls
    const float checkHeights[] = { 0.25f, 0.9f, 1.5f }; // Feet, torso, head
    
    // Check in all four horizontal directions
    const glm::vec3 checkDirs[] = {
        glm::vec3(1.0f, 0.0f, 0.0f),   // +X
        glm::vec3(-1.0f, 0.0f, 0.0f),  // -X
        glm::vec3(0.0f, 0.0f, 1.0f),   // +Z
        glm::vec3(0.0f, 0.0f, -1.0f)   // -Z
    };
    
    glm::vec3 wallNormal(0.0f);
    bool foundWall = false;
    
    // Track which directions have walls for edge detection
    bool wallsFound[4] = {false, false, false, false};
    
    // Check for walls at each height level
    for (float height : checkHeights) {
    for (int i = 0; i < 4; i++) {
            glm::vec3 checkPos = position + checkDirs[i] * wallCheckDistance + glm::vec3(0.0f, height, 0.0f);
        glm::ivec3 blockPos(
            std::floor(checkPos.x),
            std::floor(checkPos.y),
            std::floor(checkPos.z)
        );
        
        int blockType = world->getBlock(blockPos);
        if (isBlockSolid(blockType)) {
                // Add stronger weighting for walls at mid-heights (more important to player collision)
                float weight = (height == 0.9f) ? 1.5f : 1.0f;
                wallNormal -= checkDirs[i] * weight; // Accumulate normals pointing away from walls
            foundWall = true;
                wallsFound[i] = true;
            }
        }
    }
    
    // Also check for diagonal walls by checking corners at mid height
    const float cornerCheckHeight = 0.9f;
    const float cornerDistance = 0.12f;
    
    const glm::vec3 cornerDirs[] = {
        glm::vec3(1.0f, 0.0f, 1.0f),    // +X+Z
        glm::vec3(1.0f, 0.0f, -1.0f),   // +X-Z
        glm::vec3(-1.0f, 0.0f, 1.0f),   // -X+Z
        glm::vec3(-1.0f, 0.0f, -1.0f)   // -X-Z
    };
    
    // Track which corners have walls for edge detection
    bool cornerWallsFound[4] = {false, false, false, false};
    
    for (int i = 0; i < 4; i++) {
        const glm::vec3& dir = cornerDirs[i];
        glm::vec3 normalizedDir = glm::normalize(dir);
        glm::vec3 checkPos = position + normalizedDir * cornerDistance + glm::vec3(0.0f, cornerCheckHeight, 0.0f);
        
        glm::ivec3 blockPos(
            std::floor(checkPos.x),
            std::floor(checkPos.y),
            std::floor(checkPos.z)
        );
        
        int blockType = world->getBlock(blockPos);
        if (isBlockSolid(blockType)) {
            wallNormal -= normalizedDir; // Add to wall normal
            foundWall = true;
            cornerWallsFound[i] = true;
        }
    }
    
    // If we found a wall, try to nudge away
    if (foundWall && glm::length(wallNormal) > 0.001f) {
        wallNormal = glm::normalize(wallNormal);
        
        static int debugCounter = 0;
        bool verboseDebug = m_debugMode && (debugCounter++ % 120 == 0);
        
        if (verboseDebug) {
            std::cout << "Detected wall collision, wall normal: (" 
                      << wallNormal.x << ", " << wallNormal.y << ", " << wallNormal.z << ")" << std::endl;
        }
        
        // Check specifically for vertical edge collisions based on wall pattern
        bool isVerticalEdge = false;
        glm::vec3 edgeAvoidanceNormal(0.0f);
        
        // If we have walls in adjacent directions or specific corner patterns, it's likely a vertical edge
        if ((wallsFound[0] && wallsFound[2]) || (wallsFound[0] && wallsFound[3]) || 
            (wallsFound[1] && wallsFound[2]) || (wallsFound[1] && wallsFound[3])) {
            isVerticalEdge = true;
            
            // Calculate an optimal avoidance direction (diagonal away from the edge)
            if (wallsFound[0] && wallsFound[2]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));
            } else if (wallsFound[0] && wallsFound[3]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(-1.0f, 0.0f, 1.0f));
            } else if (wallsFound[1] && wallsFound[2]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f));
            } else if (wallsFound[1] && wallsFound[3]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
            }
        }
        // Also check corner patterns for edges
        else if (cornerWallsFound[0] || cornerWallsFound[1] || cornerWallsFound[2] || cornerWallsFound[3]) {
            isVerticalEdge = true;
            // Calculate direction to move away from the edge
            if (cornerWallsFound[0]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));
            } else if (cornerWallsFound[1]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(-1.0f, 0.0f, 1.0f));
            } else if (cornerWallsFound[2]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f));
            } else if (cornerWallsFound[3]) {
                edgeAvoidanceNormal = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
            }
        }
        
        // Handle vertical edge collisions with specialized pushing
        if (isVerticalEdge) {
            if (verboseDebug) {
                std::cout << "Vertical edge detected, trying specialized correction" << std::endl;
            }
            
            // Use stronger pushes for vertical edges and try different strategies
            const float edgePushDistances[] = {0.15f, 0.25f, 0.35f};
            
            // First try a pure horizontal push along the calculated optimal direction
            for (float pushDistance : edgePushDistances) {
                glm::vec3 correctedPos = position + edgeAvoidanceNormal * pushDistance;
                
                if (!collidesWithBlocks(correctedPos, velocity, world, false)) {
                    if (verboseDebug) {
                        std::cout << "Corrected vertical edge collision with horizontal push of " 
                                  << pushDistance << std::endl;
                    }
                    position = correctedPos;
                    return;
                }
            }
            
            // If horizontal push fails, try adding an upward component to slide up the edge
            for (float pushDistance : edgePushDistances) {
                // Add a stronger upward bias for edges
                glm::vec3 upwardEdgeNormal = glm::normalize(edgeAvoidanceNormal + glm::vec3(0.0f, 0.5f, 0.0f));
                glm::vec3 correctedPos = position + upwardEdgeNormal * pushDistance;
                
                if (!collidesWithBlocks(correctedPos, velocity, world, false)) {
                    if (verboseDebug) {
                        std::cout << "Corrected vertical edge collision with upward-biased push of " 
                                  << pushDistance << std::endl;
                    }
                    position = correctedPos;
                    return;
                }
            }
            
            // Last resort - try a purely vertical push with large value
            if (velocity.y <= 0) {  // Only when falling or not moving vertically
                for (float upwardPush = 0.15f; upwardPush <= 0.4f; upwardPush += 0.1f) {
                    glm::vec3 correctedPos = position + glm::vec3(0.0f, upwardPush, 0.0f);
                    
                    if (!collidesWithBlocks(correctedPos, velocity, world, false)) {
                        if (verboseDebug) {
                            std::cout << "Corrected vertical edge collision with pure vertical push of " 
                                      << upwardPush << std::endl;
                        }
                        position = correctedPos;
                        return;
                    }
                }
            }
        }
        
        // If not a vertical edge or if edge correction failed, use regular wall correction
        // Try pushing away from the wall with progressive strengths
        const float pushDistances[] = { 0.1f, 0.15f, 0.2f };
        
        for (float pushDistance : pushDistances) {
        glm::vec3 correctedPos = position + wallNormal * pushDistance;
        
        // Only use the correction if it resolves the collision
        if (!collidesWithBlocks(correctedPos, velocity, world, false)) {
                if (verboseDebug) {
                    std::cout << "Corrected wall collision with push distance " << pushDistance << std::endl;
                }
            position = correctedPos;
                return;
            }
        }
        
        // If horizontal pushes don't work, try adding a slight upward component
        // to help slide up small lips and edges
        for (float pushDistance : pushDistances) {
            // Calculate a slightly upward-biased push vector
            glm::vec3 upwardWallNormal = glm::normalize(wallNormal + glm::vec3(0.0f, 0.3f, 0.0f));
            glm::vec3 correctedPos = position + upwardWallNormal * pushDistance;
            
            if (!collidesWithBlocks(correctedPos, velocity, world, false)) {
                if (verboseDebug) {
                    std::cout << "Corrected wall collision with upward bias and push distance " 
                              << pushDistance << std::endl;
                }
                position = correctedPos;
                return;
            }
        }
        
        // Last resort - try a purely vertical nudge if sliding is involved
        if (velocity.y < 0) {
            for (float upwardPush = 0.1f; upwardPush <= 0.3f; upwardPush += 0.1f) {
                glm::vec3 correctedPos = position + glm::vec3(0.0f, upwardPush, 0.0f);
                
                if (!collidesWithBlocks(correctedPos, velocity, world, false)) {
                    if (verboseDebug) {
                        std::cout << "Corrected wall collision with purely vertical push of " 
                                  << upwardPush << std::endl;
                    }
                    position = correctedPos;
                    return;
                }
            }
        }
        
        if (verboseDebug) {
            std::cout << "Warning: Failed to correct wall collision" << std::endl;
        }
    }
}

glm::vec3 CollisionSystem::getCameraPosition(const glm::vec3& playerPos, float cameraHeight, const glm::vec3& forward, World* world) {
    if (!world) return playerPos + glm::vec3(0.0f, cameraHeight, 0.0f);
    
    // Start with default camera position (above player)
    glm::vec3 cameraPos = playerPos + glm::vec3(0.0f, cameraHeight, 0.0f);
    
    // Check if camera is inside a block
    if (!isInsideBlock(cameraPos, world)) {
        // Camera is already in a safe position
        return cameraPos;
    }
    
    // Camera is clipping, need to adjust it
    static int debugCounter = 0;
    bool verboseDebug = m_debugMode && (debugCounter++ % 120 == 0);
    
    if (verboseDebug) {
        std::cout << "Camera is clipping into a block at position (" 
                  << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
    }
    
    // First try to adjust camera horizontally toward the player's looking direction
    // This often works well for corners/edges that the player is looking toward
    glm::vec3 horizontalForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
    
    // Try several adjustments with increasing intensity
    const float adjustments[] = { 0.2f, 0.3f, 0.4f };
    for (float adjustment : adjustments) {
        // Move camera in the direction player is looking (helps when player looks at a wall)
        glm::vec3 adjustedPos = cameraPos + horizontalForward * adjustment;
        
        if (!isInsideBlock(adjustedPos, world)) {
            if (verboseDebug) {
                std::cout << "Adjusted camera forward by " << adjustment 
                          << " to avoid clipping" << std::endl;
            }
            return adjustedPos;
        }
        
        // If forward doesn't work, try backward (helps when backing into walls)
        adjustedPos = cameraPos - horizontalForward * adjustment;
        if (!isInsideBlock(adjustedPos, world)) {
            if (verboseDebug) {
                std::cout << "Adjusted camera backward by " << adjustment 
                          << " to avoid clipping" << std::endl;
            }
            return adjustedPos;
        }
    }
    
    // Try to find any direction that works by checking in 8 directions
    const float radialAdjustment = 0.35f;
    for (int i = 0; i < 8; i++) {
        float angle = static_cast<float>(i) * M_PI / 4.0f;
        glm::vec3 direction(cos(angle), 0.0f, sin(angle));
        glm::vec3 adjustedPos = cameraPos + direction * radialAdjustment;
        
        if (!isInsideBlock(adjustedPos, world)) {
            if (verboseDebug) {
                std::cout << "Adjusted camera radially in direction " << i 
                          << " to avoid clipping" << std::endl;
            }
            return adjustedPos;
        }
    }
    
    // If horizontal adjustments fail, try adjusting height
    for (float yAdjust = 0.1f; yAdjust <= 0.5f; yAdjust += 0.1f) {
        glm::vec3 adjustedPos = cameraPos + glm::vec3(0.0f, yAdjust, 0.0f);
        if (!isInsideBlock(adjustedPos, world)) {
            if (verboseDebug) {
                std::cout << "Adjusted camera upward by " << yAdjust 
                          << " to avoid clipping" << std::endl;
            }
            return adjustedPos;
        }
    }
    
    // If all else fails, return player position (last resort)
    if (verboseDebug) {
        std::cout << "WARNING: Failed to find safe camera position, reverting to player position" << std::endl;
    }
    return playerPos + glm::vec3(0.0f, cameraHeight, 0.0f);
} 