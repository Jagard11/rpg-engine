// ./include/Physics/CollisionSystem.hpp
#ifndef COLLISION_SYSTEM_HPP
#define COLLISION_SYSTEM_HPP

#include <glm/glm.hpp>
#include <vector>
#include "World/World.hpp"
#include "Utils/SphereUtils.hpp"

// Constants for collision detection
namespace CollisionConfig {
    static const float COLLISION_OFFSET = 0.25f;  // Increased from 0.15f to prevent sinking
    static const float GROUND_OFFSET = 0.3f;      // Increased from 0.2f to keep player higher
    static const float STEP_HEIGHT = 0.55f;       // Maximum height player can automatically step up
    static const float PLAYER_RADIUS = 0.4f;      // Slightly smaller than a full block
    static const float VERTICAL_OFFSET = 0.1f;    // Check slightly below/above to catch all collisions
}

/**
 * Centralized collision detection system for the game
 * Handles all collision queries between entities and the voxel world
 */
class CollisionSystem {
public:
    CollisionSystem(const World& world) : world(world) {}
    
    /**
     * Check if a position would collide with terrain or blocks
     * 
     * @param position Position to check
     * @param playerDir Direction the player is facing (for corner checks)
     * @return True if position collides with the environment
     */
    bool checkCollision(const glm::vec3& position, const glm::vec3& playerDir) const {
        // Get surface radius using standardized method
        float surfaceR = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
        
        // Calculate distance from center with double precision for accuracy
        double px = static_cast<double>(position.x);
        double py = static_cast<double>(position.y);
        double pz = static_cast<double>(position.z);
        double distFromCenter = sqrt(px*px + py*py + pz*pz);
        
        // If we're below the surface, we've collided - USING STANDARD COLLISION RADIUS
        if (distFromCenter < SphereUtils::getCollisionRadiusMeters()) {
            return true;
        }
        
        // Check for collisions with specific blocks within bounding box
        // Generate test points at player's body corners and edges
        std::vector<glm::vec3> testPoints;
        
        // Calculate up vector at player position (pointing away from planet center)
        glm::vec3 upDir = glm::normalize(position);
        
        // Calculate perpendicular axes for the bounding box
        glm::vec3 rightDir = glm::normalize(glm::cross(playerDir, upDir));
        glm::vec3 forwardDir = glm::normalize(glm::cross(upDir, rightDir));
        
        // Add test points at feet level
        testPoints.push_back(position + upDir * CollisionConfig::VERTICAL_OFFSET); // Center
        testPoints.push_back(position + rightDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET); // Right
        testPoints.push_back(position - rightDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET); // Left
        testPoints.push_back(position + forwardDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET); // Forward
        testPoints.push_back(position - forwardDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET); // Back
        
        // Add diagonal points
        testPoints.push_back(position + rightDir * CollisionConfig::PLAYER_RADIUS + forwardDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET);
        testPoints.push_back(position + rightDir * CollisionConfig::PLAYER_RADIUS - forwardDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET);
        testPoints.push_back(position - rightDir * CollisionConfig::PLAYER_RADIUS + forwardDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET);
        testPoints.push_back(position - rightDir * CollisionConfig::PLAYER_RADIUS - forwardDir * CollisionConfig::PLAYER_RADIUS + upDir * CollisionConfig::VERTICAL_OFFSET);
        
        // Check if any test point is inside a solid block
        for (const auto& testPoint : testPoints) {
            int blockX = static_cast<int>(floor(testPoint.x));
            int blockY = static_cast<int>(floor(testPoint.y));
            int blockZ = static_cast<int>(floor(testPoint.z));
            
            // Check if this position contains a non-air block
            Block block = world.getBlock(blockX, blockY, blockZ);
            if (block.type != BlockType::AIR) {
                return true;
            }
        }
        
        // No collision detected
        return false;
    }
    
    /**
     * Check if entity is on the ground
     * 
     * @param position Entity position
     * @param checkDistance How far to check for ground
     * @return True if the entity is on the ground
     */
    bool isOnGround(const glm::vec3& position, float checkDistance) const {
        // Calculate gravity direction (toward planet center)
        glm::vec3 gravityDir = -glm::normalize(position);
        
        // Calculate a point below current position to check for ground
        glm::vec3 testPos = position + gravityDir * checkDistance;
        
        // Check if there's a block below
        return checkCollision(testPos, glm::vec3(0.0f));
    }
    
    /**
     * Calculate a safe position on the surface for player respawn/teleport
     * 
     * @param direction Normalized direction from planet center
     * @param playerHeight Height of the player
     * @return Safe position on the surface
     */
    glm::vec3 findSafeSpawnPosition(const glm::vec3& direction, float playerHeight) const {
        float surfaceR = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
        float targetHeight = surfaceR + CollisionConfig::GROUND_OFFSET;
        
        // Start at exact surface position
        glm::vec3 candidatePos = direction * targetHeight;
        
        // If this position is already free from collisions, use it
        if (!checkCollision(candidatePos, glm::vec3(1.0f, 0.0f, 0.0f))) {
            return candidatePos;
        }
        
        // Otherwise, search upward for clear space
        for (float offset = 0.1f; offset <= 5.0f; offset += 0.1f) {
            candidatePos = direction * (targetHeight + offset);
            if (!checkCollision(candidatePos, glm::vec3(1.0f, 0.0f, 0.0f))) {
                return candidatePos;
            }
        }
        
        // If all else fails, return a position high above the surface
        return direction * (targetHeight + 10.0f);
    }

private:
    const World& world;
};

#endif // COLLISION_SYSTEM_HPP