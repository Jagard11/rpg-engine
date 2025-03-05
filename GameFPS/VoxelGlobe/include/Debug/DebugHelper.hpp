// ./include/Debug/DebugHelper.hpp
#ifndef DEBUG_HELPER_HPP
#define DEBUG_HELPER_HPP

#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include "World/World.hpp"

// Global configuration for the game
namespace GlobalConfig {
    // Debug flags
    static const bool ENABLE_DETAILED_DEBUG = true;
    static const bool USE_ADVANCED_PROJECTION = true;
    
    // Surface configuration
    static const float SURFACE_RADIUS_OFFSET = 8.0f;  // Distance from planet center to surface
    static const float GROUND_OFFSET = 0.2f;          // How high above surface the player should be
    static const float COLLISION_OFFSET = 0.15f;      // Collision detection offset
    
    // Collision system
    static const bool FORCE_SYNC_UPDATES = true;      // Force synchronous updates after block changes
}

// Helper functions for debugging
class DebugHelper {
public:
    // Get the surface radius consistently
    static float getSurfaceRadius(const World& world) {
        return world.getRadius() + GlobalConfig::SURFACE_RADIUS_OFFSET;
    }
    
    // Log coordinate information
    static void logCoords(const std::string& prefix, const glm::vec3& position, const World& world) {
        if (!GlobalConfig::ENABLE_DETAILED_DEBUG) return;
        
        float surfaceR = getSurfaceRadius(world);
        float distFromCenter = glm::length(position);
        float heightAboveSurface = distFromCenter - surfaceR;
        
        std::cout << prefix << " position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << prefix << " distance from center: " << distFromCenter 
                  << ", height above surface: " << heightAboveSurface << std::endl;
        
        // Get chunk coordinates
        int chunkX = static_cast<int>(floor(position.x / static_cast<float>(Chunk::SIZE)));
        int chunkY = static_cast<int>(floor(position.y / static_cast<float>(Chunk::SIZE)));
        int chunkZ = static_cast<int>(floor(position.z / static_cast<float>(Chunk::SIZE)));
        
        std::cout << prefix << " chunk coords: (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        
        // Get local origin
        const glm::ivec3& origin = world.getLocalOrigin();
        std::cout << "World local origin: (" << origin.x << ", " << origin.y << ", " << origin.z << ")" << std::endl;
    }
    
    // Log block information
    static void logBlock(const std::string& prefix, int x, int y, int z, BlockType type, const World& world) {
        if (!GlobalConfig::ENABLE_DETAILED_DEBUG) return;
        
        // Calculate chunk coordinates
        int chunkX = static_cast<int>(floor(x / static_cast<float>(Chunk::SIZE)));
        int chunkY = static_cast<int>(floor(y / static_cast<float>(Chunk::SIZE)));
        int chunkZ = static_cast<int>(floor(z / static_cast<float>(Chunk::SIZE)));
        
        // Calculate local block position
        int localX = x - chunkX * Chunk::SIZE;
        int localY = y - chunkY * Chunk::SIZE;
        int localZ = z - chunkZ * Chunk::SIZE;
        
        std::cout << prefix << " block at world (" << x << ", " << y << ", " << z 
                  << ") -> chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                  << ") local (" << localX << ", " << localY << ", " << localZ 
                  << ") type: " << static_cast<int>(type) << std::endl;
                  
        // Calculate distance from center
        float distFromCenter = glm::length(glm::vec3(x, y, z));
        float surfaceR = getSurfaceRadius(world);
        
        std::cout << prefix << " block distance from center: " << distFromCenter 
                  << ", height vs surface: " << (distFromCenter - surfaceR) << std::endl;
    }
    
    // Log collision information
    static void logCollision(const std::string& prefix, const glm::vec3& position, bool collided, const World& world) {
        if (!GlobalConfig::ENABLE_DETAILED_DEBUG) return;
        
        float surfaceR = getSurfaceRadius(world);
        float distFromCenter = glm::length(position);
        
        std::cout << prefix << " collision check at " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Distance from center: " << distFromCenter 
                  << ", surface at: " << surfaceR 
                  << ", result: " << (collided ? "COLLISION" : "NO COLLISION") << std::endl;
    }
};

#endif