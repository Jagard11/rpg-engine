#ifndef WORLD_HPP
#define WORLD_HPP

#include "World/Chunk.hpp"
#include "Utils/CoordinateSystem.hpp"
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

/**
 * Represents the entire voxel world as a spherical planet
 * Manages chunk loading/unloading and handles Earth-scale coordinates
 */
class World {
public:
    // Planet radius in kilometers, converted to meters (6,371,000 meters = Earth's radius)
    static constexpr double EARTH_RADIUS_METERS = 6371000.0;
    
    // Constructor initializes with Earth's radius
    World();
    
    // Update chunks around the player
    void update(const glm::vec3& playerPos);
    
    // Access the chunk map (mutable)
    std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash>& getChunks();
    
    // Access the chunk map (const)
    const std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash>& getChunks() const;
    
    // Set a block in the world
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);
    
    // Get a block from the world
    Block getBlock(int worldX, int worldY, int worldZ) const;
    
    // Get the planet radius
    double getRadius() const { return radius; }
    
    // Get the surface radius (planet radius + offset)
    double getSurfaceRadius() const;
    
    // Get the local origin (used for origin rebasing)
    glm::ivec3 getLocalOrigin() const { return localOrigin; }
    
    // Get the voxel size at a given distance from center
    double getVoxelWidthAt(double distanceFromCenter) const;
    
    // Transform a world position to relative-to-player coordinates (reduces floating point errors)
    glm::dvec3 worldToLocalSpace(const glm::dvec3& worldPos) const;
    
    // Transform a local position back to world coordinates
    glm::dvec3 localToWorldSpace(const glm::dvec3& localPos) const;
    
    // Get the coordinate system
    const CoordinateSystem& getCoordinateSystem() const { return coordSystem; }

private:
    // Map of all loaded chunks, keyed by (chunkX, chunkY, chunkZ, mergeFactor)
    std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash> chunks;
    
    // Radius of the planet in meters
    double radius;
    
    // Local origin for relative coordinates (typically centered on player)
    glm::ivec3 localOrigin;
    
    // Coordinate system for handling Earth-scale coordinates
    CoordinateSystem coordSystem;
    
    // Frame counter for logging
    int frameCounter;
};

#endif // WORLD_HPP