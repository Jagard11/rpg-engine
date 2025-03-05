// ./include/Utils/PlanetConfig.hpp
#ifndef PLANET_CONFIG_HPP
#define PLANET_CONFIG_HPP

/**
 * Constants for planet configuration
 * Central declaration to avoid duplication and conflicts
 */
namespace PlanetConfig {
    // Earth's actual radius in meters
    constexpr double SURFACE_RADIUS_METERS = 6371000.0;
    constexpr double TERRAIN_DEPTH_METERS = 5000.0;        // 5km below sea level
    constexpr double MAX_BUILD_HEIGHT_METERS = 15000.0;    // 15km above sea level
    
    // Voxel parameters
    constexpr double VOXEL_HEIGHT_METERS = 1.0;            // 1m height for all voxels
    constexpr double VOXEL_WIDTH_AT_SURFACE = 1.0;         // 1m width at sea level
    
    // Collision constants
    constexpr double COLLISION_OFFSET_METERS = 0.25;       // Offset for collision detection
    
    // Chunk size constant 
    constexpr int CHUNK_SIZE = 16;
}

#endif // PLANET_CONFIG_HPP