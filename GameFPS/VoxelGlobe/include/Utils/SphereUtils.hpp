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

// Include needed headers
#include <cstring> // For memcpy

// Simple utility to generate Perlin-like noise
class TerrainNoiseGenerator {
private:
    // Permutation table for noise generation
    static inline const int perm[512] = {
        151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
        140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
        247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
        57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
        74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
        60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
        65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
        200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
        52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
        207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
        119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
        129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
        218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
        81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
        184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
        222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
        // Repeat the table to avoid overflow calculations
        151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
        140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
        247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
        57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
        74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
        60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
        65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
        200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
        52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
        207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
        119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
        129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
        218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
        81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
        184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
        222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
    };

public:
    // Hash function for noise
    static int hash(int x, int y) {
        return perm[(perm[x & 255] + y) & 255];
    }
    
    // Fade function for smooth interpolation (Ken Perlin's improved version)
    static double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    // Linear interpolation
    static double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }
    
    // Gradient function (simplified 2D gradient)
    static double grad(int hash, double x, double y) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }
    
    // Generate 2D noise in range [0, 1]
    static double noise(double x, double y) {
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;
        
        x -= floor(x);
        y -= floor(y);
        
        double u = fade(x);
        double v = fade(y);
        
        int A = hash(X, Y);
        int B = hash(X + 1, Y);
        int C = hash(X, Y + 1);
        int D = hash(X + 1, Y + 1);
        
        double result = lerp(v, 
                            lerp(u, grad(A, x, y), grad(B, x-1, y)),
                            lerp(u, grad(C, x, y-1), grad(D, x-1, y-1)));
        
        // Scale from [-1, 1] to [0, 1]
        return (result + 1.0) / 2.0;
    }
    
    // Generate multi-octave noise for more natural terrain
    static double fractalNoise(double x, double y, int octaves = 6, double persistence = 0.5) {
        double total = 0;
        double frequency = 1;
        double amplitude = 1;
        double maxValue = 0;
        
        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2;
        }
        
        return total / maxValue;
    }
};

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
     * Calculate height variation for a given position
     * Uses a dramatic sine wave pattern to test full height range
     * 
     * @param position The position on the planet surface
     * @return Height offset in meters
     */
    static double getHeightVariation(const glm::dvec3& position) {
        // Verify we have a valid position
        if (glm::length(position) < 0.001) {
            return 0.0; // Safety: Return zero height for positions near origin
        }
        
        // Normalize position to get direction from center
        glm::dvec3 dir = glm::normalize(position);
        
        // Create a simple, large wavelength pattern
        // Using dramatic sine waves that vary over several chunks
        double wave1 = sin(dir.x * 0.5) * 7000.0; // 7km amplitude wave based on x
        double wave2 = cos(dir.z * 0.5) * 7000.0; // 7km amplitude wave based on z
        
        // Combine waves to create dramatic terrain pattern
        double combinedHeight = (wave1 + wave2) * 0.5;
        
        // Safety clamp to valid range (with slight margin)
        return std::max(-PlanetConfig::TERRAIN_DEPTH_METERS * 0.95, 
                  std::min(combinedHeight, PlanetConfig::MAX_BUILD_HEIGHT_METERS * 0.95));
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
     * Determine the block type based on distance from planet center and position
     * Takes into account height variation
     * 
     * @param distFromCenter The distance from planet center in meters
     * @param position The world position for height variation
     * @return The appropriate BlockType (defined in Types.hpp)
     */
    static int getBlockTypeForElevation(double distFromCenter, const glm::dvec3& position) {
        try {
            double surfaceR = getSurfaceRadiusMeters();
            
            // Safety check for invalid positions
            if (glm::length(position) < 0.001) {
                // Fall back to standard elevation-based terrain
                return getBlockTypeForElevation(distFromCenter);
            }
            
            // Get height variation for this position - dramatic sine waves
            double heightVariation = getHeightVariation(position);
            
            // Adjust surface radius with height variation
            double adjustedSurfaceR = surfaceR + heightVariation;
            
            // Debug output for extreme heights
            if (std::abs(heightVariation) > PlanetConfig::TERRAIN_DEPTH_METERS * 0.8) {
                static int debugCounter = 0;
                if (debugCounter++ % 1000 == 0) {  // Don't spam logs
                    std::cout << "Significant height variation detected: " << heightVariation << "m at position " 
                              << position.x << ", " << position.y << ", " << position.z 
                              << " (dist from center: " << distFromCenter << "m)" << std::endl;
                }
            }
            
            // 0 = AIR, 1 = DIRT, 2 = GRASS
            if (distFromCenter < adjustedSurfaceR - PlanetConfig::TERRAIN_DEPTH_METERS) {
                return 1; // DIRT for deep underground (fallback for out-of-range)
            } else if (distFromCenter < adjustedSurfaceR) {
                return 1; // DIRT for most underground
            } else if (distFromCenter < adjustedSurfaceR + 1.0) {
                return 2; // GRASS for surface layer (top 1 meter)
            } else {
                return 0; // AIR for above surface
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in getBlockTypeForElevation: " << e.what() << std::endl;
            return getBlockTypeForElevation(distFromCenter);
        } catch (...) {
            // If anything goes wrong, fall back to standard method
            std::cerr << "Unknown exception in getBlockTypeForElevation" << std::endl;
            return getBlockTypeForElevation(distFromCenter);
        }
    }
    
    /**
     * Original method for backward compatibility
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