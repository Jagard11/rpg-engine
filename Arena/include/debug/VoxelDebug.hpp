#pragma once

#include <string>
#include <chrono>
#include <glm/glm.hpp>
#include "world/World.hpp"
#include "player/Player.hpp"

namespace Debug {

// Structure to track player position/movement at chunk boundaries
struct PlayerBoundaryEvent {
    int64_t timestamp;        // Microseconds since epoch
    glm::vec3 position;       // Player position
    glm::vec3 velocity;       // Player velocity
    bool isAtXBoundary;       // Is at X chunk boundary
    bool isAtZBoundary;       // Is at Z chunk boundary
    bool isAtYBoundary;       // Is at Y chunk boundary
};

/**
 * @brief Debug tools for voxel/chunk manipulation issues
 */
class VoxelDebug {
public:
    /**
     * @brief Initialize the debug system
     */
    static void initialize();
    
    /**
     * @brief Check if the debug system is initialized
     */
    static bool isInitialized();
    
    /**
     * @brief Called when F12 is pressed to dump debug information
     * @param world The current world instance
     * @param player The current player instance
     */
    static void dumpDebugInfo(World* world, Player* player);
    
    /**
     * @brief Generate a report of chunks around the player's current position
     * @param world The current world instance
     * @param player The current player instance
     * @param radius The horizontal radius in chunks to report around the player
     * @return Path to the generated report file
     */
    static std::string generateChunkReport(World* world, Player* player, int radius = 8);
    
    /**
     * @brief Record information about an attempted voxel manipulation
     * @param world The world instance
     * @param blockPos The position of the block being manipulated
     * @param success Whether the manipulation was successful
     * @param action Description of the action (add/remove)
     */
    static void recordVoxelOperation(World* world, const glm::ivec3& blockPos, bool success, const std::string& action);
    
    /**
     * @brief Record when a player gets stuck at a seam
     * @param player The player instance
     * @param position The position where the player is stuck
     */
    static void recordPlayerStuck(Player* player, const glm::vec3& position);
    
    /**
     * @brief Record a stack trace with the provided context message
     * @param contextMessage Context message describing the trace
     */
    static void recordStackTrace(const std::string& contextMessage);
    
    /**
     * @brief Start or stop tracking player movement at chunk boundaries
     * @param enable Whether to enable tracking
     */
    static void enableBoundaryTracking(bool enable);
    
    /**
     * @brief Check if boundary tracking is enabled
     */
    static bool isBoundaryTrackingEnabled();
    
    /**
     * @brief Record a player position at a chunk boundary
     * @param position The player's position
     * @param velocity The player's velocity
     * @param isAtXBoundary Whether player is at an X chunk boundary
     * @param isAtZBoundary Whether player is at a Z chunk boundary
     * @param isAtYBoundary Whether player is at a Y chunk boundary
     */
    static void recordBoundaryEvent(const glm::vec3& position, const glm::vec3& velocity, 
                                   bool isAtXBoundary, bool isAtZBoundary, bool isAtYBoundary);
    
    /**
     * @brief Save the recorded boundary events to a CSV file
     */
    static void saveChunkBoundaryEvents();

private:
    // Record keeping for debug operations
    static bool s_initialized;
    static std::chrono::steady_clock::time_point s_startTime;
    
    // Debug directory
    static constexpr const char* DEBUG_DIR = "/home/jagard/Downloads/GIT/rpg-engine/Arena/build/debug";
};

} // namespace Debug 