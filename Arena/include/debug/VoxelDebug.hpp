#pragma once

#include <string>
#include <chrono>
#include "world/World.hpp"
#include "player/Player.hpp"

namespace Debug {

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
     * @brief Called when F12 is pressed to dump debug information
     * @param world The current world instance
     * @param player The current player instance
     */
    static void dumpDebugInfo(World* world, Player* player);
    
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
     * @param context Context message describing the trace
     */
    static void recordStackTrace(const std::string& context);

private:
    // Record keeping for debug operations
    static bool s_initialized;
    static std::chrono::steady_clock::time_point s_startTime;
    
    // Debug directory
    static constexpr const char* DEBUG_DIR = "/home/jagard/Downloads/GIT/rpg-engine/Arena/build/debug";
};

} // namespace Debug 