// ./include/Utils/SphereUtils.hpp
#ifndef SPHERE_UTILS_HPP
#define SPHERE_UTILS_HPP

// Include GLEW first if needed
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream> // For error reporting

// Constants for planet configuration
namespace PlanetConfig {
    // Surface radius offset from planet center (approximately 8km above radius)
    static const float SURFACE_RADIUS_OFFSET = 8.0f;
    
    // Terrain layer constants (consistent with previous implementation)
    static const float TERRAIN_DEPTH = 5.0f;        // Depth of DIRT below surface
    static const float MAX_BUILD_HEIGHT = 15.0f;    // Maximum height above surface for building
}

class SphereUtils {
public:
    /**
     * Get surface radius for a planet with the given radius
     * 
     * @param planetRadius The basic radius of the planet in voxel units
     * @return The surface radius (where blocks appear at surface level)
     */
    static float getSurfaceRadius(float planetRadius) {
        return planetRadius + PlanetConfig::SURFACE_RADIUS_OFFSET;
    }
    
    /**
     * Project a world point onto the sphere surface
     * This is the definitive implementation to ensure consistency across the codebase
     * 
     * @param worldPos The world position to project
     * @param surfaceR The surface radius of the planet
     * @param isInner Whether the point is below the surface (for correct projection direction)
     * @param faceType The type of face being projected (2=top, 3=bottom, others=side faces)
     * @return The projected point on the sphere
     */
    static glm::vec3 projectToSphere(const glm::vec3& worldPos, float surfaceR, bool isInner, int faceType) {
        // Safety checks for invalid input
        if (glm::length(worldPos) < 0.001f) {
            return glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        // Get the voxel's integer block position (floor for consistency with collision)
        glm::ivec3 blockPos = glm::ivec3(floor(worldPos.x), floor(worldPos.y), floor(worldPos.z));
        
        // Calculate block center in world space (x+0.5, y+0.5, z+0.5)
        glm::vec3 blockCenter = glm::vec3(blockPos) + glm::vec3(0.5f);
        
        // Get normalized direction from world origin to block center
        glm::vec3 blockDir = glm::normalize(blockCenter);
        
        // Calculate distance from center to block center - with double precision
        double bx = static_cast<double>(blockCenter.x);
        double by = static_cast<double>(blockCenter.y);
        double bz = static_cast<double>(blockCenter.z);
        double blockDistance = sqrt(bx*bx + by*by + bz*bz);
        
        // Calculate height layer using floor to ensure consistent layers
        int heightLayer = static_cast<int>(floor(blockDistance - surfaceR));
        
        // Base radius for this height layer (exact match to collision detection)
        float baseRadius = surfaceR + static_cast<float>(heightLayer);
        
        // Select appropriate radius based on face type
        float radius;
        
        // Face types:
        // 2 = top face (+Y), 3 = bottom face (-Y), others = side faces
        if (faceType == 2) { // Top face (+Y)
            // Exactly 1.0 unit above the base radius - matching collision
            radius = baseRadius + 1.0f;
        } else if (faceType == 3) { // Bottom face (-Y)
            // Exactly at the base radius - matching collision 
            radius = baseRadius;
        } else { // Side faces
            // For side faces, use exact local Y position (0.0 to 1.0)
            float localY = worldPos.y - blockPos.y;
            radius = baseRadius + localY;
        }
        
        // Project the vertex onto the sphere at the calculated radius
        // Using the block's center direction ensures perfect alignment
        return blockDir * radius;
    }
    
    /**
     * Check if a world position is within valid building/editing range
     * 
     * @param pos The world position to check
     * @param planetRadius The basic radius of the planet
     * @return True if the position is within valid editing range
     */
    static bool isWithinBuildRange(const glm::vec3& pos, float planetRadius) {
        // Calculate distance from center using double precision for accuracy
        double px = static_cast<double>(pos.x);
        double py = static_cast<double>(pos.y);
        double pz = static_cast<double>(pos.z);
        double distFromCenter = sqrt(px*px + py*py + pz*pz);
        
        float surfaceR = getSurfaceRadius(planetRadius);
        
        // Valid range: From below surface (crust) to MAX_BUILD_HEIGHT above sea level
        // TERRAIN_DEPTH units below surface is the bottom of the crust
        // MAX_BUILD_HEIGHT units above surface is the build limit
        return (distFromCenter >= planetRadius - PlanetConfig::TERRAIN_DEPTH) && 
               (distFromCenter <= surfaceR + PlanetConfig::MAX_BUILD_HEIGHT);
    }
    
    /**
     * Calculate the height above or below surface for a given world position
     * 
     * @param pos The world position
     * @param planetRadius The basic radius of the planet
     * @return The height relative to surface (positive = above, negative = below)
     */
    static float getHeightRelativeToSurface(const glm::vec3& pos, float planetRadius) {
        double px = static_cast<double>(pos.x);
        double py = static_cast<double>(pos.y);
        double pz = static_cast<double>(pos.z);
        double distFromCenter = sqrt(px*px + py*py + pz*pz);
        
        float surfaceR = getSurfaceRadius(planetRadius);
        return static_cast<float>(distFromCenter - surfaceR);
    }
    
    /**
     * Determine the block type based on distance from planet center
     * 
     * @param distFromCenter The distance from planet center
     * @param surfaceR The surface radius
     * @return The appropriate BlockType (defined in Types.hpp)
     */
    static int getBlockTypeForElevation(double distFromCenter, float surfaceR) {
        // This method returns int rather than BlockType to avoid circular dependencies
        // Caller should cast to BlockType
        // 0 = AIR, 1 = DIRT, 2 = GRASS
        
        if (distFromCenter < surfaceR - PlanetConfig::TERRAIN_DEPTH) {
            return 1; // DIRT for deep underground
        } else if (distFromCenter < surfaceR) {
            return 2; // GRASS for surface layer 
        } else {
            return 0; // AIR for above surface
        }
    }
};

#endif // SPHERE_UTILS_HPP