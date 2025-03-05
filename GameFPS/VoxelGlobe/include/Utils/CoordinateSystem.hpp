// ./include/Utils/CoordinateSystem.hpp
#ifndef COORDINATE_SYSTEM_HPP
#define COORDINATE_SYSTEM_HPP

#include <glm/glm.hpp>
#include <tuple>
#include "Utils/SphereUtils.hpp"

/**
 * Contains utilities for handling Earth-scale coordinates with high precision.
 * Provides conversion between absolute world coordinates (double precision) and
 * local rendering coordinates (float precision) that are rebased around the player.
 */
class CoordinateSystem {
public:
    /**
     * Initialize the coordinate system with a planet radius.
     * @param planetRadius The radius of the planet in meters (Earth ~= 6371000)
     */
    CoordinateSystem(double planetRadius) 
        : planetRadius(planetRadius), 
          originChunkX(0), originChunkY(0), originChunkZ(0) {}
    
    /**
     * Set the origin chunk for local coordinate calculations.
     * Should be updated to the player's current chunk.
     */
    void setOriginChunk(int chunkX, int chunkY, int chunkZ) {
        originChunkX = chunkX;
        originChunkY = chunkY;
        originChunkZ = chunkZ;
    }
    
    /**
     * Convert a global double-precision world position to a local float-precision position
     * relative to the current origin chunk. This ensures rendered coordinates stay small.
     */
    glm::vec3 worldToLocal(const glm::dvec3& worldPos, int chunkSize) const {
        // Calculate the offset from the origin chunk in world space
        double originX = originChunkX * chunkSize;
        double originY = originChunkY * chunkSize;
        double originZ = originChunkZ * chunkSize;
        
        // Subtract the origin to get local coordinates
        return glm::vec3(
            static_cast<float>(worldPos.x - originX),
            static_cast<float>(worldPos.y - originY),
            static_cast<float>(worldPos.z - originZ)
        );
    }
    
    /**
     * Convert a local float-precision position back to a global double-precision world position.
     */
    glm::dvec3 localToWorld(const glm::vec3& localPos, int chunkSize) const {
        // Calculate the offset from the origin chunk in world space
        double originX = originChunkX * chunkSize;
        double originY = originChunkY * chunkSize;
        double originZ = originChunkZ * chunkSize;
        
        // Add the origin to get world coordinates
        return glm::dvec3(
            static_cast<double>(localPos.x) + originX,
            static_cast<double>(localPos.y) + originY,
            static_cast<double>(localPos.z) + originZ
        );
    }
    
    /**
     * Calculate if a position is within the valid building range
     * (from 5km below surface to 15km above surface).
     */
    bool isWithinBuildRange(const glm::dvec3& worldPos) const {
        return SphereUtils::isWithinBuildRange(worldPos);
    }
    
    /**
     * Create a normalized direction vector from planet center to position.
     * Returns high-precision vector for accurate direction calculations.
     */
    glm::dvec3 directionFromCenter(const glm::dvec3& worldPos) const {
        return glm::normalize(worldPos);
    }
    
    /**
     * Calculate the tapering factor for a block at the given distance from center.
     * This determines how much the block should taper toward the center
     * to maintain the spherical shape.
     */
    double calculateTaperingFactor(double distanceFromCenter) const {
        // Calculate the voxel width at this distance
        double voxelWidth = SphereUtils::getVoxelWidthAt(distanceFromCenter);
        
        // Calculate the voxel width one meter closer to the center
        double voxelWidthBelow = SphereUtils::getVoxelWidthAt(distanceFromCenter - 1.0);
        
        // Return the ratio (how much smaller the bottom face should be)
        return voxelWidthBelow / voxelWidth;
    }
    
    /**
     * Get the voxel width at a given distance from center.
     * Width increases linearly with distance from center.
     */
    double getVoxelWidthAt(double distanceFromCenter) const {
        return SphereUtils::getVoxelWidthAt(distanceFromCenter);
    }
    
    /**
     * Get the planet radius.
     */
    double getPlanetRadius() const {
        return planetRadius;
    }
    
    /**
     * Calculate the vertices of a frustum block at the given position.
     * @return Array of 8 vertices defining the frustum
     */
    std::array<glm::dvec3, 8> calculateFrustumVertices(const glm::dvec3& blockPos) const {
        std::array<glm::dvec3, 8> vertices;
        
        // Calculate block center
        glm::dvec3 blockCenter = blockPos + glm::dvec3(0.5, 0.5, 0.5);
        
        // Determine distance from center and tapering factor
        double distFromCenter = glm::length(blockCenter);
        double taperingFactor = calculateTaperingFactor(distFromCenter);
        
        // Get direction from center to block
        glm::dvec3 dirToBlock = glm::normalize(blockCenter);
        
        // Calculate orthogonal axes for the block faces
        glm::dvec3 up = dirToBlock;
        glm::dvec3 right = glm::normalize(glm::cross(
            glm::abs(up.y) > 0.99 ? glm::dvec3(1, 0, 0) : glm::dvec3(0, 1, 0), 
            up
        ));
        glm::dvec3 forward = glm::normalize(glm::cross(right, up));
        
        // Size of top face (at current distance) is 1.0
        double topSize = 1.0;
        
        // Size of bottom face is smaller based on tapering
        double bottomSize = topSize * taperingFactor;
        
        // Half-sizes for convenience
        double topHalfSize = topSize * 0.5;
        double bottomHalfSize = bottomSize * 0.5;
        
        // Calculate vertices for top face (further from center)
        vertices[0] = blockCenter + (up * 0.5) - (right * topHalfSize) - (forward * topHalfSize);
        vertices[1] = blockCenter + (up * 0.5) + (right * topHalfSize) - (forward * topHalfSize);
        vertices[2] = blockCenter + (up * 0.5) + (right * topHalfSize) + (forward * topHalfSize);
        vertices[3] = blockCenter + (up * 0.5) - (right * topHalfSize) + (forward * topHalfSize);
        
        // Calculate vertices for bottom face (closer to center)
        vertices[4] = blockCenter - (up * 0.5) - (right * bottomHalfSize) - (forward * bottomHalfSize);
        vertices[5] = blockCenter - (up * 0.5) + (right * bottomHalfSize) - (forward * bottomHalfSize);
        vertices[6] = blockCenter - (up * 0.5) + (right * bottomHalfSize) + (forward * bottomHalfSize);
        vertices[7] = blockCenter - (up * 0.5) - (right * bottomHalfSize) + (forward * bottomHalfSize);
        
        return vertices;
    }
    
    /**
     * Get the current origin chunk coordinates.
     */
    glm::ivec3 getOriginChunk() const {
        return glm::ivec3(originChunkX, originChunkY, originChunkZ);
    }
    
private:
    double planetRadius;
    int originChunkX;
    int originChunkY;
    int originChunkZ;
};

#endif // COORDINATE_SYSTEM_HPP