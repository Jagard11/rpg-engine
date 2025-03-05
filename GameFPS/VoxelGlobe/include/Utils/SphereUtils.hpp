// ./include/Utils/SphereUtils.hpp
#ifndef SPHERE_UTILS_HPP
#define SPHERE_UTILS_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include "Utils/PlanetConfig.hpp"

// Forward declaration for Chunk
class Chunk;

/**
 * Utility class for standardized Earth-scale sphere calculations
 * Provides consistent methods for all sphere-related operations across the codebase
 */
class SphereUtils {
public:
    /**
     * Get surface radius (sea level) in meters
     * @return The exact surface radius in meters
     */
    static double getSurfaceRadiusMeters() {
        return PlanetConfig::SURFACE_RADIUS_METERS;
    }
    
    /**
     * Get collision radius (surface plus offset) in meters
     * @return The collision radius in meters
     */
    static double getCollisionRadiusMeters() {
        return getSurfaceRadiusMeters() + PlanetConfig::COLLISION_OFFSET_METERS;
    }
    
    /**
     * Get the surface radius for a specific world
     * Provided for backward compatibility with code using World references
     * @param worldRadius The planet radius in meters
     * @return The surface radius in meters
     */
    static float getSurfaceRadius(float worldRadius) {
        return static_cast<float>(getSurfaceRadiusMeters());
    }
    
    /**
     * Calculate the width of a voxel at a specific distance from center
     * Voxels taper toward the core, width decreases with depth
     * 
     * @param distanceFromCenter The distance from planet center in meters
     * @return The voxel width at that distance in meters
     */
    static double getVoxelWidthAt(double distanceFromCenter) {
        // Calculate how much the voxel should taper based on distance from surface
        double surfaceRadius = getSurfaceRadiusMeters();
        
        // Safety check for negative or extremely small distances
        if (distanceFromCenter < 0.1) {
            distanceFromCenter = 0.1;
        }
        
        // At sea level, width is 1m x 1m
        // Below sea level, width decreases proportionally to distance from center
        double scaleRatio = distanceFromCenter / surfaceRadius;
        return PlanetConfig::VOXEL_WIDTH_AT_SURFACE * scaleRatio;
    }
    
    /**
     * Check if a world position is within valid building/editing range
     * 
     * @param pos The world position to check in meters
     * @return True if the position is within valid editing range
     */
    static bool isWithinBuildRange(const glm::dvec3& pos) {
        // Calculate distance from center using double precision for accuracy
        double distFromCenter = glm::length(pos);
        double surfaceR = getSurfaceRadiusMeters();
        
        // Valid range: From 5km below sea level to 15km above sea level
        return (distFromCenter >= surfaceR - PlanetConfig::TERRAIN_DEPTH_METERS) && 
               (distFromCenter <= surfaceR + PlanetConfig::MAX_BUILD_HEIGHT_METERS);
    }
    
    /**
     * Calculate the height above or below surface (sea level) for a given world position
     * 
     * @param pos The world position in meters
     * @return The height relative to surface (positive = above, negative = below) in meters
     */
    static double getHeightRelativeToSurface(const glm::dvec3& pos) {
        double distFromCenter = glm::length(pos);
        double surfaceR = getSurfaceRadiusMeters();
        return distFromCenter - surfaceR;
    }
    
    /**
     * Determine the block type based on distance from planet center
     * 
     * @param distFromCenter The distance from planet center in meters
     * @return The appropriate BlockType (defined in Types.hpp)
     */
    static int getBlockTypeForElevation(double distFromCenter) {
        double surfaceR = getSurfaceRadiusMeters();
        
        // 0 = AIR, 1 = DIRT, 2 = GRASS
        if (distFromCenter < surfaceR - PlanetConfig::TERRAIN_DEPTH_METERS) {
            return 1; // DIRT for deep underground (fallback for out-of-range)
        } else if (distFromCenter < surfaceR) {
            return 1; // DIRT for most underground
        } else if (distFromCenter < surfaceR + 1.0) {
            return 2; // GRASS for surface layer (top 1 meter)
        } else {
            return 0; // AIR for above surface
        }
    }
    
    /**
     * Convert a global coordinate to a chunk-relative coordinate
     * Helps reduce precision issues with Earth-scale rendering
     * 
     * @param worldCoord The global coordinate
     * @param chunkCoord The chunk coordinate the point belongs to
     * @param chunkSize The size of a chunk
     * @return The coordinate relative to the chunk origin
     */
    static double worldToChunkRelative(double worldCoord, int chunkCoord, int chunkSize) {
        return worldCoord - (chunkCoord * chunkSize);
    }
    
    /**
     * Rebases a world position relative to a local origin to reduce floating-point errors
     * 
     * @param worldPos The absolute world position
     * @param originChunkX The X-coordinate of the origin chunk
     * @param originChunkY The Y-coordinate of the origin chunk
     * @param originChunkZ The Z-coordinate of the origin chunk
     * @param chunkSize The size of a chunk
     * @return The rebased position relative to the origin
     */
    static glm::vec3 rebasePosition(const glm::dvec3& worldPos, int originChunkX, int originChunkY, int originChunkZ, int chunkSize) {
        double originX = originChunkX * chunkSize;
        double originY = originChunkY * chunkSize;
        double originZ = originChunkZ * chunkSize;
        
        return glm::vec3(
            static_cast<float>(worldPos.x - originX),
            static_cast<float>(worldPos.y - originY),
            static_cast<float>(worldPos.z - originZ)
        );
    }
    
    /**
     * Calculate a scale factor for a position that helps maintain proper sphere projection
     * Used to avoid extreme coordinate values in rendering and raycasting
     * 
     * @param position The position to calculate scale for
     * @param referenceRadius The reference radius (usually planet surface)
     * @return A scale factor that can be applied to operations
     */
    static float calculateScaleFactor(const glm::vec3& position, float referenceRadius) {
        float distFromCenter = glm::length(position);
        // Return a scale factor that prevents extreme values
        // When very close to center, use minimum scale to avoid division by zero
        return distFromCenter < 1.0f ? 1.0f : std::min(1.0f, referenceRadius / distFromCenter);
    }
    
    /**
     * Transform the world position to a rendering-friendly local position
     * This method rebases coordinates and scales them to prevent precision issues
     * 
     * @param worldPos Original world position (in Earth coordinates)
     * @param localOrigin Local origin to rebase around (usually player chunk)
     * @param chunkSize Size of chunks for rebasing calculation
     * @return A rendering-friendly position suitable for graphics operations
     */
    static glm::vec3 worldToRenderingSpace(const glm::dvec3& worldPos, const glm::ivec3& localOrigin, int chunkSize) {
        // First rebase to get a local-space position relative to origin
        glm::vec3 localPos = rebasePosition(worldPos, localOrigin.x, localOrigin.y, localOrigin.z, chunkSize);
        
        // For extremely long distances, apply scaling to prevent precision issues
        float dist = glm::length(localPos);
        if (dist > 10000.0f) {
            // Scale down proportionally to stay within reasonable rendering space
            float scale = 10000.0f / dist;
            return localPos * scale;
        }
        
        return localPos;
    }
};

#endif // SPHERE_UTILS_HPP