#ifndef WORLD_HPP
#define WORLD_HPP

#include "World/Chunk.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include <utility>
#include <memory>

// Custom hash function for tuple keys (chunkX, chunkY, chunkZ, mergeFactor)
struct quad_hash {
    std::size_t operator()(const std::tuple<int, int, int, int>& t) const {
        return std::hash<int>{}(std::get<0>(t)) ^ 
               (std::hash<int>{}(std::get<1>(t)) << 1) ^ 
               (std::hash<int>{}(std::get<2>(t)) << 2) ^ 
               (std::hash<int>{}(std::get<3>(t)) << 3);
    }
};

class World {
public:
    // Reduced radius to avoid floating point precision issues
    // 6371.0f is closer to Earth's radius in kilometers
    World() : radius(6371.0f), localOrigin(0, 0, 0) {}
    
    // Update chunks around the player
    void update(const glm::vec3& playerPos);
    
    // Access the chunk map (mutable)
    std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash>& getChunks();
    
    // Access the chunk map (const)
    const std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash>& getChunks() const;
    
    // Convert cube coordinates to sphere coordinates
    glm::vec3 cubeToSphere(int face, int x, int z, float y) const;
    
    // Find the height of the surface at a given position
    float findSurfaceHeight(float chunkX, float chunkZ) const;
    
    // Set a block in the world
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);
    
    // Get a block from the world
    Block getBlock(int worldX, int worldY, int worldZ) const;
    
    // Get the planet radius
    float getRadius() const { return radius; }
    
    // Get the surface radius (planet radius + offset)
    float getSurfaceRadius() const;
    
    // Get the local origin (used for chunk coordinate system)
    glm::ivec3 getLocalOrigin() const { return localOrigin; }

private:
    // Map of all loaded chunks, keyed by (chunkX, chunkY, chunkZ, mergeFactor)
    std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash> chunks;
    
    // Radius of the planet in voxel units
    float radius;
    
    // Local origin for chunk coordinates (typically centered on player)
    glm::ivec3 localOrigin;
    
    // Frame counter for logging
    int frameCounter = 0;
};

#endif // WORLD_HPP