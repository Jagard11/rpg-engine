// ./include/Debug/DebugManager.hpp
#ifndef DEBUG_MANAGER_HPP
#define DEBUG_MANAGER_HPP

#include "Debug/Logger.hpp"

/**
 * Management class for debug settings and flags.
 * Acts as a bridge between the legacy debug approach and the new logging system.
 */
class DebugManager {
public:
    DebugManager();
    static DebugManager& getInstance();

    // General debug toggles
    bool showVoxelEdges() const;
    bool isCullingEnabled() const;
    bool useFaceColors() const;

    // Specific log category toggles
    bool logPlayerInfo() const;
    bool logRaycast() const;
    bool logChunkUpdates() const;
    bool logBlockPlacement() const;
    bool logCollision() const;
    bool logInventory() const;
    
    // Mesh debugging options
    bool debugVertexScaling() const;

    // Setters
    void setShowVoxelEdges(bool enabled);
    void setCullingEnabled(bool enabled);
    void setUseFaceColors(bool enabled);
    void setLogPlayerInfo(bool enabled);
    void setLogRaycast(bool enabled);
    void setLogChunkUpdates(bool enabled);
    void setLogBlockPlacement(bool enabled);
    void setLogCollision(bool enabled);
    void setLogInventory(bool enabled);
    void setDebugVertexScaling(bool enabled);
    
    // Configure global log level
    void setLogLevel(LogLevel level);
    
    // Initialize the logging system
    void initializeLogging();
    
    // Save and load settings
    void saveSettings(const std::string& filename = "debug_settings.json") const;
    bool loadSettings(const std::string& filename = "debug_settings.json");

private:
    bool showVoxelEdges_;
    bool enableCulling;
    bool useFaceColors_;
    bool logPlayerInfo_;
    bool logRaycast_;
    bool logChunkUpdates_;
    bool logBlockPlacement_;
    bool logCollision_;
    bool logInventory_;
    bool debugVertexScaling_; // Flag for controlling visual vertex scaling debugging
    
    // Helper to convert legacy categories to Logger categories
    LogCategory mapToLogCategory(bool* flag) const;
};

#endif