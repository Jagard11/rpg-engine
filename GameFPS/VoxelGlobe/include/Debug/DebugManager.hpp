// ./include/Debug/DebugManager.hpp
#ifndef DEBUG_MANAGER_HPP
#define DEBUG_MANAGER_HPP

class DebugManager {
public:
    DebugManager();
    static DebugManager& getInstance();

    // General debug toggles
    bool showVoxelEdges() const { return showVoxelEdges_; }
    bool isCullingEnabled() const { return enableCulling; }
    bool useFaceColors() const { return useFaceColors_; }

    // Specific log toggles
    bool logPlayerInfo() const { return logPlayerInfo_; }
    bool logRaycast() const { return logRaycast_; }
    bool logChunkUpdates() const { return logChunkUpdates_; }
    bool logBlockPlacement() const { return logBlockPlacement_; }
    bool logCollision() const { return logCollision_; }
    bool logInventory() const { return logInventory_; } // Added new toggle

    // Setters
    void setShowVoxelEdges(bool enabled) { showVoxelEdges_ = enabled; }
    void setCullingEnabled(bool enabled) { enableCulling = enabled; }
    void setUseFaceColors(bool enabled) { useFaceColors_ = enabled; }
    void setLogPlayerInfo(bool enabled) { logPlayerInfo_ = enabled; }
    void setLogRaycast(bool enabled) { logRaycast_ = enabled; }
    void setLogChunkUpdates(bool enabled) { logChunkUpdates_ = enabled; }
    void setLogBlockPlacement(bool enabled) { logBlockPlacement_ = enabled; }
    void setLogCollision(bool enabled) { logCollision_ = enabled; }
    void setLogInventory(bool enabled) { logInventory_ = enabled; } // Added new setter

private:
    bool showVoxelEdges_;
    bool enableCulling;
    bool useFaceColors_;
    bool logPlayerInfo_;
    bool logRaycast_;
    bool logChunkUpdates_;
    bool logBlockPlacement_;
    bool logCollision_;
    bool logInventory_; // Added new member
};

#endif